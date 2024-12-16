#include "table_words_to_paragraphs.h"

const char* DDL_CREATE_TABLE_IF_NOT_EXISTS_WORDS_TO_PARAGRAPHS = R"(
CREATE TABLE IF NOT EXISTS WordsToParagraphs (
    word_id INTEGER NOT NULL,
    paragraph_id INTEGER NOT NULL,
    word_position INTEGER NOT NULL,
    PRIMARY KEY (word_id, paragraph_id),
    FOREIGN KEY(word_id) REFERENCES Words(id) ON DELETE CASCADE,  -- Cascade delete if word is deleted
    FOREIGN KEY(paragraph_id) REFERENCES Paragraphs(id) ON DELETE CASCADE  -- Cascade delete if paragraph is deleted
);

CREATE INDEX IF NOT EXISTS idx_word_id ON WordsToParagraphs(word_id);
CREATE INDEX IF NOT EXISTS idx_paragraph_id ON WordsToParagraphs(paragraph_id);
CREATE INDEX IF NOT EXISTS idx_word_and_paragraph_id ON WordsToParagraphs(word_id, paragraph_id);
)";

const char* DMC_ANALYZE_WORDS_TO_PARAGRAPHS = R"(
    ANALYZE WordsToParagraphs;
)";

const char* DML_INSERT_WORD_TO_PARAGRAPH = R"(
    INSERT INTO WordsToParagraphs (word_id, paragraph_id, word_position) VALUES (?, ?, ?);
)";

const char* DML_SELECT_COMPOUND_QUERY_1 = R"(
    SELECT subquery.paragraph_id, subquery.word_id 
    FROM WordsToParagraphs subquery 
    WHERE subquery.paragraph_id IN (
        SELECT DISTINCT paragraph_id 
        FROM WordsToParagraphs 
        WHERE word_id = ?
    ) 
    ORDER BY subquery.paragraph_id, subquery.word_position;
)";

/*
* Note that these use the IN clause instead of the EQUALS clause, compared to DML_SELECT_COMPOUND_QUERY_1
*/
const char* DML_SELECT_COMPOUND_QUERY_1_PART_1 = R"( 
    SELECT subquery.paragraph_id, subquery.word_id 
    FROM WordsToParagraphs subquery 
    WHERE subquery.paragraph_id IN (
        SELECT DISTINCT paragraph_id 
        FROM WordsToParagraphs 
        WHERE word_id IN ()";

const char* DML_SELECT_COMPOUND_QUERY_1_PART_2 = R"(
    )
    ) 
    ORDER BY subquery.paragraph_id, subquery.word_position;
)";


