#!/usr/bin/env bash
# HKT BLETools iOS 编译 + 真机装机一条龙
#
# 用法:
#   ./Tools/build_install.sh                    # Release 构建 → IPA → 装到第一台可用 iPhone → 启动
#   ./Tools/build_install.sh -c Debug           # Debug 构建
#   ./Tools/build_install.sh -d <设备ID>         # 指定 devicectl 设备 ID（xcrun devicectl list devices 查询）
#   ./Tools/build_install.sh -d none            # 只编译打包 IPA，不装机
#   ./Tools/build_install.sh -o <输出目录>       # IPA 输出目录（默认 ios/build/ipa）
#   ./Tools/build_install.sh -t <TeamID>        # 临时覆盖签名团队（不改 pbxproj）
#   ./Tools/build_install.sh --no-launch        # 装完不启动
#
# 前提:
#   - Xcode 已登录正式 Apple 开发者账号 tech@hktchn.com（Apple Developer Program）：
#     团队 Hunan HKT Technology Co., Ltd.，Team ID QQC56X32GR（2026-10-14 起为 pbxproj 默认，
#     证书/profile 有效期约 1 年；Xcode → Settings → Accounts 管理）。
#     历史账号：免费个人团队 J62PU4Y357（sales@hktlora.com），其包 7 天过期，已于 2026-10-14 弃用。
#   - iPhone 已与 Mac 配对（USB 或同 Wi-Fi），`xcrun devicectl list devices` 可见
#
# 团队切换: 正常情况不带 -t 即用 pbxproj 里的 DEVELOPMENT_TEAM（QQC56X32GR）；
# 需临时用其他团队时 `./Tools/build_install.sh -t <TeamID>`。
#
# 实战坑（2026-10-14）:
#   - 新团队首次出包若报 "team has no devices"：generic 目标不触发设备注册，
#     先以真机为目标跑一次（xcodebuild -destination 'id=<UDID>' ...）或 portal 手动加设备；
#   - 换签名团队后覆盖安装会被拒（application-identifier 前缀从 J62PU4Y357. 变 QQC56X32GR.），
#     先 `xcrun devicectl device uninstall app --device <ID> com.hkt.ble.bletools.ios` 再装（App 本地数据清空）。

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IOS_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROJECT="$IOS_DIR/HKTBLETools.xcodeproj"
BUNDLE_ID="com.hkt.ble.bletools.ios"
DERIVED="$IOS_DIR/build/DerivedData"

CONFIG="Release"
DEVICE="auto"          # auto=第一台 available 的 iPhone；none=不装机
OUT_DIR="$IOS_DIR/build/ipa"
LAUNCH=1
TEAM="${XCODE_TEAM:-}"  # 空 = 用 pbxproj 里的 DEVELOPMENT_TEAM；-t 可覆盖

while [[ $# -gt 0 ]]; do
  case "$1" in
    -c) CONFIG="$2"; shift 2 ;;
    -d) DEVICE="$2"; shift 2 ;;
    -o) OUT_DIR="$2"; shift 2 ;;
    -t) TEAM="$2"; shift 2 ;;
    --no-launch) LAUNCH=0; shift ;;
    *) echo "未知参数: $1（看文件头注释）" >&2; exit 1 ;;
  esac
done

say() { printf '\n==> %s\n' "$*"; }

mkdir -p "$OUT_DIR"

# 1. 编译（真机切片 + 自动签名；-allowProvisioningUpdates 静默刷新 profile）
# 版本方案（用户裁决 2026-09-28）：双端版本号必须一致——唯一事实源 = 仓库根 VERSION 文件
# （标准格式 v6.1.1-b30，用户裁决 2026-10-14：<应用名称>v<主版本号>-b<第N次编译后发布>）。
# 打包不再自动递增（旧 ios/build-counter.txt 独立计数已废）；版本升级用 ./release.sh --bump 或手工编辑 VERSION。
# CURRENT_PROJECT_VERSION 取 -b 后数字，保证同基线内单调。
VERSION_FILE="$IOS_DIR/../VERSION"
[[ -f "$VERSION_FILE" ]] || { echo "缺少版本文件: ${VERSION_FILE}（内容如 v6.1.1-b30，双端同源）" >&2; exit 1; }
FULL_VERSION=$(tr -d "[:space:]" < "$VERSION_FILE")
BUILD_NUM=$(printf '%s' "$FULL_VERSION" | sed -E 's/^.*-b([0-9]+)$/\1/')
[[ "$BUILD_NUM" =~ ^[0-9]+$ ]] || { echo "VERSION 末尾需为 -b<数字>（如 v6.1.1-b30），当前: $FULL_VERSION" >&2; exit 1; }
say "版本：${FULL_VERSION}"

say "xcodebuild 编译：$CONFIG / 真机（generic/platform=iOS）"
TEAM_ARGS=()
[[ -n "$TEAM" ]] && { TEAM_ARGS=("DEVELOPMENT_TEAM=$TEAM"); say "签名团队覆盖：$TEAM"; }
LOG="$IOS_DIR/build/last_build.log"
mkdir -p "$(dirname "$LOG")"
if ! xcodebuild -project "$PROJECT" -scheme HKTBLETools \
      -configuration "$CONFIG" \
      -destination 'generic/platform=iOS' \
      -derivedDataPath "$DERIVED" \
      ${TEAM_ARGS[@]+"${TEAM_ARGS[@]}"} \
      MARKETING_VERSION="$FULL_VERSION" CURRENT_PROJECT_VERSION="$BUILD_NUM" \
      -allowProvisioningUpdates -allowProvisioningDeviceRegistration \
      build 2>&1 | tee "$LOG"; then
  echo "构建失败，完整日志: $LOG" >&2
  exit 1
fi

# 2. 打 IPA（xcodebuild 产物已签名，Payload 直压即可；plist 读真实版本号）
say "打包 IPA"
APP_DIR="$DERIVED/Build/Products/$CONFIG-iphoneos/HKTBLETools.app"
[[ -d "$APP_DIR" ]] || { echo "找不到构建产物: $APP_DIR" >&2; exit 1; }
PB=/usr/libexec/PlistBuddy
VERSION=$($PB -c 'Print CFBundleShortVersionString' "$APP_DIR/Info.plist")
BUILD=$($PB -c 'Print CFBundleVersion' "$APP_DIR/Info.plist")
# 命名规范（与 Android releases 统一）：HKTBLETools-<版本>-<日期>.ipa；版本串已含构建数（v6.1.1-bN）
IPA="$OUT_DIR/HKTBLETools-$VERSION-$(date +%Y%m%d).ipa"
rm -rf "$OUT_DIR/Payload"
mkdir -p "$OUT_DIR/Payload"
cp -R "$APP_DIR" "$OUT_DIR/Payload/"
(cd "$OUT_DIR" && zip -qry "$(basename "$IPA")" Payload && rm -rf Payload)
echo "IPA: $IPA ($(du -h "$IPA" | cut -f1))"

# 3. 设备解析
if [[ "$DEVICE" == "none" ]]; then
  say "跳过装机（-d none）"
  exit 0
fi
if [[ "$DEVICE" == "auto" ]]; then
  # 注意 /available/ 会误匹配 "unavailable"，必须显式排除
  DEVICE=$(xcrun devicectl list devices 2>/dev/null | awk -F'  +' '/available/ && !/unavailable/ {print $3; exit}')
  if [[ -z "$DEVICE" ]]; then
    echo "未找到可用 iPhone（xcrun devicectl list devices 检查连接/配对），IPA 保留在: $IPA" >&2
    exit 1
  fi
fi

# 4. 安装
say "安装到设备 $DEVICE"
xcrun devicectl device install app --device "$DEVICE" "$IPA"

# 5. 启动
if [[ "$LAUNCH" -eq 1 ]]; then
  say "启动 $BUNDLE_ID"
  xcrun devicectl device process launch --device "$DEVICE" "$BUNDLE_ID"
fi

say "完成 ✓  （正式团队 profile 约 1 年有效；历史免费团队包 7 天过期，App 打不开时重打重装）"
