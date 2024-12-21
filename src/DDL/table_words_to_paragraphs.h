#pragma once

static constexpr char DDL_CREATE_TABLE_IF_NOT_EXISTS_WORDS_TO_PARAGRAPHS[484] = R"(
CREATE TABLE IF NOT EXISTS WordsToParagraphs (
    word_id INTEGER NOT NULL,
    paragraph_id INTEGER NOT NULL,
    word_position INTEGER NOT NULL,
    PRIMARY KEY (word_id, paragraph_id),
    FOREIGN KEY(word_id) REFERENCES Words(id) ON DELETE CASCADE,
    FOREIGN KEY(paragraph_id) REFERENCES Paragraphs(id) ON DELETE CASCADE
);

CREATE INDEX IF NOT EXISTS idx_word_id ON WordsToParagraphs(word_id);
CREATE INDEX IF NOT EXISTS idx_paragraph_id ON WordsToParagraphs(paragraph_id);
)";

static constexpr char DMC_ANALYZE_WORDS_TO_PARAGRAPHS[33] = R"(
    ANALYZE WordsToParagraphs;
)";

static constexpr char DML_INSERT_WORD_TO_PARAGRAPH[93] = R"(
    INSERT INTO WordsToParagraphs (word_id, paragraph_id, word_position) VALUES (?, ?, ?);
)";

static constexpr char DML_SELECT_COMPOUND_1_EQUALS[1359] = R"(
    WITH selected_word_id AS (
        SELECT id FROM Words WHERE word = ?
    ), 
    paragraphs_data AS (
        SELECT 
            subquery.paragraph_id, 
            paragraphs.original_text, 
            selected_word_id.id AS matched_word_id
        FROM WordsToParagraphs subquery

        JOIN Paragraphs paragraphs 
            ON subquery.paragraph_id = paragraphs.id

        JOIN selected_word_id
            ON subquery.word_id = selected_word_id.id

        WHERE subquery.paragraph_id IN (
            SELECT DISTINCT paragraph_id
            FROM WordsToParagraphs
            WHERE word_id = matched_word_id           -- using the equals clause (instead of IN) returns only the row that matches the selected_word_id. so not all of the rows we need are returned.
        )
    )
    SELECT                                            -- so we join again. In testing, this query is ~1000 microseconds faster on short queries (1 or 2 letters, when beginning to type a word) than some shorter, seemingly simpler queries that I tried.
        paragraphs_data.paragraph_id,
        paragraphs_data.original_text,
        paragraphs_data.matched_word_id,
        wtp.word_id
    FROM WordsToParagraphs wtp
    
    JOIN paragraphs_data
        ON wtp.paragraph_id = paragraphs_data.paragraph_id

    ORDER BY wtp.paragraph_id, wtp.word_position;
)";

static constexpr char DML_SELECT_COMPOUND_1_LIKE[1381] = R"(
    WITH selected_word_ids AS (
        SELECT id FROM Words WHERE word != ? AND word LIKE ?
    ), 
    paragraphs_data AS (
        SELECT 
            subquery.paragraph_id, 
            paragraphs.original_text, 
            selected_word_ids.id AS matched_word_id
        FROM WordsToParagraphs subquery

        JOIN Paragraphs paragraphs 
            ON subquery.paragraph_id = paragraphs.id

        JOIN selected_word_ids
            ON subquery.word_id = selected_word_ids.id

        WHERE subquery.paragraph_id IN (
            SELECT DISTINCT paragraph_id
            FROM WordsToParagraphs
            WHERE word_id = matched_word_id           -- using the equals clause (instead of IN) returns only the row that matches the selected_word_ids. so not all of the rows we need are returned.
        )
    )
    SELECT                                            -- so we join again. In testing, this query is ~1000 microseconds faster on short queries (1 or 2 letters, when beginning to type a word) than some shorter, seemingly simpler queries that I tried.
        paragraphs_data.paragraph_id,
        paragraphs_data.original_text,
        paragraphs_data.matched_word_id,
        wtp.word_id
    FROM WordsToParagraphs wtp
    
    JOIN paragraphs_data
        ON wtp.paragraph_id = paragraphs_data.paragraph_id

    ORDER BY wtp.paragraph_id, wtp.word_position;
)";

// static constexpr char DML_SELECT_COMPOUND_1[1679] = R"(
//     WITH selected_word_ids AS (
//         SELECT 
//             id,
//             CASE
//                 WHEN word = ? THEN TRUE
//                 WHEN word LIKE ? THEN FALSE
//             END AS is_exact_match
//         FROM Words
//         WHERE word LIKE ?
//     ),  
//     paragraphs_data AS (
//         SELECT 
//             subquery.paragraph_id, 
//             paragraphs.original_text, 
//             selected_word_ids.id AS matched_word_id, 
//             selected_word_ids.is_exact_match AS matched_word_is_exact_match
//         FROM WordsToParagraphs subquery

//         JOIN Paragraphs paragraphs 
//             ON subquery.paragraph_id = paragraphs.id

//         JOIN selected_word_ids
//             ON subquery.word_id = selected_word_ids.id

//         WHERE subquery.paragraph_id IN (
//             SELECT DISTINCT paragraph_id
//             FROM WordsToParagraphs
//             WHERE word_id = matched_word_id             -- using the equals clause (instead of IN) returns only the row that matches the selected_word_ids. so not all of the rows we need are returned.
//         )
//     )
//     SELECT                                              -- so we join again. In testing, this query is ~1000 microseconds faster on short queries (1 or 2 letters, when beginning to type a word) than some shorter, seemingly simpler query strategies that I tried.
//         paragraphs_data.paragraph_id,
//         paragraphs_data.original_text,
//         paragraphs_data.matched_word_id,
//         wtp.word_id, 
//         paragraphs_data.matched_word_is_exact_match 
//     FROM WordsToParagraphs wtp
    
//     JOIN paragraphs_data
//         ON wtp.paragraph_id = paragraphs_data.paragraph_id

//     ORDER BY wtp.paragraph_id, wtp.word_position;
// )";
