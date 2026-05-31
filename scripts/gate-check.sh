#!/bin/bash
# ============================================================
# gate-check.sh — XYPlot SDK Gate Check Script
# ============================================================
# Owner: Agent B (基础设施)
#
# 用途: 本地门禁脚本 — CI gate 的本地等价物。
#
# 模式:
#   bash scripts/gate-check.sh           默认: 仅契约门禁
#   bash scripts/gate-check.sh --full    全量: 构建 + 全部测试
#   bash scripts/gate-check.sh --quick   快速: 跳过 clean reconfigure
#
# PO 前提 2: "主分支启用接口契约测试，未通过不得合并。"
# ============================================================

set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
PROJECT_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
BUILD_DIR="$PROJECT_DIR/build/gate-check"
LOG_DIR="$PROJECT_DIR/build"
LOG_FILE="$LOG_DIR/gate-check.log"

# ──── Parse arguments ────
FULL_MODE=false
QUICK_MODE=false

for arg in "$@"; do
    case "$arg" in
        --full)  FULL_MODE=true ;;
        --quick) QUICK_MODE=true ;;
        *)       echo "Usage: $0 [--full] [--quick]"; exit 2 ;;
    esac
done

# ──── Banner ────
echo ""
echo "═══════════════════════════════════════════"
echo " XYPlot SDK — Interface Contract Gate"
echo " Date: $(date '+%Y-%m-%d %H:%M:%S')"
echo " Mode: $([ "$FULL_MODE" = true ] && echo 'FULL (build + tests)' || echo 'CONTRACT (gate only)')"
echo "═══════════════════════════════════════════"
echo ""

# ──── Ensure log directory exists ────
mkdir -p "$LOG_DIR"

# ═══════════════════════════════════════════
# STEP 1: Configure
# ═══════════════════════════════════════════
echo "[1/5] Configure CMake..."
if [ "$QUICK_MODE" = false ]; then
    rm -rf "$BUILD_DIR"
fi
cmake -B "$BUILD_DIR" -DCMAKE_BUILD_TYPE=Debug 2>&1 | tee "$LOG_FILE"
echo "  ✅ Configure done."
echo ""

# ═══════════════════════════════════════════
# STEP 2: Build Contract Test (GATE)
# ═══════════════════════════════════════════
echo "[2/5] Build contract test (GATE)..."
if cmake --build "$BUILD_DIR" --target test_interface_contract --config Debug 2>&1 | tee -a "$LOG_FILE"; then
    echo "  ✅ BUILD PASSED — ContractDevice implements all pure virtual methods."
else
    echo ""
    echo "┌─────────────────────────────────────────┐"
    echo "│  ❌❌❌  GATE FAILED: BUILD ERROR  ❌❌❌  │"
    echo "└─────────────────────────────────────────┘"
    echo ""
    echo "The contract test FAILED TO COMPILE."
    echo "This means IRenderDevice has an incompatible change."
    echo ""
    echo "Possible causes:"
    echo "  1. A new pure virtual method was added to IRenderDevice"
    echo "     but ContractDevice was NOT updated in"
    echo "     tests/test_interface_contract.cpp"
    echo "  2. A method signature was changed incompatibly."
    echo "  3. A type definition in include/xyplot/types.h was changed."
    echo ""
    echo "REQUIRED FIX:"
    echo "  1. Read docs/interface-freeze.md for change control policy."
    echo "  2. Update tests/test_interface_contract.cpp."
    echo "  3. Re-run: bash scripts/gate-check.sh"
    echo ""
    echo "❌ MERGE BLOCKED until this passes."
    exit 1
fi
echo ""

# ═══════════════════════════════════════════
# STEP 3: Run Contract Test
# ═══════════════════════════════════════════
echo "[3/5] Run contract test..."
if "$BUILD_DIR/test_interface_contract" 2>&1 | tee -a "$LOG_FILE"; then
    echo "  ✅ RUNTIME PASSED — All assertions verified."
else
    echo ""
    echo "┌──────────────────────────────────────────┐"
    echo "│ ❌❌❌  GATE FAILED: RUNTIME ERROR  ❌❌❌  │"
    echo "└──────────────────────────────────────────┘"
    echo "Contract test compiled but assertions failed at runtime."
    exit 1
fi
echo ""

# ═══════════════════════════════════════════
# STEP 4: CTest Verification
# ═══════════════════════════════════════════
echo "[4/5] CTest verification..."
cd "$BUILD_DIR"
if ctest -R interface_contract_compile --output-on-failure 2>&1 | tee -a "$LOG_FILE"; then
    echo "  ✅ CTEST PASSED."
else
    echo "  ❌ CTEST FAILED."
    exit 1
fi
echo ""

# ═══════════════════════════════════════════
# STEP 5: Full Build + All Tests (if --full)
# ═══════════════════════════════════════════
if [ "$FULL_MODE" = true ]; then
    echo "[5/5] Full build (all sources)..."
    if cmake --build "$BUILD_DIR" --config Debug 2>&1 | tee -a "$LOG_FILE"; then
        echo "  ✅ FULL BUILD PASSED."
    else
        echo ""
        echo "┌──────────────────────────────────────────┐"
        echo "│  ⚠️  FULL BUILD FAILED                   │"
        echo "└──────────────────────────────────────────┘"
        echo "The contract gate passed, but the full build has errors."
        echo "Check the log above for details."
        exit 1
    fi
    echo ""

    echo "  Running all tests..."
    if ctest --output-on-failure 2>&1 | tee -a "$LOG_FILE"; then
        echo "  ✅ ALL TESTS PASSED."
    else
        echo "  ⚠️  SOME TESTS FAILED."
        exit 1
    fi
    echo ""
else
    echo "[5/5] Skipped — use --full for complete build + all tests."
    echo ""
fi

# ═══════════════════════════════════════════
# Gate Passed
# ═══════════════════════════════════════════
echo "═══════════════════════════════════════════"
echo " ✅ GATE PASSED — All checks green."
echo "    Interface contract is stable."
echo "    Merge is allowed."
echo "═══════════════════════════════════════════"
echo ""
echo "Audit log: $LOG_FILE"
exit 0
