CONTAINER_ENGINE := env("CONTAINER_ENGINE", "podman")

default:
    @just --list

# Lint + auto-fix all issues (style, formatting, etc.)
lint-fix:
    {{CONTAINER_ENGINE}} image inspect megalinter-xinfc:latest >/dev/null 2>&1 || {{CONTAINER_ENGINE}} build --build-arg TARGETARCH={{ arch() }} -f docker/Dockerfile.megalinter -t megalinter-xinfc:latest .
    echo "Reusing megalinter-xinfc:latest image"
    {{CONTAINER_ENGINE}} run --rm \
        -e APPLY_FIXES=all \
        -v $(pwd):/tmp/lint -w /tmp/lint \
        megalinter-xinfc:latest

# Lint without auto-fix (for CI / read-only)
lint:
    {{CONTAINER_ENGINE}} image inspect megalinter-xinfc:latest >/dev/null 2>&1 || {{CONTAINER_ENGINE}} build --build-arg TARGETARCH={{ arch() }} -f docker/Dockerfile.megalinter -t megalinter-xinfc:latest .
    echo "Reusing megalinter-xinfc:latest image"
    {{CONTAINER_ENGINE}} run --rm \
        -e APPLY_FIXES=none \
        -v $(pwd):/tmp/lint -w /tmp/lint \
        megalinter-xinfc:latest

# Quick format-only (assumes image already built)
fmt:
    {{CONTAINER_ENGINE}} run --rm \
        -e APPLY_FIXES=all \
        -e ENABLE_LINTERS="C_CLANG_FORMAT,C_CLANG_TIDY,BASH_SHFMT,YAML_PRETTIER,JAVASCRIPT_ES,JAVASCRIPT_PRETTIER" \
        -e JAVASCRIPT_DEFAULT_STYLE="prettier" \
        -v $(pwd):/tmp/lint -w /tmp/lint \
        megalinter-xinfc:latest

# Install git hooks (one-time setup)
hooks:
    pre-commit install

# Check docs freshness
drift:
    drift check

# All checks (read-only — fails on findings)
check: lint drift test

# Auto-fix all fixable issues (redundant declarations, format, etc.)
fix: lint-fix

# Run all tests (smoke + unit)
test: test-smoke test-unit

# Smoke tests only
test-smoke:
    {{CONTAINER_ENGINE}} build -f docker/Dockerfile.dev -t xinfc-dev:latest .
    {{CONTAINER_ENGINE}} run --rm \
        -v $(pwd):/workspace -w /workspace \
        xinfc-dev:latest \
        sh -c "cd tests/unit && make smoke"

# Unit tests only
test-unit:
    {{CONTAINER_ENGINE}} build -f docker/Dockerfile.dev -t xinfc-dev:latest .
    {{CONTAINER_ENGINE}} run --rm \
        -v $(pwd):/workspace -w /workspace \
        xinfc-dev:latest \
        sh -c "cd tests/unit && make test"

SDK_VERSIONS := "v24.10.7 v25.12.4"

# Build SDK container image from source (one-time, slow)
build-image:
    {{CONTAINER_ENGINE}} build --no-cache --platform linux/amd64 \
        --build-arg OPENWRT_VERSION=24.10.7 \
        -t xinfc-builder \
        -f docker/Dockerfile .

# Build all .ipk/.apk packages in official SDK containers (same as CI)
build:
    #!/usr/bin/env bash
    set -euo pipefail
    CE={{CONTAINER_ENGINE}}
    mkdir -p release
    for tag in {{SDK_VERSIONS}}; do
        cid=$($CE run -d --platform linux/amd64 \
            -v "$(pwd):/src:ro,Z" \
            "openwrt/sdk:mediatek-filogic-$tag" \
            /bin/sh -c '
                set -e
                ./scripts/feeds update -a
                ./scripts/feeds install luci-base
                cp -r /src/package/xinfc /builder/package/xinfc
                cp -r /src/package/luci-app-xinfc /builder/package/feeds/luci/luci-app-xinfc
                echo "CONFIG_PACKAGE_xinfc=y" > .config
                echo "CONFIG_PACKAGE_luci-app-xinfc=y" >> .config
                make defconfig
                make package/xinfc/compile V=s -j4
                make package/luci-app-xinfc/compile V=s -j4
            '
        )
        echo "--- SDK $tag ---"
        rc=$($CE wait "$cid")
        $CE logs "$cid" 2>&1
        echo "Exit: $rc"
        $CE cp "$cid:/builder/bin/packages/." ./release/ 2>/dev/null
        $CE rm "$cid" 2>/dev/null
    done
    find ./release/ -name "*.ipk" -exec mv -t ./release/ {} + 2>/dev/null
    find ./release/ -name "*.apk" -exec mv -t ./release/ {} + 2>/dev/null
    rm -rf ./release/aarch64_cortex-a53 2>/dev/null
    echo "--- Packages ---"
    ls -la ./release/
