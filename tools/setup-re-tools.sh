#!/usr/bin/env bash
# Idempotent setup of the reverse-engineering toolchain (REA + JADX) in a fresh container.
set -euo pipefail

JADX_VERSION=1.5.1
MCP_JAR_SHA=6e5eacf500b64292bfb73c49797c1958f6ee44646e43e868039ae7feb573ff75
REA_DIR=/home/user/morluto/rea

# JADX CLI
if ! command -v jadx >/dev/null; then
  mkdir -p /opt/jadx /tmp/jadx-dl
  curl -fsSL -o /tmp/jadx-dl/jadx.zip "https://github.com/skylot/jadx/releases/download/v${JADX_VERSION}/jadx-${JADX_VERSION}.zip"
  unzip -q -o /tmp/jadx-dl/jadx.zip -d /opt/jadx
  ln -sf /opt/jadx/bin/jadx /usr/local/bin/jadx
fi

# jadx-headless-mcp (the engine REA drives), pinned and hash-checked
mkdir -p /opt/jadx-mcp
JAR=/opt/jadx-mcp/jadx-headless-mcp-0.7.1-all.jar
if [ ! -f "$JAR" ]; then
  curl -fsSL -o "$JAR" https://github.com/1013503897/jadx-headless-mcp/releases/download/v0.7.1/jadx-headless-mcp-0.7.1-all.jar
fi
echo "${MCP_JAR_SHA}  ${JAR}" | sha256sum -c -

# REA (clone your fork instead by setting REA_REPO)
if [ ! -d "$REA_DIR/.git" ]; then
  GIT_LFS_SKIP_SMUDGE=1 git clone --depth 1 "${REA_REPO:-https://github.com/morluto/rea}" "$REA_DIR"
fi
cd "$REA_DIR"
[ -d node_modules ] || npm ci --ignore-scripts
[ -d dist ] || npm run build

echo "export REA_JADX_MCP_JAR=$JAR" > /etc/profile.d/rea.sh
echo "Done. Use: REA_JADX_MCP_JAR=$JAR node $REA_DIR/scripts/rea.mjs --help"
