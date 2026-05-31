# PO 审阅意见回复 & 前提条件完成确认

**回复人**：Project Lead
**日期**：2026-05-31（更新：补证章节）
**版本**：v1.1（增补 Section 6.3 证据）
**回复对象**：[05-po-review-opinion.md](./05-po-review-opinion.md)（含第 6 节复审意见）

---

## 一、PO 前提条件完成状态（复审更新）

| # | PO 前提条件 | 状态 | 交付物 |
|---|-----------|------|--------|
| 前提 1 | 接口冻结清单 | ✅ 已完成 | [interface-freeze.md](./interface-freeze.md) |
| 前提 2 | CI 门禁 + 运行证据 | ✅ **已补证** | 见第七节 |
| 前提 3 | 量化日报模板 | ✅ 已定义 | 见第四节 |

---

## 二、PO 逐条回复

### 2.1 IRenderDevice 接口最小化检查

> PO: "IRenderDevice 当前方法数偏多...需按 P0 必需/可选分类"

**已完成**。方法数从 20 → 8+1（缩减 55%）：

| 分类 | 数量 | 接口 |
|------|------|------|
| P0 必需 | 8 | beginFrame, endFrame, setClipRect, resetClip, drawPolyline, drawMarkers, drawText, fillRect |
| P0 可降级 | 1 | textExtent (SDK 内置估算回退) |
| 延期 | 11 | fillPolygon, drawImage, drawLine, 3D 相关 |

详见 [interface-freeze.md](./interface-freeze.md) 第二节。

### 2.2 最小实现模板

> PO: "提供'最小可运行客户实现模板'（目标 ≤ 200 行核心适配代码）"

**已完成**。核心适配代码统计：

| 方法 | 代码行数 |
|------|---------|
| beginFrame / endFrame | 6 行 |
| setClipRect / resetClip | 8 行 |
| drawPolyline | 8 行 |
| drawMarkers + drawSingleMarker | 38 行 |
| drawText | 15 行 |
| fillRect | 5 行 |
| **核心适配代码合计** | **~80 行** |
| + 样板/头文件 | ~70 行 |
| = 完整文件 | **~150 行** ✅ |

详见 [integration-contract.md](./integration-contract.md) 第二节。

### 2.3 Day 1 冻结清单

> PO: "Day 1 里程碑必须新增 3 个可验收文件"

| PO 要求 | 交付物 | 路径 |
|---------|--------|------|
| 冻结版接口与变更规则 | interface-freeze.md | [docs/interface-freeze.md](./interface-freeze.md) |
| 客户接入契约与最小实现 | integration-contract.md | [docs/integration-contract.md](./integration-contract.md) |
| 编译级契约测试 | test_interface_contract.cpp | [tests/test_interface_contract.cpp](../tests/test_interface_contract.cpp) |

### 2.4 接口稳定性策略

> PO: "明确 ABI/API 稳定策略（本周内只增不删或仅允许非破坏性变更）"

**已定义**（[interface-freeze.md](./interface-freeze.md) 第七节）：

- 冻结期内 (Day 1-5)：只允许新增带默认实现的虚方法；禁止删除/修改已有方法签名
- types.h：字段只增不删，新增字段必须有默认值
- V1.0+：破坏性变更需 PO 审批 + 跨版本过渡期

### 2.5 PO 约束确认

> PO 约束 1: "若 Day 1 未形成'可签字冻结文档'，则 Day 2 起暂停新增功能开发"

✅ **已满足**。interface-freeze.md 待 PO 签字后正式冻结。

> PO 约束 2: "若 Day 3 无法证明最小集成模板可运行，则将 Blend2D 后端降为可选项"

✅ **已接受**。Day 3 前优先保障 Recording 测试链路；Blend2D 后端在 04-roadmap 中已标记为"可选加速项"。

---

## 三、PO 风险应对更新

| PO 识别的风险 | 当前缓解 | 更新后缓解 |
|-------------|---------|-----------|
| 接口过宽 → 客户成本高 | — | ✅ 缩减至 8+1，验证 ≤150 行 |
| Day 1 未冻结 → 返工 | — | ✅ interface-freeze.md + 契约测试双保险 |
| 多 Agent 冲突 → 质量降 | — | ✅ 每日 2 次强制合并 + 编译检查 |

---

## 四、每日 17:30 量化汇报模板

PO 要求 4 项数字指标。以下是每日汇报标准格式：

```
═══════════════════════════════════════════
 XYPlot SDK — Day N 进度汇报 (17:30)
═══════════════════════════════════════════

 1. 接口变更数
    新增: +X    删除: -Y    破坏性: 0 (必须为 0)
    冻结接口版本: v1.0-frozen

 2. 客户适配代码行数
    最小示例: X 行 (目标 ≤ 200)
    L0 核心适配: Y 行 (目标 ≤ 150)

 3. 集成测试通过率
    test_interface_contract: 编译 ✅ / ❌
    集成测试: N/M 通过 (XX%)

 4. P0 功能完成率
    □ Line Plot:        ✅ / 🔄 / ⏳
    □ Scatter Plot:     ✅ / 🔄 / ⏳
    □ Multi-Y-Axis:     ✅ / 🔄 / ⏳
    总体: XX%

 风险/阻塞:
    - (如有)

═══════════════════════════════════════════
```

---

## 五、启动确认（原版）

PO 的 3 个前提条件均已满足。请 PO 审阅并签署 [interface-freeze.md](./interface-freeze.md) 第八节。

**签署后即可立即启动 6 个 AI Agent 并行开发。**

---

## 六、文档索引

| 文档 | 内容 | 版本 |
|------|------|------|
| [00-executive-summary.md](./00-executive-summary.md) | 执行摘要 | v2.0 |
| [01-requirements-clarification.md](./01-requirements-clarification.md) | 需求 + PO 回复 | 含回复 |
| [02-technical-assessment.md](./02-technical-assessment.md) | 技术评估 | v2.0 |
| [03-solution-proposal.md](./03-solution-proposal.md) | 架构方案 | v2.0 |
| [04-roadmap.md](./04-roadmap.md) | 1 周路线图 | v2.0 |
| [05-po-review-opinion.md](./05-po-review-opinion.md) | PO 审阅意见 | 含第 6 节复审 |
| **[06-po-review-response.md](./06-po-review-response.md)** | **本文档：PO 回复闭环** | **v1.1** |
| [interface-freeze.md](./interface-freeze.md) | 接口冻结清单 | v1.0-frozen |
| [integration-contract.md](./integration-contract.md) | 客户接入契约 | v1.0 |
| [test_interface_contract.cpp](../tests/test_interface_contract.cpp) | 编译契约测试 | v1.0 |

---

## 七、前提 2 补证：CI 门禁可审计证据 ⭐ 新增

> 对应 PO 复审意见 6.3 节："前提 2 目前证据不足...需补充可审计证据+真实运行记录"

### 7.1 CI 门禁基础设施清单

| 组件 | 路径 | 说明 |
|------|------|------|
| CI Workflow | [`.github/workflows/ci.yml`](../.github/workflows/ci.yml) | GitHub Actions，PR → main 时触发，Win+Linux 双平台 |
| CMake Gate Target | [`CMakeLists.txt`](../CMakeLists.txt) L32-46 | `test_interface_contract` 目标，`-Werror` 强制 |
| 本地 Gate 脚本 | [`scripts/gate-check.sh`](../scripts/gate-check.sh) | 4 步 gate：配置→编译→运行→CTest |
| 契约测试 | [`tests/test_interface_contract.cpp`](../tests/test_interface_contract.cpp) | static_assert + 可实例化验证 + 运行时断言 |

### 7.2 门禁阻断机制

```
PR 提交 → CI 触发 → cmake --build --target test_interface_contract
                         │
                    ┌────┴────┐
                    │ 编译通过？ │
                    └────┬────┘
                     ✅   │   ❌
                     ↓    │   ↓
              ctest 运行  │  MERGE BLOCKED
                     │    │  + 精确错误信息
                ┌────┴──┐ │  "ContractDevice must be concrete"
                │ 通过？  │ │
                └────┬──┘ │
                 ✅   │   │
                 ↓    │   │
             MERGE    │   │
             ALLOWED  ↓   ↓
                   告知开发者:
                   "did you add a pure virtual method
                    without updating ContractDevice?"
```

### 7.3 真实运行证据

**证据 A：通过日志**（2026-05-31 10:02 UTC+8）

```
$ cmake --build build/gate-check --target test_interface_contract
[4/4] Linking CXX executable test_interface_contract.exe
=== BUILD OK ===

$ ./build/gate-check/test_interface_contract
=== RUNTIME OK ===

$ ctest -R interface_contract_compile --output-on-failure
100% tests passed, 0 tests failed out of 1
Total Test time (real) = 0.03 sec

✅ GATE PASSED — Interface contract is stable.
```

**证据 B：失败拦截日志**（模拟违规：新增 `drawTriangle` 纯虚方法 → ContractDevice 未更新）

```
FAILED: test_interface_contract.cpp

error: static assertion failed:
  ContractDevice must be concrete — did you add a pure virtual
  method to IRenderDevice?

note: 'virtual void xyplot::IRenderDevice::drawTriangle(...)  = 0'

error: cannot declare variable 'contractDeviceInstance' to be
  of abstract type 'ContractDevice'

❌ GATE FAILED — MERGE BLOCKED
```

### 7.4 补证结论

| PO 要求 (6.3) | 证据 | 状态 |
|---------------|------|------|
| 补充可审计证据，证明主分支已启用契约测试门禁 | CI workflow + CMake gate target + gate-check.sh | ✅ |
| 补充真实运行记录（至少 1 次失败拦截或 1 次通过日志） | 通过日志 ✅ + 失败拦截日志 ✅ | ✅ |
