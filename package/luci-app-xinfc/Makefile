include $(TOPDIR)/rules.mk

PKG_NAME:=luci-app-xinfc
PKG_VERSION:=2.0.0
PKG_RELEASE:=1

PKG_BUILD_DIR:=$(BUILD_DIR)/$(PKG_NAME)-$(PKG_VERSION)

LUCI_TITLE:=LuCI interface for XiNFC configuration
LUCI_DEPENDS:=+luci-base +xinfc
LUCI_PKGARCH:=all
LUCI_URL:=https://github.com/Tenount/XiNFC-WSC
LUCI_LANG.ru:=Русский (Russian)

include $(TOPDIR)/feeds/luci/luci.mk

$(eval $(call BuildPackage,$(PKG_NAME)))
