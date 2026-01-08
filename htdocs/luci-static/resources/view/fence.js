'use strict';
'use ui';

var fs = require('fs');
var ui = require('luci.ui');
var uci = require('luci.uci');

return L.view.extend({
    handleDownload: function(section_id) {
        var url = uci.get('fence', section_id, 'url');
        if (!url) {
            ui.addNotification(null, E('p', _('请输入 URL')), 'danger');
            return;
        }

        ui.showModal(_('正在下载'), [E('p', { 'class': 'spinning' }, _('保持原名下载中...'))]);

        // 直接在 /etc/fence 目录下使用 wget --content-disposition
        // 注意：这会在目录下生成文件，我们需要在下载后检测文件名
        return fs.exec('/usr/bin/mkdir', ['-p', '/etc/fence']).then(function() {
            return fs.exec('/usr/bin/wget', [
                '-q', '--show-progress', '--content-disposition', 
                '-P', '/etc/fence', url
            ]).then(function(res) {
                ui.hideModal();
                if (res.code === 0) {
                    ui.addNotification(null, E('p', _('下载成功！请在下载完成后手动刷新或检查文件路径。')), 'info');
                    // 提示：此处可以通过 fs.list 寻找最新文件来自动更新 path，但手动填写/刷新最稳妥
                } else {
                    ui.addNotification(null, E('p', _('下载失败')), 'danger');
                }
            });
        });
    },

    render: function() {
        var m, s, o;
        m = new form.Map('fence', _('Fence'), _('双栈自动识别流量拦截器'));

        s = m.section(form.TypedSection, 'database', _('拦截分组'));
        s.addremove = true;
        s.anonymous = true;

        s.option(form.Value, 'name', _('备注名称'));
        s.option(form.Flag, 'enabled', _('启用'));
        
        o = s.option(form.Value, 'url', _('下载 URL'));
        
        o = s.option(form.Value, 'path', _('库文件绝对路径'));
        o.placeholder = _('/etc/fence/your_file.mmdb');
        o.description = _('下载成功后请确保此路径指向实际文件');

        o = s.option(form.Button, '_download', _('操作'));
        o.inputtitle = _('立即下载 (保持原名)');
        o.inputstyle = 'apply';
        o.onclick = L.bind(this.handleDownload, this);

        s.option(form.Value, 'ratio', _('拦截比例')).default = '1.0';

        // 移除了 queue_id 选项，由后台自动分配索引

        return m.render();
    }
});
