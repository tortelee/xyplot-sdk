// ============================================================
// datatable.h — DataTable: column-major storage, CSV parsing
// ============================================================
// Owner: Agent C (核心算法)
// Consumers: Agent D (图类型), Agent E (Plot 门面)
// Depends on: types.h (frozen)
// ============================================================
#pragma once
#include "xyplot/types.h"
#include <string>
#include <vector>
#include <cstddef>

namespace xyplot {

// ──── DataTable ────
// Column-major storage for efficient column-wise operations.
// Each column is a contiguous std::vector<double>.
// Columns are named for ergonomic access.
//
// Memory: O(rows × cols) with minimal overhead.
// Copy: value semantics — copy is deep.
class DataTable {
public:
    DataTable() = default;

    // ──── Factories ────

    /// Create from raw in-memory arrays.
    /// colNames may be nullptr (columns will be named "col0", "col1", …).
    static DataTable fromMemory(const double* data, int rows, int cols,
                                const char* const* colNames = nullptr);

    /// Load from CSV file. First line treated as header if `hasHeader` is true.
    /// Returns empty table on parse error (check rowCount() == 0).
    static DataTable fromCSV(const char* filename, bool hasHeader = true);

    // ──── Column access ────

    /// Returns pointer to column data (rows contiguous doubles), or nullptr.
    const double* column(int index) const;
    double*       column(int index);

    /// Returns pointer to column data by name, or nullptr if not found.
    const double* column(const char* name) const;
    double*       column(const char* name);

    // ──── Metadata ────

    int                rowCount()    const;
    int                colCount()    const;
    const std::string& columnName(int index) const;

    /// Returns column index by name, or -1 if not found.
    int columnIndex(const char* name) const;

    // ──── Mutation ────

    /// Append a column. Returns its index.
    int addColumn(const char* name, const double* data, int rows);

    /// Reserve rows for building. After reserveRows, use column(index)[row]=val.
    void reserveRows(int n);

private:
    std::vector<std::string>      m_names;
    std::vector<std::vector<double>> m_columns;  // column-major: [col][row]
};

} // namespace xyplot
