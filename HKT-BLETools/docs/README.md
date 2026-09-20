# docs 目录导航

文档按平台分组；各子目录内另有更细的索引（iOS 侧见 `ios/design/README.md`、`ios/architecture/README.md`）。

| 目录 | 状态 | 内容 |
|------|------|------|
| [android/](android/) | **冻结**（止于 V3.21 · 2026-09-02） | Android 研发期全部文档：使用指南、协议参数分析、openspec 规格与变更归档、代码评审 |
| [ios/](ios/) | **活跃**（iOS v1） | 需求总纲、architecture/（软件设计说明·测试基线）、design/（高保真原型·规格清单·ui-spec·对照截图）、operations/（构建装机指南）、reviews/（设计评审） |

## 快速入口

- 高保真原型（UI 唯一事实源）：[ios/design/prototype/index.html](ios/design/prototype/index.html)
- 需求-规格权威清单：[ios/design/2026-09-06-ios-v1-requirements-spec-checklist.md](ios/design/2026-09-06-ios-v1-requirements-spec-checklist.md)
- iOS 构建装机指南：[ios/operations/2026-09-15-ios-build-install-guide.md](ios/operations/2026-09-15-ios-build-install-guide.md)
- 协议契约与测试向量：仓库根 [../shared/protocol/](../shared/protocol/)（金向量在 `../shared/fixtures/`）
- 固件追溯台账：[../shared/devices/firmware-traceability.md](../shared/devices/firmware-traceability.md)
- 协议审计报告：[../shared/protocol/协议审计-2026-09-10.md](../shared/protocol/协议审计-2026-09-10.md)
- 固件缺陷统一台账：monorepo 兄弟目录 `HKT-Firmwares/docs/固件缺陷台账.md`
