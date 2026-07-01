#!/bin/sh
# XiNFC installer — fetches latest release from GitHub
# shellcheck shell=dash

set -e

REPO="https://github.com/${GITHUB_REPOSITORY:-Tenount/XiNFC-WSC}"
API="https://api.github.com/repos/${REPO##https://github.com/}/releases/latest"
DOWNLOAD_DIR="/tmp/xinfc-pkgs"
COUNT=3

PKG_IS_APK=0
command -v apk >/dev/null 2>&1 && PKG_IS_APK=1

rm -rf "$DOWNLOAD_DIR"
mkdir -p "$DOWNLOAD_DIR"

RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m'

info() { printf "${GREEN}[INFO]${NC} %s\n" "$1"; }
warn() { printf "${YELLOW}[WARN]${NC} %s\n" "$1"; }
error() {
    printf "${RED}[ERROR]${NC} %s\n" "$1"
    exit 1
}

check_root() {
    [ "$(id -u)" -eq 0 ] || error "Run as root: sudo ./install.sh"
}

detect_package_manager() {
    if [ "$PKG_IS_APK" -eq 1 ]; then
        PKG_MGR="apk"
        PKG_ADD="apk add --allow-untrusted"
        PKG_EXT="apk"
    else
        PKG_MGR="opkg"
        PKG_ADD="opkg install"
        PKG_EXT="ipk"
    fi
    info "Package manager: $PKG_MGR"
}

check_version() {
    local version
    if [ -f /etc/openwrt_release ]; then
        version=$(grep DISTRIB_RELEASE /etc/openwrt_release | cut -d"'" -f2 | cut -d. -f1,2)
    else
        error "Cannot detect OpenWrt version (/etc/openwrt_release not found)"
    fi

    local major
    major=$(echo "$version" | cut -d. -f1)
    local minor
    minor=$(echo "$version" | cut -d. -f2)

    if [ "$major" -lt 24 ] || { [ "$major" -eq 24 ] && [ "$minor" -lt 10 ]; }; then
        error "OpenWrt $version detected. XiNFC requires >= 24.10"
    fi
    info "OpenWrt version: $version (OK)"
}

download_packages() {
    info "Fetching latest release..."

    if [ "${DL_ALL-}" = "1" ]; then
        grep_pat='https://[^"[:space:]]*\.\(ipk\|apk\)'
    else
        grep_pat='https://[^"[:space:]]*\.'"${PKG_EXT}"
    fi

    if [ -n "${GITHUB_TOKEN-}" ]; then
        wget -qO- --header="Authorization: Bearer ${GITHUB_TOKEN}" "$API" | grep -o "$grep_pat" | while read -r url; do
            filename=$(basename "$url")
            filepath="$DOWNLOAD_DIR/$filename"

            attempt=0
            while [ $attempt -lt $COUNT ]; do
                info "Downloading $filename (attempt $((attempt + 1)))..."
                if wget -q --header="Authorization: Bearer ${GITHUB_TOKEN}" -O "$filepath" "$url" && [ -s "$filepath" ]; then
                    info "$filename — OK"
                    break
                fi
                warn "Retrying $filename..."
                rm -f "$filepath"
                attempt=$((attempt + 1))
            done

            if [ $attempt -eq $COUNT ]; then
                warn "Failed to download $filename after $COUNT attempts"
            fi
        done
    else
        wget -qO- "$API" | grep -o "$grep_pat" | while read -r url; do
            filename=$(basename "$url")
            filepath="$DOWNLOAD_DIR/$filename"

            attempt=0
            while [ $attempt -lt $COUNT ]; do
                info "Downloading $filename (attempt $((attempt + 1)))..."
                if wget -q -O "$filepath" "$url" && [ -s "$filepath" ]; then
                    info "$filename — OK"
                    break
                fi
                warn "Retrying $filename..."
                rm -f "$filepath"
                attempt=$((attempt + 1))
            done

            if [ $attempt -eq $COUNT ]; then
                warn "Failed to download $filename after $COUNT attempts"
            fi
        done
    fi

    _found=0
    for _f in "$DOWNLOAD_DIR"/*.ipk "$DOWNLOAD_DIR"/*.apk; do
        [ -f "$_f" ] && _found=1
    done
    if [ "$_found" -eq 0 ]; then
        error "No packages were downloaded"
    fi
}

install_packages() {
    local files="" i18n_file=""
    for f in "$DOWNLOAD_DIR"/*."${PKG_EXT}"; do
        case "$(basename "$f")" in
            luci-i18n-xinfc*) i18n_file="$f" ;;
            *) files="$files $f" ;;
        esac
    done

    for f in $files; do
        case "$(basename "$f")" in
            xinfc-*)
                $PKG_ADD "$f"
                sleep 2
                ;;
        esac
    done
    for f in $files; do
        case "$(basename "$f")" in
            xinfc-*) ;;
            *)
                info "Installing $(basename "$f")..."
                $PKG_ADD "$f"
                sleep 2
                ;;
        esac
    done

    if [ -n "$i18n_file" ]; then
        local RUS=""
        if [ -t 0 ]; then
            info "Install Russian translation? (y/n)"
            read -r RUS
        fi
        case "$RUS" in
            y | Y)
                info "Installing $(basename "$i18n_file")..."
                $PKG_ADD "$i18n_file"
                ;;
        esac
    fi

    info "XiNFC installed successfully!"
}

cleanup() {
    rm -rf "$DOWNLOAD_DIR"
}

show_usage() {
    info "Configure via:"
    info "  LuCI: http://$(uci get network.lan.ipaddr 2>/dev/null || echo '192.168.1.1')/cgi-bin/luci/admin/system/xinfc"
    info "  CLI:  xinfc-wsc <ssid> <password> <encryption>"
}

main() {
    info "XiNFC Installer"
    info "================"
    info ""

    case "${1-}" in
        download)
            detect_package_manager
            DL_ALL=1
            download_packages
            ;;
        *)
            check_root
            detect_package_manager
            check_version
            download_packages
            install_packages
            cleanup
            show_usage
            ;;
    esac
}

main "$@"
