# Container toolchain (reverse engineering)

Run `bash tools/setup-re-tools.sh` at the start of any new session. It is idempotent.
The container is wiped when idle, so nothing below survives unless this script reruns.

| Tool | Version | Location | Notes |
| --- | --- | --- | --- |
| Node.js / npm | 22.22 / 10.9 | preinstalled | meets REA's requirement (>=22.19) |
| OpenJDK (full, with javac) | 21 | preinstalled | needed by JADX |
| JADX CLI | 1.5.1 | /opt/jadx, `jadx` on PATH | installed by script |
| jadx-headless-mcp | 0.7.1 | /opt/jadx-mcp | sha256-pinned; set `REA_JADX_MCP_JAR` |
| REA (rea-agents) | 6.7.0 | /home/user/morluto/rea | `node scripts/rea.mjs <cmd>` |
| objdump, unzip, curl, python3, ps | system | /usr/bin | objdump is a supported REA native provider |

Not available: Hopper, IDA, adb device access, Ghidra, apktool, tshark.
Outbound network is proxied and allowlisted; a blocked host returns 403.

Scope: analyze apps you own or are authorized to examine (see REA's DISCLAIMER.md).
