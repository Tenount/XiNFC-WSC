include $(TOPDIR)/rules.mk

PKG_NAME:=xinfc
PKG_VERSION:=2.0.0
PKG_RELEASE:=1

XINFC_COPYRIGHT_HOLDER:=Tenount
XINFC_COPYRIGHT_YEAR:=2026

PKG_LICENSE:=GPL-3.0-only
PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

include $(INCLUDE_DIR)/package.mk

define Package/xinfc
SECTION:=utils
CATEGORY:=Utilities
TITLE:=NFC configuration tool for Xiaomi routers
URL:=https://github.com/Tenount/XiNFC-WSC
DEPENDS:=+libc
endef

define Package/xinfc/description
CLI tool for writing Wi-Fi credentials to NFC chip on Xiaomi routers.
endef

define Package/xinfc/conffiles
/etc/config/xinfc
endef

define Build/Prepare
	mkdir -p $(PKG_BUILD_DIR)/src
	$(CP) ./src/* $(PKG_BUILD_DIR)/src/
endef

define Build/Compile
	$(TARGET_CC) $(TARGET_CFLAGS) $(TARGET_LDFLAGS) -O2 -Wall -Wextra -Werror -Wshadow -Wconversion -Wformat=2 -Wpedantic \
	-DXINFC_VERSION=\"$(PKG_VERSION)\" \
	-DXINFC_COPYRIGHT_HOLDER=\"$(XINFC_COPYRIGHT_HOLDER)\" \
	-DXINFC_COPYRIGHT_YEAR=\"$(XINFC_COPYRIGHT_YEAR)\" \
	-I$(PKG_BUILD_DIR)/src \
	$(PKG_BUILD_DIR)/src/main.c \
	$(PKG_BUILD_DIR)/src/xinfc-core.c \
	$(PKG_BUILD_DIR)/src/xinfc-utils.c \
	$(PKG_BUILD_DIR)/src/i2c_nfc_device.c \
	$(PKG_BUILD_DIR)/src/wifi.c \
	-o $(PKG_BUILD_DIR)/xinfc-wsc
endef

define Package/xinfc/install
	$(INSTALL_DIR) $(1)/usr/sbin
	$(INSTALL_BIN) $(PKG_BUILD_DIR)/xinfc-wsc $(1)/usr/sbin/
	$(INSTALL_DIR) $(1)/etc/config
	$(INSTALL_CONF) ./root/etc/config/xinfc $(1)/etc/config/xinfc
	$(INSTALL_DIR) $(1)/etc/hotplug.d/iface
	$(INSTALL_BIN) ./files/hotplug-nfc-sync.sh $(1)/etc/hotplug.d/iface/99-nfc-sync
	$(INSTALL_DIR) $(1)/usr/share/xinfc
	$(INSTALL_BIN) ./files/sync-handler.sh $(1)/usr/share/xinfc/sync-handler.sh
	$(INSTALL_BIN) ./files/write-admin.sh $(1)/usr/share/xinfc/write-admin.sh
	$(INSTALL_BIN) ./files/write-wifi.sh $(1)/usr/share/xinfc/write-wifi.sh
	$(INSTALL_BIN) ./files/write-text.sh $(1)/usr/share/xinfc/write-text.sh
	$(INSTALL_BIN) ./files/clear-nfc.sh $(1)/usr/share/xinfc/clear-nfc.sh
	$(INSTALL_DATA) ./files/utils.sh $(1)/usr/share/xinfc/utils.sh
	$(INSTALL_DIR) $(1)/etc/init.d
	$(INSTALL_BIN) ./files/xinfc-sync.init $(1)/etc/init.d/xinfc-sync
endef

# OpenWrt BuildPackage uses $(eval), which double-expands text.
define Package/xinfc/postinst
	#!/bin/sh
	[ -n "$$$${IPKG_INSTROOT}" ] && exit 0
	[ -f /etc/.xinfc-admin-written ] && exit 0
	/usr/share/xinfc/write-admin.sh && touch /etc/.xinfc-admin-written
endef

define Package/xinfc/prerm
	#!/bin/sh
	[ -n "$$$${IPKG_INSTROOT}" ] || rm -f /etc/.xinfc-admin-written
endef

$(eval $(call BuildPackage,xinfc))
