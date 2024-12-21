#pragma once

static constexpr char DDL_CREATE_TABLE_IF_NOT_EXISTS_WORDS[166] = R"(
CREATE TABLE IF NOT EXISTS Words (
    id INTEGER PRIMARY KEY AUTOINCREMENT,
    word TEXT NOT NULL UNIQUE
);

CREATE INDEX IF NOT EXISTS idx_words ON Words(word);
)";

static constexpr char DMC_ANALYZE_WORDS[21] = R"(
    ANALYZE Words;
)";

static constexpr char DML_INSERT_WORD[43] = R"(
    INSERT INTO Words (word) VALUES (?);
)";

static constexpr char DML_EXPLAIN_QUERY_PLAN_DML_SELECT_ID_FROM_WORDS_WHERE_WORD_EQUALS[62] = R"(
    EXPLAIN QUERY PLAN SELECT id FROM Words WHERE word = ?;
)";

static constexpr char DML_SELECT_ID_FROM_WORDS_WHERE_WORD_EQUALS[43] = R"(
    SELECT id FROM Words WHERE word = ?;
)";

static constexpr char DML_EXPLAIN_QUERY_PLAN_DML_SELECT_ID_FROM_WORDS_WHERE_WORD_LIKE[65] = R"(
    EXPLAIN QUERY PLAN SELECT id FROM Words WHERE word LIKE ?;
)";

static constexpr char DML_SELECT_ID_FROM_WORDS_WHERE_WORD_LIKE[46] = R"(
    SELECT id FROM Words WHERE word LIKE ?;
)";
