#!/usr/bin/env bash
# HKT-BLETools 双端发布打包一条龙：APK + IPA 版本号强制一致（用户裁决 2026-09-28）
#
# 唯一版本事实源 = 仓库根 VERSION 文件（内容如 V6.1.1b30）。APK/IPA 的
# 文件名、系统版本号（Android versionName / iOS CFBundleShortVersionString）
# 与 App 内关于页显示全部同源，不再存在双端各自计数。
#
# 用法:
#   ./release.sh                 # 用当前 VERSION 双端打包（重打包/续签用，不改版本号）
#   ./release.sh --bump          # 先把 VERSION 的 b<N> 尾数 +1，再双端打包（发新版用）
#   ./release.sh V6.2.0b1        # 显式设定版本号（写入 VERSION），再双端打包
#
# 产物:
#   releases/android/HKTBLETools-<版本>-<日期>.apk
#   releases/ios/HKTBLETools-<版本>-<日期>.ipa

set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
VERSION_FILE="$ROOT/VERSION"
JBR17="/Applications/Android Studio.app/Contents/jbr/Contents/Home"
say() { printf '\n==> %s\n' "$*"; }

[[ -f "$VERSION_FILE" ]] || { echo "缺少版本文件: ${VERSION_FILE}（内容如 V6.1.1b30，双端同源）" >&2; exit 1; }
current_version() { tr -d "[:space:]" < "$VERSION_FILE"; }

case "${1:-}" in
  "") ;;
  --bump)
    V=$(current_version)
    NUM=$(printf '%s' "$V" | sed -E 's/^.*b([0-9]+)$/\1/')
    [[ "$NUM" =~ ^[0-9]+$ ]] || { echo "VERSION 末尾需为 b<数字>（如 V6.1.1b30），当前: $V" >&2; exit 1; }
    NEW=$((NUM + 1))
    printf '%s\n' "${V%"b$NUM"}b$NEW" > "$VERSION_FILE"
    say "版本升级：$V → $(current_version)"
    ;;
  V*)
    printf '%s\n' "$1" > "$VERSION_FILE"
    say "版本设定：$1"
    ;;
  *) echo "未知参数: ${1}（看文件头注释）" >&2; exit 1 ;;
esac

VERSION=$(current_version)
DATE=$(date +%Y%m%d)
[[ "$VERSION" =~ ^V[0-9.]+b[0-9]+$ ]] || { echo "版本号格式需为 V<主>.<次>.<修订>b<N>（如 V6.1.1b30），当前: $VERSION" >&2; exit 1; }
say "本次版本：${VERSION}（${DATE}）"

# ---- Android：prod-debug（debug 签名可直接安装）----
say "Android assembleProdDebug"
JAVA_HOME="$JBR17" "$ROOT/android/gradlew" -p "$ROOT/android" assembleProdDebug \
  -Dorg.gradle.java.home="$JBR17" --console=plain
APK_SRC="$ROOT/android/app/build/outputs/apk/prod/debug/app-prod-debug.apk"
[[ -f "$APK_SRC" ]] || { echo "找不到 APK 产物: $APK_SRC" >&2; exit 1; }
APK_OUT="$ROOT/releases/android/HKTBLETools-$VERSION-$DATE.apk"
mkdir -p "$(dirname "$APK_OUT")"
cp "$APK_SRC" "$APK_OUT"
echo "APK: $APK_OUT ($(du -h "$APK_OUT" | cut -f1))"

# ---- iOS：Release 真机切片 → IPA（不装机；装机用 ios/Tools/build_install.sh）----
say "iOS Release → IPA"
"$ROOT/ios/Tools/build_install.sh" -d none -o "$ROOT/releases/ios"

say "完成 ✓  双端版本号：$VERSION"
