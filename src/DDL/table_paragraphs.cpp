#include "table_paragraphs.h"

const char* DDL_CREATE_TABLE_IF_NOT_EXISTS_PARAGRAPHS = R"(
CREATE TABLE IF NOT EXISTS Paragraphs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    original_text TEXT NOT NULL,
    normalized_text TEXT NOT NULL
);

CREATE INDEX IF NOT EXISTS idx_normalized_text ON Paragraphs(normalized_text);
)";

const char* DMC_ANALYZE_PARAGRAPHS = R"(
    ANALYZE Paragraphs;
)";

const char* DML_INSERT_PARAGRAPH = R"(
    INSERT INTO Paragraphs (original_text, normalized_text) VALUES (?, ?);
)";

const char* DML_SELECT_ORIGINAL_TEXT_FROM_PARAGRAPHS_WHERE_ID_EQUALS = R"(
    SELECT original_text FROM Paragraphs WHERE id = ?;
)";

const char* DML_SELECT_ORIGINAL_TEXT_FROM_PARAGRAPHS_WHERE_ID_EQUALS_PART_1 = "SELECT original_text FROM Paragraphs WHERE id IN (";

const char* DML_SELECT_ORIGINAL_TEXT_FROM_PARAGRAPHS_WHERE_ID_EQUALS_PART_2 = ");";