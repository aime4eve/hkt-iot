# 部署指南（团队服务器）

平台 = 本仓库内的 `platform/`（Node.js + Fastify + Vue3，无数据库），解码器数据 = 本仓库 `decoders/` 目录。容器以只读挂载方式使用仓库，不修改解码器。

## 首次部署（约 5 分钟）

```bash
# 1. 服务器装好 Docker 与 git 后，克隆本仓库（放哪都行，建议 /opt/HKT-Decoders）
cd /opt
git clone <本仓库远端地址> HKT-Decoders
cd HKT-Decoders

# 2. 构建并启动
docker compose -f platform/docker-compose.yml up -d --build

# 3. 验证
curl -s http://localhost:8620/api/devices | head -c 200
```

浏览器访问 `http://<服务器IP>:8620` 即可使用。内网直接访问，v1 无账号体系；如需加一道口令，用 nginx 反代加 basic auth 即可。

## 日常使用

- **看最新解码器**：页面右上角「⟳ 同步最新」按钮会执行 `git pull --ff-only`。前提：仓库目录的 git 远端已配好且可拉取（有写权限问题就配成只读拉取方式，如 `https://` 或部署密钥）。
- **更新解码器的正确姿势**：本地改 `decoders/` → 按规范改名/加版本/补 changelog → 提交推送 → 平台点「同步最新」。

## 升级平台

```bash
cd /opt/HKT-Decoders
git pull
docker compose -f platform/docker-compose.yml up -d --build
```

## 配置项（环境变量，见 docker-compose.yml）

| 变量 | 默认 | 说明 |
| --- | --- | --- |
| DECODERS_ROOT | 仓库根 | 解码器仓库路径（容器内 /data/repo） |
| PORT | 8620 | 服务端口 |
| DECODE_TIMEOUT_MS | 3000 | 单条负载解码硬超时，超时即熔断并标记该条失败 |

## 健康检查与排障

```bash
docker logs hkt-decoder-platform --tail 50
curl -s http://localhost:8620/api/devices | jq '.devices | length'   # 应为 16
```

- 某条负载一直转圈？该条会被硬超时熔断（默认 3 秒），不会拖垮服务；若要放宽，调大 `DECODE_TIMEOUT_MS`。
- 「同步最新」报错？多为远端认证问题：进容器 `git -C /data/repo pull` 看具体输出。
- 手册 PDF 打不开？确认对应设备目录 `docs/` 里文件存在且 decoder.json 的 `authority.file` 指向正确。

## 安全说明

- 解码脚本在独立 worker 线程 + vm 受限上下文中执行，无网络、无文件系统访问，死循环被双重超时熔断。
- 所有文件读取接口限制在仓库根内，路径穿越返回 404（已测）。
- 服务面向团队内网；不要把 8620 端口直接暴露到公网。
