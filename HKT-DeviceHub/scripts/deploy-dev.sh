#!/usr/bin/env bash
set -Eeuo pipefail

# Deploy HKT-DeviceHub to the dev host (172.17.10.206).
# Usage: ./scripts/deploy-dev.sh
#
# Steps: package → scp artifacts → docker compose up → connectivity self-check.
# Credentials come from the remote shell environment / .env next to the
# compose file; nothing secret is baked into this repo.

REMOTE="hkt@172.17.10.206"
REMOTE_DIR="~/devicehub"
ROOT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
HEALTH_PORT=8080

cd "$ROOT_DIR"

echo "==> [1/4] Building JAR (skip tests)..."
./mvnw -q package -DskipTests
JAR_NAME="$(find target -maxdepth 1 -type f -name 'devicehub-*.jar' ! -name '*sources*' -print -quit)"
if [[ -z "$JAR_NAME" ]]; then
  echo "Deployment aborted: package did not produce a server JAR." >&2
  exit 1
fi
JAR_NAME="${JAR_NAME#target/}"
echo "    built target/$JAR_NAME"

echo "==> [2/4] Syncing artifacts to $REMOTE:$REMOTE_DIR ..."
ssh "$REMOTE" "mkdir -p $REMOTE_DIR/target $REMOTE_DIR/rocketmq"
scp -q "target/$JAR_NAME" "$REMOTE:$REMOTE_DIR/target/$JAR_NAME"
scp -q Dockerfile docker-compose.dev.yml "$REMOTE:$REMOTE_DIR/"
scp -q rocketmq/broker.conf "$REMOTE:$REMOTE_DIR/rocketmq/broker.conf"

echo "==> [3/4] Starting stack on remote..."
# Bind-mounted RocketMQ dirs must be writable by the in-container `rocketmq`
# user (uid/gid 3000). Fix ownership via a throwaway root container so no host
# sudo is needed.
ssh "$REMOTE" "cd $REMOTE_DIR && mkdir -p data/rocketmq/namesrv/logs data/rocketmq/broker/logs data/rocketmq/broker/store && docker run --rm --user root -v $REMOTE_DIR/data/rocketmq:/data --entrypoint sh apache/rocketmq:5.1.4 -c 'chown -R 3000:3000 /data' && docker compose -f docker-compose.dev.yml up -d --build"

echo "==> [4/4] Connectivity self-check (from remote host)..."
ssh "$REMOTE" "
  set -u
  check() { curl -fsS --max-time 5 -o /dev/null \"\$1\" && echo \"[OK]   \$2\" || echo \"[FAIL] \$2\"; }
  reachable() { code=\$(curl -sS --max-time 5 -o /dev/null -w '%{http_code}' \"\$1\" || true); [ \"\$code\" != '000' ] && echo \"[OK]   \$2 (http \$code)\" || echo \"[FAIL] \$2\"; }
  check http://172.22.3.16:8848/nacos/v1/console/health/readiness 'Nacos 172.22.3.16:8848'
  reachable http://172.22.3.105/api/auth/login 'ThingsBoard 172.22.3.105'
  nc -z -w 5 localhost 9876 && echo '[OK]   RocketMQ namesrv 9876' || echo '[FAIL] RocketMQ namesrv 9876'
"

echo "==> Waiting for app health..."
HEALTH_OK=false
for _ in $(seq 1 12); do
  if HEALTH_RESPONSE="$(ssh "$REMOTE" "curl -fsS --max-time 5 http://localhost:$HEALTH_PORT/actuator/health" 2>/dev/null)" \
     && grep -q '"status"[[:space:]]*:[[:space:]]*"UP"' <<<"$HEALTH_RESPONSE"; then
    HEALTH_OK=true
    echo "$HEALTH_RESPONSE"
    break
  fi
  sleep 5
done
if [[ "$HEALTH_OK" != true ]]; then
  echo "WARNING: health endpoint did not return UP yet; check: ssh $REMOTE 'docker logs devicehub-app'" >&2
fi

echo ""
echo "Deploy complete."
echo "  Health:   http://172.17.10.206:$HEALTH_PORT/actuator/health"
echo "  Swagger:  http://172.17.10.206:$HEALTH_PORT/swagger-ui.html"
echo "  RocketMQ Dashboard: http://172.17.10.206:18080"
