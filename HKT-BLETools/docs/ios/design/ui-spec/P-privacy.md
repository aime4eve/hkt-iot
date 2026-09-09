# 隐私说明页（P_privacy）— 规格卡（从冻结原型逐条提取）

> 事实源：`docs/ios/design/prototype/index.html`（已确认·冻结 2026-09-08）。
> 提取位置：`P_privacy()` L492-499；`PRIVACY` 内容对象 L464-491（ZH/EN 各 11 节，逐字=本卡正文唯一出处，不在本卡重复誊抄）；CSS `.card/.kv`；i18n `privacy="隐私说明/Privacy"`、`privUpdated="生效日期/Effective date"`。
> 原型变更时同一次提交更新本卡（§9 第 4 条）。

---

## 1. 页面结构

```
navbar.small：back 胶囊「‹ 返回」+ 标题「隐私说明」（无首页按钮，用户 2026-09-10 裁决）
body（padding 0 16 24）
  ├─ 生效日期行：kv 13px 居中，margin 2 0 10 →「生效日期: 2026-09-08 · v1.0」
  └─ 11 张卡片（.card：padding 13 14、r10、margin-bottom 11）
       ├─ 节标题：14px/700，margin-bottom 6
       └─ 正文：.kv 13px text2 行高 1.6，white-space:pre-line（\n 换行、• 列表保留）
```

## 2. 内容

- 正文 = 原型 `PRIVACY.zh.secs` / `PRIVACY.en.secs` 逐字移植（11 节：概述 / 我们处理的信息 / 存储与保留 / 无网络传输与第三方 / 相机使用 / 蓝牙权限 / 数据导出 / 您的权利 / 儿童隐私 / 本说明的变更 / 联系我们）。
- 语言随当前语言（跟随系统或设置页覆盖）切换。
- 版本与生效日期：2026-09-08 · v1.0（与 R-21 正式稿一致）。

## 3. iOS 状态

- 入口：设置页「隐私说明」行（P-07 已就位，本批接通）。
- 正文为纯静态内容；无交互元素。
