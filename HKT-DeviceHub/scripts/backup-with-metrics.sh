#!/usr/bin/env bash
# backup-with-metrics.sh — 备份包装层：执行原备份脚本并把成败写成 Prometheus textfile 指标
# 设计约束：本层任何错误都不得影响备份本体（退出码透传、写指标失败只放弃不报错）
# 指标（206 node-exporter textfile 采集器暴露，123 Prometheus 抓取）：
#   devicehub_backup_last_run_status                 1=最近一次尝试成功 0=失败
#   devicehub_backup_last_run_timestamp_seconds      最近一次尝试的 Unix 时间戳
#   devicehub_backup_last_success_timestamp_seconds  最近一次成功的 Unix 时间戳（仅成功时写）
#   devicehub_backup_duration_seconds                最近一次尝试耗时
# 告警联动（smart-parking 仓 alerts.yml）：
#   DeviceHubBackupFailed  status==0
#   DeviceHubBackupStale   time()-last_success>90000(25h)，指标整个缺失同样触发
# crontab（206，hkt）：
#   30 19 * * * $HOME/devicehub/scripts/backup-with-metrics.sh >> $HOME/devicehub/backups/metrics-wrapper.log 2>&1
set -u

BASE="$HOME/devicehub"
METRICS_DIR="$BASE/backups/metrics"
OUT="$METRICS_DIR/devicehub_backup.prom"
START_TS="$(date +%s)"

RC=0
bash "$BASE/scripts/backup-devicehub.sh" || RC=$?
END_TS="$(date +%s)"

STATUS=0
[ "$RC" -eq 0 ] && STATUS=1

# 原子写（tmp+mv），644 供 node-exporter(nobody) 读；失败静默放弃——
# 指标缺失时 BackupStale（or vector(0) 兜底）会告警，不会静默假绿
if mkdir -p "$METRICS_DIR" 2>/dev/null \
  && TMP="$(mktemp "$METRICS_DIR/.backup.XXXXXX" 2>/dev/null)"; then
  {
    echo "# HELP devicehub_backup_last_run_status Latest backup attempt result (1=ok 0=failed)"
    echo "# TYPE devicehub_backup_last_run_status gauge"
    echo "devicehub_backup_last_run_status $STATUS"
    echo "# HELP devicehub_backup_last_run_timestamp_seconds Unix time of latest backup attempt"
    echo "# TYPE devicehub_backup_last_run_timestamp_seconds gauge"
    echo "devicehub_backup_last_run_timestamp_seconds $END_TS"
    echo "# HELP devicehub_backup_duration_seconds Duration of latest backup attempt"
    echo "# TYPE devicehub_backup_duration_seconds gauge"
    echo "devicehub_backup_duration_seconds $((END_TS - START_TS))"
    if [ "$STATUS" -eq 1 ]; then
      echo "# HELP devicehub_backup_last_success_timestamp_seconds Unix time of latest successful backup"
      echo "# TYPE devicehub_backup_last_success_timestamp_seconds gauge"
      echo "devicehub_backup_last_success_timestamp_seconds $END_TS"
    fi
  } > "$TMP" && chmod 644 "$TMP" 2>/dev/null && mv -f "$TMP" "$OUT" 2>/dev/null
fi

exit "$RC"
