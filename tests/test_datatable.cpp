// ============================================================
// test_datatable.cpp — DataTable unit tests
// ============================================================
// Owner: Agent C
// §12 Phase C: Quality Deepening — P0 task
// Tests: fromMemory, fromCSV, column access, metadata, edge cases
// ============================================================
#include "../src/datatable.h"
#include <cassert>
#include <cstdio>
#include <cmath>
#include <cstring>
#include <fstream>
#include <string>
#include <vector>

static int g_testsPassed = 0;
static int g_testsFailed = 0;

#define TEST(name) \
    do { printf("  TEST: %s ... ", name); fflush(stdout); } while(0)

#define PASS() \
    do { printf("PASSED\n"); g_testsPassed++; } while(0)

#define CHECK(cond) \
    do { \
        if (!(cond)) { \
            printf("FAILED\n    Assertion: %s\n", #cond); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

#define CHECK_CLOSE(a, b, eps) \
    do { \
        double _a = (a), _b = (b); \
        if (std::abs(_a - _b) > (eps)) { \
            printf("FAILED\n    Expected ~%g, got %g (eps=%g)\n", _b, _a, (double)(eps)); \
            g_testsFailed++; \
            return; \
        } \
    } while(0)

// ──── Helper: create a temporary CSV file ────
static std::string writeTempCSV(const std::string& content) {
    std::string path = "test_temp_datatable.csv"; // CTest runs from build dir
    std::ofstream f(path);
    f << content;
    f.close();
    return path;
}

// ──── Test 1: fromMemory — normal 3×2 matrix ────
void test_fromMemory_normal() {
    TEST("fromMemory normal 3rows × 2cols");
    const double data[] = {1.0, 10.0,  2.0, 20.0,  3.0, 30.0};  // row-major
    const char* names[] = {"x", "y"};

    auto table = xyplot::DataTable::fromMemory(data, 3, 2, names);
    CHECK(table.rowCount() == 3);
    CHECK(table.colCount() == 2);
    CHECK(table.columnName(0) == "x");
    CHECK(table.columnName(1) == "y");

    // column-major: column 0 = {1, 2, 3}
    const double* col0 = table.column(0);
    CHECK(col0 != nullptr);
    CHECK_CLOSE(col0[0], 1.0, 1e-10);
    CHECK_CLOSE(col0[1], 2.0, 1e-10);
    CHECK_CLOSE(col0[2], 3.0, 1e-10);

    const double* col1 = table.column(1);
    CHECK(col1 != nullptr);
    CHECK_CLOSE(col1[0], 10.0, 1e-10);
    CHECK_CLOSE(col1[1], 20.0, 1e-10);
    CHECK_CLOSE(col1[2], 30.0, 1e-10);
    PASS();
}

// ──── Test 2: fromMemory — null data ────
void test_fromMemory_null_data() {
    TEST("fromMemory null data → empty table");
    auto table = xyplot::DataTable::fromMemory(nullptr, 5, 2, nullptr);
    CHECK(table.rowCount() == 0);
    CHECK(table.colCount() == 0);
    PASS();
}

// ──── Test 3: fromMemory — zero rows ────
void test_fromMemory_zero_rows() {
    TEST("fromMemory 0 rows → empty table");
    const double data[] = {1.0, 2.0};
    auto table = xyplot::DataTable::fromMemory(data, 0, 2, nullptr);
    CHECK(table.rowCount() == 0);
    CHECK(table.colCount() == 0);
    PASS();
}

// ──── Test 4: fromMemory — zero cols ────
void test_fromMemory_zero_cols() {
    TEST("fromMemory 0 cols → empty table");
    const double data[] = {1.0, 2.0};
    auto table = xyplot::DataTable::fromMemory(data, 3, 0, nullptr);
    CHECK(table.rowCount() == 0);
    CHECK(table.colCount() == 0);
    PASS();
}

// ──── Test 5: fromMemory — single row single col ────
void test_fromMemory_1x1() {
    TEST("fromMemory 1 row × 1 col");
    const double data[] = {42.0};
    auto table = xyplot::DataTable::fromMemory(data, 1, 1, nullptr);
    CHECK(table.rowCount() == 1);
    CHECK(table.colCount() == 1);
    CHECK(table.columnName(0) == "col0");  // auto-named
    const double* col = table.column(0);
    CHECK(col != nullptr);
    CHECK_CLOSE(col[0], 42.0, 1e-10);
    PASS();
}

// ──── Test 6: fromMemory — null column names → auto-named ────
void test_fromMemory_default_names() {
    TEST("fromMemory null colNames → auto \"col0\", \"col1\", ...");
    const double data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0};
    auto table = xyplot::DataTable::fromMemory(data, 3, 2, nullptr);
    CHECK(table.columnName(0) == "col0");
    CHECK(table.columnName(1) == "col1");
    PASS();
}

// ──── Test 7: column() — out of bounds returns nullptr ────
void test_column_oob() {
    TEST("column() out-of-bounds → nullptr");
    const double data[] = {1.0, 2.0, 3.0};
    auto table = xyplot::DataTable::fromMemory(data, 3, 1);
    CHECK(table.column(-1) == nullptr);
    CHECK(table.column(1) == nullptr);
    CHECK(table.column(100) == nullptr);
    PASS();
}

// ──── Test 8: column(name) — found and not found ────
void test_column_by_name() {
    TEST("column(const char* name)");
    const double data[] = {1.0, 2.0, 3.0, 4.0};
    const char* names[] = {"apples", "oranges"};
    auto table = xyplot::DataTable::fromMemory(data, 2, 2, names);

    const double* c1 = table.column("apples");
    CHECK(c1 != nullptr);
    CHECK_CLOSE(c1[0], 1.0, 1e-10);

    const double* c2 = table.column("oranges");
    CHECK(c2 != nullptr);
    CHECK_CLOSE(c2[0], 2.0, 1e-10);

    // Non-existent name
    CHECK(table.column("bananas") == nullptr);
    CHECK(table.column(nullptr) == nullptr);
    PASS();
}

// ──── Test 9: columnIndex(name) ────
void test_columnIndex() {
    TEST("columnIndex lookup");
    const double data[] = {1.0, 2.0};
    const char* names[] = {"a", "b"};
    auto table = xyplot::DataTable::fromMemory(data, 1, 2, names);

    CHECK(table.columnIndex("a") == 0);
    CHECK(table.columnIndex("b") == 1);
    CHECK(table.columnIndex("c") == -1);
    CHECK(table.columnIndex(nullptr) == -1);
    PASS();
}

// ──── Test 10: columnName() — out-of-bounds returns empty ────
void test_columnName_oob() {
    TEST("columnName() out-of-bounds → empty string");
    const double data[] = {1.0, 2.0};
    auto table = xyplot::DataTable::fromMemory(data, 1, 1);
    CHECK(table.columnName(-1).empty());
    CHECK(table.columnName(1).empty());
    CHECK(table.columnName(100).empty());
    PASS();
}

// ──── Test 11: rowCount / colCount — matches input ────
void test_counts_consistency() {
    TEST("rowCount/colCount consistency");
    const double data[] = {1.0, 2.0, 3.0, 4.0, 5.0, 6.0, 7.0, 8.0, 9.0, 10.0,
                           11.0, 12.0, 13.0, 14.0, 15.0, 16.0, 17.0, 18.0, 19.0, 20.0};
    auto table = xyplot::DataTable::fromMemory(data, 5, 4, nullptr);
    CHECK(table.rowCount() == 5);
    CHECK(table.colCount() == 4);
    // rowCount derived from first column
    for (int c = 0; c < table.colCount(); c++) {
        const double* col = table.column(c);
        CHECK(col != nullptr);
        // All columns should have same row count
        for (int r = 0; r < table.rowCount(); r++) {
            CHECK(std::isfinite(col[r]));
        }
    }
    PASS();
}

// ──── Test 12: column() — pointer stability ────
void test_column_pointer_stability() {
    TEST("column() pointer stability — same index twice → same pointer");
    const double data[] = {1.0, 2.0, 3.0, 4.0};
    auto table = xyplot::DataTable::fromMemory(data, 2, 2);
    const double* p1 = table.column(0);
    const double* p2 = table.column(0);
    CHECK(p1 == p2);  // must be exactly same pointer
    PASS();
}

// ──── Test 13: empty table defaults ────
void test_default_constructed() {
    TEST("default constructed table is empty");
    xyplot::DataTable table;
    CHECK(table.rowCount() == 0);
    CHECK(table.colCount() == 0);
    CHECK(table.column(0) == nullptr);
    CHECK(table.columnName(0).empty());
    CHECK(table.columnIndex("anything") == -1);
    PASS();
}

// ──── Test 14: fromCSV — normal file ────
void test_fromCSV_normal() {
    TEST("fromCSV normal file with header + 3 data rows");
    std::string csv =
        "time,value,error\n"
        "0.0,1.0,0.1\n"
        "1.0,2.0,0.2\n"
        "2.0,3.0,0.3\n";
    std::string path = writeTempCSV(csv);

    auto table = xyplot::DataTable::fromCSV(path.c_str(), true);
    CHECK(table.rowCount() == 3);
    CHECK(table.colCount() == 3);
    CHECK(table.columnName(0) == "time");
    CHECK(table.columnName(1) == "value");
    CHECK(table.columnName(2) == "error");

    const double* col0 = table.column(0);
    CHECK_CLOSE(col0[0], 0.0, 1e-10);
    CHECK_CLOSE(col0[1], 1.0, 1e-10);
    CHECK_CLOSE(col0[2], 2.0, 1e-10);

    const double* col1 = table.column(1);
    CHECK_CLOSE(col1[0], 1.0, 1e-10);
    CHECK_CLOSE(col1[1], 2.0, 1e-10);
    CHECK_CLOSE(col1[2], 3.0, 1e-10);

    std::remove(path.c_str());
    PASS();
}

// ──── Test 15: fromCSV — no header ────
void test_fromCSV_no_header() {
    TEST("fromCSV no header → auto-named columns");
    std::string csv =
        "1.0,2.0\n"
        "3.0,4.0\n"
        "5.0,6.0\n";
    std::string path = writeTempCSV(csv);

    auto table = xyplot::DataTable::fromCSV(path.c_str(), false);
    CHECK(table.rowCount() == 3);
    CHECK(table.colCount() == 2);
    CHECK(table.columnName(0) == "col0");
    CHECK(table.columnName(1) == "col1");

    const double* col1 = table.column(1);
    CHECK_CLOSE(col1[0], 2.0, 1e-10);
    CHECK_CLOSE(col1[1], 4.0, 1e-10);
    CHECK_CLOSE(col1[2], 6.0, 1e-10);

    std::remove(path.c_str());
    PASS();
}

// ──── Test 16: fromCSV — empty file ────
void test_fromCSV_empty_file() {
    TEST("fromCSV empty file → empty table");
    std::string path = writeTempCSV("");
    auto table = xyplot::DataTable::fromCSV(path.c_str());
    CHECK(table.rowCount() == 0);
    CHECK(table.colCount() == 0);
    std::remove(path.c_str());
    PASS();
}

// ──── Test 17: fromCSV — header only, no data rows ────
void test_fromCSV_header_only() {
    TEST("fromCSV header only → 0 rows, N cols");
    std::string csv = "colA,colB,colC\n";
    std::string path = writeTempCSV(csv);

    auto table = xyplot::DataTable::fromCSV(path.c_str(), true);
    CHECK(table.rowCount() == 0);  // no data rows → 0
    CHECK(table.colCount() == 3);  // but header parsed → 3 cols
    CHECK(table.columnName(0) == "colA");
    CHECK(table.columnName(1) == "colB");
    CHECK(table.columnName(2) == "colC");

    std::remove(path.c_str());
    PASS();
}

// ──── Test 18: fromCSV — file with empty lines ────
void test_fromCSV_with_empty_lines() {
    TEST("fromCSV with empty lines → skipped, data intact");
    std::string csv =
        "x,y\n"
        "\n"
        "1.0,10.0\n"
        "\n"
        "2.0,20.0\n"
        "\n";
    std::string path = writeTempCSV(csv);

    auto table = xyplot::DataTable::fromCSV(path.c_str(), true);
    CHECK(table.rowCount() == 2);     // empty lines skipped
    CHECK(table.colCount() == 2);
    CHECK_CLOSE(table.column(0)[0], 1.0, 1e-10);
    CHECK_CLOSE(table.column(0)[1], 2.0, 1e-10);

    std::remove(path.c_str());
    PASS();
}

// ──── Test 19: fromCSV — file doesn't exist ────
void test_fromCSV_nonexistent() {
    TEST("fromCSV nonexistent file → empty table");
    auto table = xyplot::DataTable::fromCSV("/tmp/no_such_file_xyplot_test.csv");
    CHECK(table.rowCount() == 0);
    CHECK(table.colCount() == 0);
    PASS();
}

// ──── Test 20: fromCSV — negative and decimal values ────
void test_fromCSV_negative_values() {
    TEST("fromCSV negative and decimal values");
    std::string csv =
        "x,y\n"
        "-5.5,0.0\n"
        "0.0,-3.14\n"
        "10.0,99.9\n";
    std::string path = writeTempCSV(csv);

    auto table = xyplot::DataTable::fromCSV(path.c_str());
    CHECK(table.rowCount() == 3);
    const double* x = table.column("x");
    const double* y = table.column("y");
    CHECK_CLOSE(x[0], -5.5, 1e-10);
    CHECK_CLOSE(y[0], 0.0, 1e-10);
    CHECK_CLOSE(x[1], 0.0, 1e-10);
    CHECK_CLOSE(y[1], -3.14, 1e-10);
    CHECK_CLOSE(x[2], 10.0, 1e-10);
    CHECK_CLOSE(y[2], 99.9, 1e-10);

    std::remove(path.c_str());
    PASS();
}

// ──── Test 21: fromCSV — large dataset performance ────
void test_fromMemory_large() {
    TEST("fromMemory large dataset (100k rows)");
    const int rows = 100000;
    const int cols = 2;
    std::vector<double> data(rows * cols);
    for (int i = 0; i < rows; i++) {
        data[i * cols + 0] = static_cast<double>(i) * 0.01;
        data[i * cols + 1] = static_cast<double>(i) * 0.01 + 1.0;
    }

    auto table = xyplot::DataTable::fromMemory(data.data(), rows, cols);
    CHECK(table.rowCount() == rows);
    CHECK(table.colCount() == cols);
    // Spot check
    CHECK_CLOSE(table.column(0)[0], 0.0, 1e-10);
    CHECK_CLOSE(table.column(0)[rows - 1], (rows - 1) * 0.01, 1e-10);
    CHECK_CLOSE(table.column(1)[rows / 2], (rows / 2) * 0.01 + 1.0, 1e-10);
    PASS();
}

// ──── Test 22: fromCSV — single column ────
void test_fromCSV_single_column() {
    TEST("fromCSV single column");
    std::string csv =
        "value\n"
        "1.0\n"
        "2.0\n"
        "3.0\n";
    std::string path = writeTempCSV(csv);

    auto table = xyplot::DataTable::fromCSV(path.c_str());
    CHECK(table.rowCount() == 3);
    CHECK(table.colCount() == 1);
    CHECK(table.columnName(0) == "value");
    CHECK_CLOSE(table.column(0)[0], 1.0, 1e-10);
    CHECK_CLOSE(table.column(0)[2], 3.0, 1e-10);

    std::remove(path.c_str());
    PASS();
}

// ──── Main ────
int main() {
    printf("=== DataTable Unit Tests (Phase C — Quality Deepening) ===\n\n");

    printf("fromMemory:\n");
    test_fromMemory_normal();
    test_fromMemory_null_data();
    test_fromMemory_zero_rows();
    test_fromMemory_zero_cols();
    test_fromMemory_1x1();
    test_fromMemory_default_names();
    test_fromMemory_large();

    printf("\nColumn access:\n");
    test_column_oob();
    test_column_by_name();
    test_columnIndex();
    test_columnName_oob();
    test_counts_consistency();
    test_column_pointer_stability();

    printf("\nDefault / empty:\n");
    test_default_constructed();

    printf("\nfromCSV:\n");
    test_fromCSV_normal();
    test_fromCSV_no_header();
    test_fromCSV_empty_file();
    test_fromCSV_header_only();
    test_fromCSV_with_empty_lines();
    test_fromCSV_nonexistent();
    test_fromCSV_negative_values();
    test_fromCSV_single_column();

    printf("\n──────────────────────────\n");
    printf("  %d passed, %d failed\n", g_testsPassed, g_testsFailed);
    printf("──────────────────────────\n");

    return g_testsFailed > 0 ? 1 : 0;
}
