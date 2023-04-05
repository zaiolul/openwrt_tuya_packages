include $(TOPDIR)/rules.mk
 
PKG_NAME:=log_daemon
PKG_VERSION:=1.0.0
PKG_RELEASE:=1

include $(INCLUDE_DIR)/package.mk

define Package/log_daemon
	CATEGORY:=Examples
	TITLE:=log_daemon
	DEPENDS:= +tuya-sdk +libubus +libubox +libblobmsg-json
endef

define Package/log_daemon/description
	Communicates with tuya cloud, logs data
endef

define Package/log_daemon/install
	$(INSTALL_DIR) $(1)/usr/bin
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/log_daemon $(1)/usr/bin
	$(INSTALL_BIN) ./files/log_daemon.init $(1)/etc/init.d/log_daemon
	$(INSTALL_CONF) ./files/log_daemon.config $(1)/etc/config/log_daemon

endef

$(eval $(call BuildPackage,log_daemon))