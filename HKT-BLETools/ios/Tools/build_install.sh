#!/usr/bin/env bash
# HKT BLETools iOS 编译 + 真机装机一条龙（免费个人团队开发签名，复用智慧畜牧装机经验）
#
# 用法:
#   ./Tools/build_install.sh                    # Release 构建 → IPA → 装到第一台可用 iPhone → 启动
#   ./Tools/build_install.sh -c Debug           # Debug 构建
#   ./Tools/build_install.sh -d <设备ID>         # 指定 devicectl 设备 ID（xcrun devicectl list devices 查询）
#   ./Tools/build_install.sh -d none            # 只编译打包 IPA，不装机
#   ./Tools/build_install.sh -o <输出目录>       # IPA 输出目录（默认 ios/build/ipa）
#   ./Tools/build_install.sh --no-launch        # 装完不启动
#
# 前提:
#   - Xcode 已登录团队账号（J62PU4Y357，免费个人团队，工程内已配置自动签名）
#   - iPhone 已与 Mac 配对（USB 或同 Wi-Fi），`xcrun devicectl list devices` 可见
#
# 免费账号注意: 签名 profile 7 天过期，App 在手机上打不开时重跑本脚本即续命。
# 免费团队 3 个 App ID 并发上限；新设备首次装机需构建时连着 Mac（自动注册进 profile）。

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

while [[ $# -gt 0 ]]; do
  case "$1" in
    -c) CONFIG="$2"; shift 2 ;;
    -d) DEVICE="$2"; shift 2 ;;
    -o) OUT_DIR="$2"; shift 2 ;;
    --no-launch) LAUNCH=0; shift ;;
    *) echo "未知参数: $1（看文件头注释）" >&2; exit 1 ;;
  esac
done

say() { printf '\n==> %s\n' "$*"; }

mkdir -p "$OUT_DIR"

# 1. 编译（真机切片 + 自动签名；-allowProvisioningUpdates 静默刷新免费团队 profile）
say "xcodebuild 编译：$CONFIG / 真机（generic/platform=iOS）"
LOG="$IOS_DIR/build/last_build.log"
mkdir -p "$(dirname "$LOG")"
if ! xcodebuild -project "$PROJECT" -scheme HKTBLETools \
      -configuration "$CONFIG" \
      -destination 'generic/platform=iOS' \
      -derivedDataPath "$DERIVED" \
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
IPA="$OUT_DIR/HKTBLETools-v$VERSION-b$BUILD-$(date +%Y%m%d-%H%M).ipa"
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
  DEVICE=$(xcrun devicectl list devices 2>/dev/null | awk -F'  +' '/available/ {print $3; exit}')
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

say "完成 ✓  （免费签名 7 天过期；App 打不开时重跑本脚本续命）"
