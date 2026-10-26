#!/usr/bin/env bash
# HKT BLETools App Store / TestFlight 出包：Archive → 商店签名 IPA →（可选）上传 ASC
#
# 版本映射（用户裁决 2026-10-14）：对内标准串（VERSION 文件，如 v6.2.1-b1）→
# 商店要求纯数字：CFBundleShortVersionString=6.2.1，CFBundleVersion=1
# （TestFlight/App Store 显示为 6.2.1 (1)）。
#
# 用法:
#   ./Tools/build_appstore.sh [-s]
#       Archive + 导出 App Store 签名 IPA（不上传；产物在 ios/build/appstore/）
#       -s/--skip-archive 复用已有 xcarchive 只重跑导出（改签名/证书后重试用）
#   ./Tools/build_appstore.sh --upload
#       同上，但导出目的地=upload：签名后直接上传 App Store Connect（进 TestFlight）。
#       认证走 Xcode 已登录账号会话（Settings→Accounts 的 tech@hktchn.com），
#       无需 API key；分发证书用 Cloud Managed Apple Distribution（自动创建/复用）。
#       【CI/无 GUI 会话场景的备选】altool + ASC API key：
#         xcrun altool --upload-app --type ios -f <ipa> \
#           --apiKey <KeyID> --apiIssuer <IssuerID>
#         （.p8 放 ~/.appstoreconnect/private_keys/AuthKey_<KeyID>.p8）
#
# 前提:
#   - ASC 后台已建 App 记录（Bundle ID com.hkt.ble.bletools.ios），否则上传被拒
#   - Apple Distribution 证书在钥匙串（B51D29F9，2026-10-14 建）；
#     分发 profile 由 -allowProvisioningUpdates 自动创建
#   - 上传后：内部测试立即可装（免审核）；外部测试需填 Beta 测试说明并过轻量审核
#   - 首次处理构建时 ASC 会问"App 是否使用加密"：本 App 无网络无加密框架，
#     答"否/仅豁免"（ITSAppUsesNonExemptEncryption 语义）即可
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
IOS_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
PROJECT="$IOS_DIR/HKTBLETools.xcodeproj"
SCHEME="HKTBLETools"
TEAM="QQC56X32GR"
BUNDLE_ID="com.hkt.ble.bletools.ios"

DO_UPLOAD=0
SKIP_ARCHIVE=0
for arg in "$@"; do
  case "$arg" in
    --upload) DO_UPLOAD=1 ;;
    -s|--skip-archive) SKIP_ARCHIVE=1 ;;
    *) echo "未知参数: $arg（看文件头注释）" >&2; exit 1 ;;
  esac
done

say() { printf '\n==> %s\n' "$*"; }

# ---- 版本解析：v6.2.1-b1 → SHORT=6.2.1 / BUILD=1 ----
VERSION_FILE="$IOS_DIR/../VERSION"
[[ -f "$VERSION_FILE" ]] || { echo "缺少版本文件: ${VERSION_FILE}" >&2; exit 1; }
FULL_VERSION=$(tr -d "[:space:]" < "$VERSION_FILE")
if [[ "$FULL_VERSION" =~ ^v([0-9]+\.[0-9]+\.[0-9]+)-b([0-9]+)$ ]]; then
  SHORT_VERSION="${BASH_REMATCH[1]}"
  BUILD_NUMBER="${BASH_REMATCH[2]}"
else
  echo "VERSION 需为 v<主>.<次>.<修订>-b<N>（如 v6.2.1-b1），当前: $FULL_VERSION" >&2
  exit 1
fi
say "商店版本映射：${FULL_VERSION} → ShortVersion=${SHORT_VERSION} / Build=${BUILD_NUMBER}"

# ---- 1. Archive ----
ARCHIVE_PATH="$IOS_DIR/build/HKTBLETools.xcarchive"
LOG="$IOS_DIR/build/last_appstore_build.log"
mkdir -p "$(dirname "$LOG")"
if [[ "$SKIP_ARCHIVE" -eq 1 && -d "$ARCHIVE_PATH" ]]; then
  say "跳过 Archive，复用: $ARCHIVE_PATH"
else
  rm -rf "$ARCHIVE_PATH"
  say "xcodebuild archive（Release / generic iOS / 自动签名）"
  if ! xcodebuild -project "$PROJECT" -scheme "$SCHEME" \
        -configuration Release \
        -destination 'generic/platform=iOS' \
        -archivePath "$ARCHIVE_PATH" \
        MARKETING_VERSION="$SHORT_VERSION" CURRENT_PROJECT_VERSION="$BUILD_NUMBER" \
        -allowProvisioningUpdates \
        archive 2>&1 | tee "$LOG"; then
    echo "Archive 失败，完整日志: $LOG" >&2
    exit 1
  fi
fi

# ---- 2. 导出（App Store Connect 签名；upload 模式=签名后直接上传 ASC）----
EXPORT_DIR="$IOS_DIR/build/appstore"
PLIST="$IOS_DIR/build/ExportOptions-appstore.plist"
rm -rf "$EXPORT_DIR"; mkdir -p "$EXPORT_DIR"
EXPORT_DESTINATION="export"
[[ "$DO_UPLOAD" -eq 1 ]] && EXPORT_DESTINATION="upload"
cat > "$PLIST" <<EOF
<?xml version="1.0" encoding="UTF-8"?>
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">
<plist version="1.0">
<dict>
	<key>method</key>
	<string>app-store-connect</string>
	<key>teamID</key>
	<string>${TEAM}</string>
	<key>signingStyle</key>
	<string>automatic</string>
	<key>stripSwiftSymbols</key>
	<true/>
	<key>uploadSymbols</key>
	<true/>
	<key>destination</key>
	<string>${EXPORT_DESTINATION}</string>
</dict>
</plist>
EOF

say "xcodebuild -exportArchive（app-store-connect 签名，destination=${EXPORT_DESTINATION} ）"
if ! xcodebuild -exportArchive \
      -archivePath "$ARCHIVE_PATH" \
      -exportPath "$EXPORT_DIR" \
      -exportOptionsPlist "$PLIST" \
      -allowProvisioningUpdates 2>&1 | tee -a "$LOG"; then
  echo "导出/上传失败（upload 模式常见原因：ASC 未建 App 记录），完整日志: $LOG" >&2
  exit 1
fi

if [[ "$DO_UPLOAD" -eq 1 ]]; then
  say "已提交 App Store Connect。处理约几分钟~半小时，之后 ASC → TestFlight 页可见构建 ${SHORT_VERSION} (${BUILD_NUMBER})"
else
  IPA_SRC=$(ls "$EXPORT_DIR"/*.ipa 2>/dev/null | head -1)
  [[ -n "$IPA_SRC" ]] || { echo "导出目录无 IPA: $EXPORT_DIR" >&2; exit 1; }
  IPA_OUT="$IOS_DIR/build/ipa/HKTBLETools-${FULL_VERSION}-appstore-$(date +%Y%m%d).ipa"
  mkdir -p "$(dirname "$IPA_OUT")"
  cp -f "$IPA_SRC" "$IPA_OUT"
  echo "App Store IPA: $IPA_OUT ($(du -h "$IPA_OUT" | cut -f1))"
  # 核对包内版本串与签名身份（app-store 方式导出只有 IPA，解包检查）
  rm -rf /tmp/bt_as_check && mkdir -p /tmp/bt_as_check && unzip -q "$IPA_SRC" -d /tmp/bt_as_check
  APP_DIR=$(ls -d /tmp/bt_as_check/Payload/*.app 2>/dev/null | head -1)
  if [[ -n "$APP_DIR" ]]; then
    /usr/libexec/PlistBuddy -c 'Print CFBundleShortVersionString' -c 'Print CFBundleVersion' "$APP_DIR/Info.plist"
    codesign -d --verbose=2 "$APP_DIR" 2>&1 | grep -E "Authority=|TeamIdentifier" | head -3
  fi
  say "未上传。上传：./Tools/build_appstore.sh --upload（需 ASC 已建 App 记录）"
fi
