#pragma once

static constexpr char DDL_CREATE_TABLE_IF_NOT_EXISTS_PARAGRAPHS[154] = R"(
CREATE TABLE IF NOT EXISTS Paragraphs (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    original_text TEXT NOT NULL,
    normalized_text TEXT NOT NULL
);
)";

static constexpr char DMC_ANALYZE_PARAGRAPHS[26] = R"(
    ANALYZE Paragraphs;
)";

static constexpr char DML_INSERT_PARAGRAPH[77] = R"(
    INSERT INTO Paragraphs (original_text, normalized_text) VALUES (?, ?);
)";