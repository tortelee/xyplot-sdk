// ============================================================
// datatable.cpp — DataTable implementation
// ============================================================
#include "datatable.h"
#include <fstream>
#include <sstream>
#include <algorithm>
#include <cstring>
#include <cerrno>
#include <cstdlib>

namespace xyplot {

// ──── Helpers ────

namespace {

// Fast CSV line split. Handles quoted fields minimally.
std::vector<std::string> splitCSVLine(const std::string& line) {
    std::vector<std::string> fields;
    std::string field;
    bool inQuotes = false;

    for (size_t i = 0; i < line.size(); ++i) {
        char c = line[i];
        if (c == '"') {
            inQuotes = !inQuotes;
        } else if (c == ',' && !inQuotes) {
            fields.push_back(field);
            field.clear();
        } else {
            field += c;
        }
    }
    fields.push_back(field);
    return fields;
}

// Trim whitespace
std::string trim(const std::string& s) {
    size_t start = 0;
    while (start < s.size() && (s[start] == ' ' || s[start] == '\t' || s[start] == '\r'))
        ++start;
    size_t end = s.size();
    while (end > start && (s[end - 1] == ' ' || s[end - 1] == '\t' || s[end - 1] == '\r'))
        --end;
    return s.substr(start, end - start);
}

} // anonymous namespace

// ──── Factories ────

DataTable DataTable::fromMemory(const double* data, int rows, int cols,
                                const char* const* colNames) {
    DataTable table;
    if (!data || rows <= 0 || cols <= 0) return table;

    table.m_columns.resize(cols);

    for (int c = 0; c < cols; ++c) {
        auto& col = table.m_columns[c];
        col.resize(rows);
        for (int r = 0; r < rows; ++r) {
            // Column-major: data is stored as [col][row]
            col[r] = data[r * cols + c];
        }
    }

    table.m_names.resize(cols);
    for (int c = 0; c < cols; ++c) {
        if (colNames && colNames[c]) {
            table.m_names[c] = colNames[c];
        } else {
            table.m_names[c] = "col" + std::to_string(c);
        }
    }

    return table;
}

DataTable DataTable::fromCSV(const char* filename, bool hasHeader) {
    DataTable table;
    std::ifstream file(filename);
    if (!file.is_open()) return table;

    std::string line;
    std::vector<std::string> headerNames;
    bool headerRead = false;
    int numCols = 0;

    // First pass: read all rows into string vectors
    std::vector<std::vector<std::string>> rawRows;

    while (std::getline(file, line)) {
        if (line.empty()) continue;
        auto fields = splitCSVLine(line);

        if (hasHeader && !headerRead) {
            numCols = static_cast<int>(fields.size());
            headerNames.reserve(numCols);
            for (auto& f : fields) {
                headerNames.push_back(trim(f));
            }
            headerRead = true;
        } else {
            if (numCols == 0) numCols = static_cast<int>(fields.size());
            // Truncate or keep only first numCols fields
            if (static_cast<int>(fields.size()) > numCols) {
                fields.resize(numCols);
            }
            rawRows.push_back(std::move(fields));
        }
    }

    if (numCols == 0) return table;

    // Handle header-only CSV (has columns but no data rows)
    if (rawRows.empty()) {
        table.m_names.resize(numCols);
        table.m_columns.resize(numCols);  // each column empty (0 rows)
        for (int c = 0; c < numCols; ++c) {
            table.m_names[c] = (c < static_cast<int>(headerNames.size()))
                                   ? headerNames[c]
                                   : "col" + std::to_string(c);
        }
        return table;
    }

    // Second pass: convert to double columns
    int numRows = static_cast<int>(rawRows.size());
    table.m_columns.resize(numCols);
    for (auto& col : table.m_columns) col.resize(numRows);

    for (int c = 0; c < numCols; ++c) {
        for (int r = 0; r < numRows; ++r) {
            double val = 0.0;
            if (c < static_cast<int>(rawRows[r].size())) {
                const auto& field = rawRows[r][c];
                char* end = nullptr;
                val = std::strtod(field.c_str(), &end);
                if (end == field.c_str()) val = 0.0; // parse failure → 0
            }
            table.m_columns[c][r] = val;
        }
    }

    // Set column names
    table.m_names.resize(numCols);
    for (int c = 0; c < numCols; ++c) {
        if (c < static_cast<int>(headerNames.size())) {
            table.m_names[c] = headerNames[c];
        } else {
            table.m_names[c] = "col" + std::to_string(c);
        }
    }

    return table;
}

// ──── Column access ────

const double* DataTable::column(int index) const {
    if (index < 0 || index >= static_cast<int>(m_columns.size())) return nullptr;
    return m_columns[index].data();
}

double* DataTable::column(int index) {
    if (index < 0 || index >= static_cast<int>(m_columns.size())) return nullptr;
    return m_columns[index].data();
}

const double* DataTable::column(const char* name) const {
    int idx = columnIndex(name);
    return idx >= 0 ? m_columns[idx].data() : nullptr;
}

double* DataTable::column(const char* name) {
    int idx = columnIndex(name);
    return idx >= 0 ? m_columns[idx].data() : nullptr;
}

// ──── Metadata ────

int DataTable::rowCount() const {
    if (m_columns.empty()) return 0;
    return static_cast<int>(m_columns[0].size());
}

int DataTable::colCount() const {
    return static_cast<int>(m_columns.size());
}

const std::string& DataTable::columnName(int index) const {
    static const std::string empty;
    if (index < 0 || index >= static_cast<int>(m_names.size())) return empty;
    return m_names[index];
}

int DataTable::columnIndex(const char* name) const {
    if (!name) return -1;
    for (size_t i = 0; i < m_names.size(); ++i) {
        if (m_names[i] == name) return static_cast<int>(i);
    }
    return -1;
}

// ──── Mutation ────

int DataTable::addColumn(const char* name, const double* data, int rows) {
    if (!data || rows <= 0) return -1;

    int existingRows = rowCount();
    if (existingRows > 0 && rows != existingRows) return -1; // row count mismatch

    int idx = static_cast<int>(m_columns.size());
    m_names.push_back(name ? name : "col" + std::to_string(idx));
    m_columns.emplace_back(data, data + rows);
    return idx;
}

void DataTable::reserveRows(int n) {
    for (auto& col : m_columns) col.reserve(n);
}

} // namespace xyplot
