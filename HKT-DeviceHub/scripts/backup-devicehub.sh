#!/usr/bin/env bash
# backup-devicehub.sh — DeviceHub postgres 逻辑备份 + 配置打包，rsync 推 123 异地
# 安装：hkt@wukong(206) crontab 每日 19:30 UTC（北京 03:30 低峰）
# 保留：本地与 123 各 14 天；成败写 ~/devicehub/backups/last-backup-status
set -euo pipefail

TS="$(date -u +%Y%m%d-%H%M%S)"
STAMP="$(date -u '+%F %T UTC')"
BASE="$HOME/devicehub"
BKDIR="$BASE/backups"
REMOTE="agentic@172.22.1.123"
REMOTE_DIR="/data/agentic/devicehub-backups"
KEEP_DAYS=14
LOG="$BKDIR/backup-$TS.log"

mkdir -p "$BKDIR"
exec >>"$LOG" 2>&1
echo "===== backup start $STAMP ====="

fail() {
  echo "FAILED: $1"
  echo "FAIL $STAMP: $1" > "$BKDIR/last-backup-status"
  exit 1
}

# 1) postgres 逻辑备份（容器内 pg_dump，自定义格式）
DUMP="$BKDIR/devicehub-$TS.dump"
docker exec devicehub-postgres pg_dump -U devicehub -Fc devicehub > "$DUMP" || fail "pg_dump exited non-zero"
[ -s "$DUMP" ] || fail "dump file empty"
# 完整性自检：归档可列出目录
docker exec -i devicehub-postgres pg_restore --list < "$DUMP" >/dev/null || fail "pg_restore --list failed (archive broken)"
echo "dump ok: $(du -h "$DUMP" | cut -f1)"

# 2) 配置打包（compose/.env/Dockerfile/broker.conf —— 重建除了数据全靠它）
CFG="$BKDIR/devicehub-configs-$TS.tar.gz"
tar -czf "$CFG" -C "$BASE" docker-compose.dev.yml .env Dockerfile rocketmq || fail "config tar failed"
chmod 600 "$CFG" "$DUMP"   # .env 内含凭据

# 3) 推 123 异地
ssh -o BatchMode=yes -o ConnectTimeout=10 "$REMOTE" "mkdir -p $REMOTE_DIR" || fail "ssh to 123 failed"
rsync -az --timeout=120 "$DUMP" "$CFG" "$REMOTE:$REMOTE_DIR/" || fail "rsync to 123 failed"
echo "offsite ok"

# 4) 双边保留 14 天
find "$BKDIR" -name "devicehub-*.dump" -mtime +$KEEP_DAYS -delete
find "$BKDIR" -name "devicehub-configs-*.tar.gz" -mtime +$KEEP_DAYS -delete
find "$BKDIR" -name "backup-*.log" -mtime +$KEEP_DAYS -delete
ssh -o BatchMode=yes "$REMOTE" "find $REMOTE_DIR -name 'devicehub-*.dump' -mtime +$KEEP_DAYS -delete; find $REMOTE_DIR -name 'devicehub-configs-*.tar.gz' -mtime +$KEEP_DAYS -delete" || true

echo "SUCCESS $STAMP"
echo "OK   $STAMP  dump=$(du -h "$DUMP" | cut -f1) cfg=$(du -h "$CFG" | cut -f1)" > "$BKDIR/last-backup-status"
