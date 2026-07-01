FROM debian:bookworm-slim

SHELL ["/bin/bash", "-e", "-u", "-o", "pipefail", "-c"]

ARG OPENWRT_VERSION=24.10
ARG TARGET=mediatek/filogic

ENV OPENWRT_VERSION=${OPENWRT_VERSION}
ENV TARGET=${TARGET}

RUN apt-get update && apt-get install -y --no-install-recommends \
    build-essential \
    gawk \
    git-core \
    libncurses5-dev \
    libssl-dev \
    python3 \
    rsync \
    subversion \
    swig \
    unzip \
    wget \
    zlib1g-dev \
    file \
    ca-certificates \
    zstd \
    && rm -rf /var/lib/apt/lists/*

RUN useradd -m -s /bin/bash builder

USER builder
WORKDIR /home/builder

RUN SDK_URL="https://downloads.openwrt.org/releases/${OPENWRT_VERSION}/targets/${TARGET}"; \
    echo "Downloading SDK from $SDK_URL"; \
    wget -q "$SDK_URL/" -O /tmp/sdk-page.html; \
    SDK_FILE=$(python3 -c "import sys,re; m=re.search(r'openwrt-sdk-[^\"]+\.tar\.zst',sys.stdin.read()); print(m.group(0) if m else '')" < /tmp/sdk-page.html); \
    rm -f /tmp/sdk-page.html; \
    echo "Found: $SDK_FILE"; \
    test -n "$SDK_FILE" || exit 1; \
    wget -q "$SDK_URL/$SDK_FILE"; \
    tar --zstd -xf "$SDK_FILE"; \
    rm -f "$SDK_FILE"; \
    SDK_DIR=$(find . -maxdepth 1 -type d -name 'openwrt-sdk-*' -print -quit); \
    mv "$SDK_DIR" sdk; \
    cd sdk; \
    echo "src-gz base https://downloads.openwrt.org/releases/${OPENWRT_VERSION}/targets/${TARGET}/packages" >> feeds.conf; \
    ./scripts/feeds update base luci packages 2>&1; \
    ./scripts/feeds install luci-base 2>&1

WORKDIR /home/builder/sdk
CMD ["/bin/bash"]
