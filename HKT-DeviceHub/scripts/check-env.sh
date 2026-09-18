#!/usr/bin/env bash
set -Eeuo pipefail

# Environment connectivity check for HKT-DeviceHub.
# Verifies: Nacos config/discovery, ThingsBoard, RocketMQ namesrv, local postgres.
# Usage: ./scripts/check-env.sh

NACOS_ADDR="${NACOS_SERVER_ADDR:-172.22.3.16:8848}"
TB_ADDR="${TB_BASE_URL:-http://172.22.3.105}"
ROCKETMQ_NS="${ROCKETMQ_NAME_SERVER:-172.17.10.206:9876}"
DB_HOST_PORT="${DB_HOST_PORT:-localhost:5432}"

ok=0
fail=0

check_tcp() {
  local name="$1" host="$2" port="$3"
  if nc -z -G 5 "$host" "$port" >/dev/null 2>&1; then
    echo "[OK]   $name ($host:$port)"
    ok=$((ok + 1))
  else
    echo "[FAIL] $name ($host:$port)"
    fail=$((fail + 1))
  fi
}

check_http() {
  local name="$1" url="$2"
  if curl -fsS --max-time 5 -o /dev/null "$url" 2>/dev/null; then
    echo "[OK]   $name ($url)"
    ok=$((ok + 1))
  else
    echo "[FAIL] $name ($url)"
    fail=$((fail + 1))
  fi
}

check_http "Nacos"        "http://$NACOS_ADDR/nacos/v1/console/health/readiness"
check_http "ThingsBoard"  "$TB_ADDR/api/auth/login"
check_tcp  "RocketMQ namesrv" "${ROCKETMQ_NS%%:*}" "${ROCKETMQ_NS##*:}"
check_tcp  "PostgreSQL"  "${DB_HOST_PORT%%:*}" "${DB_HOST_PORT##*:}"

echo ""
echo "env check: $ok ok, $fail failed"
[ "$fail" -eq 0 ]
