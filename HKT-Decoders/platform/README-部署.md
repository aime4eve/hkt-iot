# 部署指南（172.22.1.123 已按此部署）

平台 = `platform/`（Node.js + Fastify + Vue3，无数据库），解码器数据 = 平台专属数据目录 `DATA_DIR`（**不在代码包里，不入 git，不随部署分发**；解码器通过平台「导入」功能进入数据目录）。

服务器现状（2026-09-17 部署）：`agentic@172.22.1.123`，代码在 `~/hkt-decoder-platform/`，数据在 `~/hkt-decoder-data/`，systemd 用户服务 `hkt-decoder-platform`（Linger=yes 开机自启），端口 8620。

## 首次部署（全新服务器）

前提：服务器有 Node.js ≥ 18 与 SSH 账号；无需 root、无需 docker。

```bash
# 1. 本机打包（排除开发依赖与数据目录）
cd HKT-Decoders
tar --exclude='platform/node_modules/playwright-core' --exclude='platform/platform-data' \
    -czf /tmp/hkt-decoder-platform.tar.gz platform

# 2. 上传解压（依赖为纯 JS，node_modules 可直接随包走）
scp /tmp/hkt-decoder-platform.tar.gz user@server:/tmp/
ssh user@server
mkdir -p ~/hkt-decoder-platform ~/hkt-decoder-data
tar -xzf /tmp/hkt-decoder-platform.tar.gz -C ~/hkt-decoder-platform --strip-components=1

# 3. systemd 用户服务（若 Linger 未开启，请管理员执行 loginctl enable-linger <user>）
mkdir -p ~/.config/systemd/user
cat > ~/.config/systemd/user/hkt-decoder-platform.service <<EOF
[Unit]
Description=HKT Decoder Platform (负载解码器管理调试平台)
After=network.target

[Service]
WorkingDirectory=%h/hkt-decoder-platform
Environment=DATA_DIR=%h/hkt-decoder-data
Environment=PORT=8620
Environment=NODE_ENV=production
ExecStart=/usr/bin/node server.js
Restart=always
RestartSec=3

[Install]
WantedBy=default.target
EOF
systemctl --user daemon-reload
systemctl --user enable --now hkt-decoder-platform
```

## 初始数据录入（平台导入，不是拷目录）

```bash
# 本机把 decoders/ 打成 zip，浏览器顶栏「⬆ 导入」上传；或命令行：
cd HKT-Decoders/decoders && zip -qr /tmp/decoders-import.zip . 
curl -X POST http://server:8620/api/import -F "file=@/tmp/decoders-import.zip"
```

导入策略：新增设备直接入库；**已存在的设备只补缺文件，绝不覆盖**服务器上的台账与已修改内容——重复导入是安全的。

## 升级平台代码

```bash
# 本机重新打包 scp 后，服务器上：
tar -xzf /tmp/hkt-decoder-platform.tar.gz -C ~/hkt-decoder-platform --strip-components=1
systemctl --user restart hkt-decoder-platform
```

数据目录与升级无关，解码器数据不受影响。

## 日常运维

| 操作 | 命令 |
| --- | --- |
| 状态/日志 | `systemctl --user status hkt-decoder-platform` / `journalctl --user -u hkt-decoder-platform -f` |
| 重启 | `systemctl --user restart hkt-decoder-platform` |
| 备份 | 页面「⬇ 导出备份」下载全量 zip；或 `tar -czf backup.tar.gz ~/hkt-decoder-data` |
| 恢复 | 停服务 → 解开备份到 `~/hkt-decoder-data` → 起服务 |

## 配置（环境变量，见 unit 文件）

| 变量 | 默认 | 说明 |
| --- | --- | --- |
| DATA_DIR | `platform/../platform-data`（本地开发） | 解码器数据目录（服务器上 `~/hkt-decoder-data`） |
| PORT | 8620 | 服务端口 |
| DECODE_TIMEOUT_MS | 3000 | 单条解码硬超时（死循环熔断） |

## 安全说明

- 解码脚本在 worker 线程 + vm 受限上下文执行：无网络、无文件系统，双超时熔断。
- 文件读写锁死在数据目录内，路径穿越返回 404（已测）。
- 面向团队内网；写接口（管理/导入）无鉴权，**不要暴露公网**。需要加门禁时用 nginx 反代 + basic auth。
- 备份建议：每周「导出备份」存档一次，或直接备份 `~/hkt-decoder-data` 目录。

## Docker（可选，未用于当前部署）

`Dockerfile` + `docker-compose.yml` 仍在仓库内（compose 将仓库目录挂为 DECODERS_ROOT 的旧形态已过时，用 docker 部署时请按 DATA_DIR 卷挂载自行调整）。当前生产采用 systemd 裸 Node 方案，更贴合该服务器权限（无 sudo）。
