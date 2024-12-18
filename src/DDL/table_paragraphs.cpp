#include "table_paragraphs.h"

const char* DDL_CREATE_TABLE_IF_NOT_EXISTS_PARAGRAPHS = R"(
CREATE TABLE IF NOT EXISTS Paragraphs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    original_text TEXT NOT NULL,
    normalized_text TEXT NOT NULL
);
)";

const char* DMC_ANALYZE_PARAGRAPHS = R"(
    ANALYZE Paragraphs;
)";

const char* DML_INSERT_PARAGRAPH = R"(
    INSERT INTO Paragraphs (original_text, normalized_text) VALUES (?, ?);
)";