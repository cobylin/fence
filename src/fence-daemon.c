#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <linux/netfilter.h>
#include <libnetfilter_queue/libnetfilter_queue.h>
#include <maxminddb.h>
#include <time.h>
#include <syslog.h>

/* 实例上下文结构 */
struct db_instance {
    char name[64];
    int queue_id;
    float ratio;
    MMDB_s mmdb;
};

/* 数据包回调：自动识别 IPv4/IPv6 */
static int cb(struct nfq_q_handle *qh, struct nfgenmsg *nfmsg, struct nfq_data *nfa, void *data) {
    struct db_instance *inst = (struct db_instance *)data;
    uint32_t id = 0;
    struct nfqnl_msg_packet_hdr *ph = nfq_get_msg_packet_hdr(nfa);
    if (ph) id = ntohl(ph->packet_id);

    unsigned char *payload;
    int len = nfq_get_payload(nfa, &payload);
    
    /* 基础长度检查 */
    if (len < 20) return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);

    char ip_str[INET6_ADDRSTRLEN];
    int ip_version = (payload[0] >> 4); // 获取 IP 版本号

    if (ip_version == 4) {
        /* IPv4 解析: 目的地址在偏移 16 字节处，长度 4 字节 */
        inet_ntop(AF_INET, payload + 16, ip_str, sizeof(ip_str));
    } else if (ip_version == 6 && len >= 40) {
        /* IPv6 解析: 目的地址在偏移 24 字节处，长度 16 字节 */
        inet_ntop(AF_INET6, payload + 24, ip_str, sizeof(ip_str));
    } else {
        /* 未知协议版本，直接放行 */
        return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
    }

    /* 在 MMDB 中查询地址 (MMDB_lookup_string 支持双栈字符串输入) */
    int gai_error, mmdb_error;
    MMDB_lookup_result_s result = MMDB_lookup_string(&inst->mmdb, ip_str, &gai_error, &mmdb_error);

    if (result.found_entry) {
        /* 拦截决策 */
        if (inst->ratio >= 1.0f || ((float)rand() / (float)RAND_MAX) < inst->ratio) {
            syslog(LOG_INFO, "[%s] Blocked %s: %s", inst->name, (ip_version == 6 ? "IPv6" : "IPv4"), ip_str);
            return nfq_set_verdict(qh, id, NF_DROP, 0, NULL);
        }
    }

    return nfq_set_verdict(qh, id, NF_ACCEPT, 0, NULL);
}

int main(int argc, char **argv) {
    openlog("fence-daemon", LOG_PID, LOG_DAEMON);
    if (argc < 2) return 1;
    srand(time(NULL));

    struct nfq_handle *h = nfq_open();
    if (!h) return 1;

    /* 参数解析循环 */
    for (int i = 1; i < argc; i++) {
        char *arg = strdup(argv[i]);
        char *name = strtok(arg, ":");
        char *qid_str = strtok(NULL, ":");
        char *ratio_str = strtok(NULL, ":");
        char *path = strtok(NULL, ":");

        if (!name || !qid_str || !ratio_str || !path) continue;

        struct db_instance *inst = malloc(sizeof(struct db_instance));
        strncpy(inst->name, name, 63);
        inst->queue_id = atoi(qid_str);
        inst->ratio = atof(ratio_str);

        /* 采用内存映射打开库 */
        if (MMDB_open(path, MMDB_MODE_MMAP, &inst->mmdb) == MMDB_SUCCESS) {
            struct nfq_q_handle *qh = nfq_create_queue(h, inst->queue_id, &cb, inst);
            nfq_set_mode(qh, NFQNL_COPY_PACKET, 0xffff);
            syslog(LOG_NOTICE, "Group [%s] started on Queue %d (Ratio %.2f)", inst->name, inst->queue_id, inst->ratio);
        } else {
            syslog(LOG_ERR, "Failed to load [%s] at %s", inst->name, path);
            free(inst);
        }
    }

    /* 事件循环 */
    int fd = nfq_fd(h);
    char buf[4096] __attribute__ ((aligned));
    while (1) {
        int rv = recv(fd, buf, sizeof(buf), 0);
        if (rv >= 0) nfq_handle_packet(h, buf, rv);
    }
    return 0;
}
