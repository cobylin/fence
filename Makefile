include $(TOPDIR)/rules.mk

PKG_NAME:=fence
PKG_VERSION:=1.0
PKG_RELEASE:=2

include $(INCLUDE_DIR)/package.mk

define Package/fence
  SECTION:=luci
  CATEGORY:=LuCI
  SUBMENU:=3. Applications
  TITLE:=Auto Dual-Stack MMDB Filter
  DEPENDS:=+luci-base +libmaxminddb +libnetfilter-queue +nftables
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)
	$(CP) ./src/* $(PKG_BUILD_DIR)/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) \
		-o $(PKG_BUILD_DIR)/fence-daemon $(PKG_BUILD_DIR)/fence-daemon.c \
		-lmaxminddb -lnetfilter_queue
endef

define Package/fence/install
	$(INSTALL_DIR) $(1)/usr/bin $(1)/etc/config $(1)/etc/init.d \
		$(1)/www/luci-static/resources/view $(1)/usr/share/luci/menu.d
	
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/fence-daemon $(1)/usr/bin/fence-daemon
	$(INSTALL_BIN) ./root/etc/init.d/fence $(1)/etc/init.d/fence
	$(INSTALL_CONF) ./root/etc/config/fence $(1)/etc/config/fence
	$(INSTALL_DATA) ./htdocs/luci-static/resources/view/fence.js $(1)/www/luci-static/resources/view/fence.js
	$(INSTALL_DATA) ./menu.json $(1)/usr/share/luci/menu.d/fence.json
endef

$(eval $(call BuildPackage,fence))
