#include <iostream>

#ifdef DATABASE_LOG_EXECUTION_TIMES
#include <chrono>
#endif

#include "database.h"

#include "../extern/yaml-cpp/include/yaml-cpp/yaml.h"

#include "Structures/NormalizedText.h"

#include "DDL/table_words.h"
#include "DDL/table_paragraphs.h"
#include "DDL/table_words_to_paragraphs.h"

Database* Database::Instance = nullptr;
sqlite3* Database::db = nullptr;
int Database::rc = 0;
char* Database::errMsg = nullptr;
bool Database::bIsValid = true;


Database::Database()
{
    /*
    * Database will be opened if it already exists, or created.
    * Databased is used in NOMUTEX mode, which means seperate threads can access 
    * the database at the same time IF they use different database connections.
    * 
    * Note: NOMUTEX is not ideal for all use cases.
    */
    rc = sqlite3_open_v2(databaseFilepath, &db, SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_NOMUTEX, nullptr);
    if (rc) 
    {
        cerr << "Err: " << rc << " Can't open database: " << sqlite3_errmsg(db) << endl;
        bIsValid = false;
        return;
    }

    /*
    * Execute DDL Statements
    */
    rc = sqlite3_exec(db, DDL_CREATE_TABLE_IF_NOT_EXISTS_WORDS, 0, 0, &errMsg);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
        bIsValid = false;
        return;
    }

    rc = sqlite3_exec(db, DDL_CREATE_TABLE_IF_NOT_EXISTS_PARAGRAPHS, 0, 0, &errMsg);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
        bIsValid = false;
        return;
    }

    rc = sqlite3_exec(db, DDL_CREATE_TABLE_IF_NOT_EXISTS_WORDS_TO_PARAGRAPHS, 0, 0, &errMsg);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " SQL error: " << errMsg << endl;
        sqlite3_free(errMsg);
        bIsValid = false;
        return;
    }

#ifdef LOAD_TEST_DATA
    /*
    * Load Test Data
    */
    YAML::Node yamlTestData = YAML::LoadFile(filesystem::absolute(testDataFilepath).c_str());
    if (yamlTestData.IsNull()) 
    {                        
        cerr << "Err: " << rc << " Unable to open config/database_test_data.yml." << endl;
        bIsValid = false;
        return;
    }

    vector<NormalizedText> buffer;

    for (const auto& data : yamlTestData)
    {
        buffer.push_back(NormalizedText(data.as<string>(), MAX_PARAGRAPH_SIZE));
    }

#ifdef DATABASE_LOG_EXECUTION_TIMES
    chrono::_V2::system_clock::time_point t0  = chrono::_V2::system_clock::time_point();
    chrono::_V2::system_clock::time_point t1  = chrono::_V2::system_clock::time_point();

    t0  = chrono::high_resolution_clock::now();
#endif

    
    /*
    * Insert to all tables - Begin Transaction
    */
    const string LoadTestDataTransactionName = "Load Test Data";

    sqlite3_stmt* stmt_insert_paragraph = nullptr;
    sqlite3_stmt* stmt_insert_word = nullptr;
    sqlite3_stmt* stmt_select_word = nullptr;
    sqlite3_stmt* stmt_insert_words_to_paragraphs = nullptr;

    auto CommitTransaction = [&]() -> bool
    {
        sqlite3_finalize(stmt_insert_paragraph);
        sqlite3_finalize(stmt_insert_paragraph);
        sqlite3_finalize(stmt_insert_paragraph);
        sqlite3_finalize(stmt_insert_paragraph);
        EndTransaction(LoadTestDataTransactionName, true);
        return true;
    };

    if (!BeginTransaction(LoadTestDataTransactionName, true))
    {
        return;
    }

    rc = sqlite3_prepare_v3(db, DML_INSERT_PARAGRAPH, -1, SQLITE_PREPARE_PERSISTENT, &stmt_insert_paragraph, 0);
    if (rc != SQLITE_OK) 
    {
        FailTransaction(LoadTestDataTransactionName, "Failed to prepare insert statement for paragraphs", true, CommitTransaction);
        return;
    }

    rc = sqlite3_prepare_v3(db, DML_INSERT_WORD, -1, SQLITE_PREPARE_PERSISTENT, &stmt_insert_word, 0);
    if (rc != SQLITE_OK) 
    {
        FailTransaction(LoadTestDataTransactionName, "Failed to prepare insert statement for words", true, CommitTransaction);
        return;
    }

    rc = sqlite3_prepare_v3(db, DML_SELECT_ID_FROM_WORDS_WHERE_WORD_EQUALS, -1, SQLITE_PREPARE_PERSISTENT, &stmt_select_word, 0);
    if (rc != SQLITE_OK) 
    {
        FailTransaction(LoadTestDataTransactionName, "Failed to prepare select statement for words", true, CommitTransaction);
        return;
    }

    rc = sqlite3_prepare_v3(db, DML_INSERT_WORD_TO_PARAGRAPH, -1, SQLITE_PREPARE_PERSISTENT, &stmt_insert_words_to_paragraphs, 0);
    if (rc != SQLITE_OK) 
    {
        FailTransaction(LoadTestDataTransactionName, "Failed to prepare select statement for words to paragraphs", true, CommitTransaction);
        return;
    }

    int paragraph_id;
    int word_id;

    for (const NormalizedText& normalized_text : buffer) 
    {
        /*
        * Paragraphs Table - Begin Insert
        */
        paragraph_id = -1;

        rc = sqlite3_bind_text(stmt_insert_paragraph, 1, normalized_text.original_text.c_str(), -1, SQLITE_STATIC);
        if (rc != SQLITE_OK) 
        {
            FailTransaction(LoadTestDataTransactionName, "Failed to bind text to prepared statement for paragraphs", true, CommitTransaction);
            return;
        }

        sqlite3_bind_text(stmt_insert_paragraph, 2, normalized_text.normalized_text.c_str(), -1, SQLITE_STATIC);
        if (rc != SQLITE_OK) 
        {
            FailTransaction(LoadTestDataTransactionName, "Failed to bind text to prepared statement for paragraphs", true, CommitTransaction);
            return;
        }

        rc = sqlite3_step(stmt_insert_paragraph);
        if (rc != SQLITE_DONE) 
        {
            FailTransaction(LoadTestDataTransactionName, "Failed to insert paragraph", false, nullptr);
        } else
        {
            paragraph_id = static_cast<int>(sqlite3_last_insert_rowid(db));

            for (size_t i = 0; i < normalized_text.normalized_words.size(); ++i)
            {
                /*
                * Words Table - Begin Insert
                */
                const string& word = normalized_text.normalized_words[i];
                word_id = -1;

                rc = sqlite3_bind_text(stmt_insert_word, 1, word.c_str(), -1, SQLITE_STATIC);
                if (rc != SQLITE_OK) 
                {
                    FailTransaction(LoadTestDataTransactionName, "Failed to bind text to prepared statement for words", true, CommitTransaction);
                    return;
                }

                rc = sqlite3_step(stmt_insert_word);
                if (rc == SQLITE_CONSTRAINT) 
                {
                    /*
                    * Words Table - Begin Select
                    */
                    rc = sqlite3_bind_text(stmt_select_word, 1, word.c_str(), -1, SQLITE_STATIC);
                    if (rc != SQLITE_OK) 
                    {
                        FailTransaction(LoadTestDataTransactionName, "Failed to bind text to prepared statement for words", true, CommitTransaction);
                        return;
                    }

                    rc = sqlite3_step(stmt_select_word);
                    if (rc == SQLITE_ROW) 
                    {
                        word_id = sqlite3_column_int64(stmt_select_word, 0);
                    } else if (rc == SQLITE_DONE)
                    {
                        cerr << "Error: No existing word found, but the insert failed due to a constraint." << endl;
                    } else 
                    {
                        FailTransaction(LoadTestDataTransactionName, "Failed to select word", false, nullptr);
                    }

                    rc = sqlite3_reset(stmt_select_word);
                    if (rc != SQLITE_OK) 
                    {
                        FailTransaction(LoadTestDataTransactionName, "Failed to reset prepared statement for words", true, CommitTransaction);
                        return;
                    }
                } else if (rc == SQLITE_DONE || rc == SQLITE_OK) 
                {
                    word_id = static_cast<int>(sqlite3_last_insert_rowid(db));
                } else 
                {
                    cerr << "Err: " << rc << " Failed to insert word: " << sqlite3_errmsg(db) << endl;
                }

                rc = sqlite3_reset(stmt_insert_word);
                if (rc != SQLITE_OK && rc != SQLITE_CONSTRAINT) 
                {
                    FailTransaction(LoadTestDataTransactionName, "Failed to reset prepared statement for words", true, CommitTransaction);
                    return;
                }

                if (paragraph_id == -1 || word_id == -1)
                {
                    cerr << "Err: " << rc << " Failed to insert to words to paragraphs due to not having a valid (word_id, paragraph_id): (" << (word_id == -1 ? "missing" : "good") << ", " << (paragraph_id == -1 ? "missing" : "good") << ")" << endl;
                } else
                {
                    /*
                    * Words to Paragraphs Table - Begin Insert
                    */
                    rc = sqlite3_bind_int(stmt_insert_words_to_paragraphs, 1, word_id);
                    if (rc != SQLITE_OK) 
                    {
                        FailTransaction(LoadTestDataTransactionName, "Failed to bind int to prepared statement for words to paragraphs", true, CommitTransaction);
                        return;
                    }

                    rc = sqlite3_bind_int(stmt_insert_words_to_paragraphs, 2, paragraph_id);
                    if (rc != SQLITE_OK) 
                    {
                        FailTransaction(LoadTestDataTransactionName, "Failed to bind int to prepared statement for words to paragraphs", true, CommitTransaction);
                        return;
                    }

                    rc = sqlite3_bind_int(stmt_insert_words_to_paragraphs, 3, i);
                    if (rc != SQLITE_OK) 
                    {
                        FailTransaction(LoadTestDataTransactionName, "Failed to bind int to prepared statement for words to paragraphs", true, CommitTransaction);
                        return;
                    }

                    rc = sqlite3_step(stmt_insert_words_to_paragraphs);
                    if (rc != SQLITE_DONE && rc != SQLITE_CONSTRAINT) 
                    {
                        FailTransaction(LoadTestDataTransactionName, "Failed to insert row to words to paragraphs", false, nullptr);
                    }

                    rc = sqlite3_reset(stmt_insert_words_to_paragraphs);
                    if (rc != SQLITE_OK && rc != SQLITE_CONSTRAINT) 
                    {
                        FailTransaction(LoadTestDataTransactionName, "Failed to reset prepared statement for words to paragraphs", true, CommitTransaction);
                        return;
                    }
                }
            }
        }

        rc = sqlite3_reset(stmt_insert_paragraph);
        if (rc != SQLITE_OK) 
        {
            FailTransaction(LoadTestDataTransactionName, "Failed to reset prepared statement for paragraphs", true, CommitTransaction);
            return;
        }
    }

    rc = sqlite3_finalize(stmt_insert_words_to_paragraphs);
    if (rc != SQLITE_OK) 
    {
        FailTransaction(LoadTestDataTransactionName, "Failed to finalize prepared statement for words", true, CommitTransaction);
        return;
    }

    rc = sqlite3_finalize(stmt_select_word);
    if (rc != SQLITE_OK) 
    {
        FailTransaction(LoadTestDataTransactionName, "Failed to finalize prepared statement for words", true, CommitTransaction);
        return;
    }

    rc = sqlite3_finalize(stmt_insert_word);
    if (rc != SQLITE_OK) 
    {
        FailTransaction(LoadTestDataTransactionName, "Failed to finalize prepared statement for words", true, CommitTransaction);
        return;
    }

    rc = sqlite3_finalize(stmt_insert_paragraph);
    if (rc != SQLITE_OK) 
    {
        FailTransaction(LoadTestDataTransactionName, "Failed to finalize prepared statement for paragraphs", true, CommitTransaction);
        return;
    }

    if (!EndTransaction(LoadTestDataTransactionName, true))
    {
        return;
    }

#ifdef DATABASE_LOG_EXECUTION_TIMES
    t1  = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(t1 - t0);
    cout << "Time taken to insert data to the database: " << duration.count() << " microseconds" << endl;
#endif
#endif

    /*
    * Analyze Databases to Improve Query Performance
    */
#ifdef ANALYZE_AFTER_LOAD
#ifdef DATABASE_LOG_EXECUTION_TIMES
    t0  = chrono::high_resolution_clock::now();
#endif

    sqlite3_stmt* stmt_analyze_paragraphs;
    sqlite3_stmt* stmt_analyze_words;
    sqlite3_stmt* stmt_analyze_words_to_paragraphs;

    rc = sqlite3_prepare_v2(db, DMC_ANALYZE_PARAGRAPHS, -1, &stmt_analyze_paragraphs, 0);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " Failed to prepare analyze statement for paragraphs: " << sqlite3_errmsg(db) << endl;
    } else
    {
        rc = sqlite3_step(stmt_analyze_paragraphs);
        if (rc != SQLITE_DONE) 
        {
            cerr << "Err: " << rc << " Failed to analyze paragraphs: " << sqlite3_errmsg(db) << endl;
        }
        
    }

    rc = sqlite3_prepare_v2(db, DMC_ANALYZE_WORDS, -1, &stmt_analyze_words, 0);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " Failed to prepare analyze statement for words: " << sqlite3_errmsg(db) << endl;
    } else
    {
        rc = sqlite3_step(stmt_analyze_words);
        if (rc != SQLITE_DONE) 
        {
            cerr << "Err: " << rc << " Failed to analyze words: " << sqlite3_errmsg(db) << endl;
        }
    }

    rc = sqlite3_prepare_v2(db, DMC_ANALYZE_WORDS_TO_PARAGRAPHS, -1, &stmt_analyze_words_to_paragraphs, 0);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " Failed to prepare analyze statement for words to paragraphs: " << sqlite3_errmsg(db) << endl;
    } else
    {
        rc = sqlite3_step(stmt_analyze_words_to_paragraphs);
        if (rc != SQLITE_DONE) 
        {
            cerr << "Err: " << rc << " Failed to analyze words to paragraphs: " << sqlite3_errmsg(db) << endl;
        }
    }

    sqlite3_finalize(stmt_analyze_paragraphs);
    sqlite3_finalize(stmt_analyze_words);
    sqlite3_finalize(stmt_analyze_words_to_paragraphs);

#ifdef DATABASE_LOG_EXECUTION_TIMES
    t1  = chrono::high_resolution_clock::now();
    duration = chrono::duration_cast<chrono::microseconds>(t1 - t0);
    cout << "Time taken to analyze data: " << duration.count() << " microseconds" << endl;
#endif
#endif
}

Database* Database::Get() 
{
    if (!Instance)
    {
        Instance = new Database();
        return Instance;
    }
    return Instance;
}

void Database::Destroy() 
{
    rc = sqlite3_close_v2(db);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " Failed to close database: " << sqlite3_errmsg(db) << endl;
    }

    db = nullptr;

    if (Instance)
    {
        delete Instance;
    }
    Instance = nullptr;
}

bool Database::isValid() const
{
    if (bIsValid)
    {
        if (!Instance)
        {
            bIsValid = false;
        } else if(!db)
        {
            bIsValid = false;
        }
    }
    return bIsValid;
}

bool Database::ExplainWordsTableQueryPlan(const string& normalized_word, const TextQueryType Type) 
{
    if (!normalized_word.empty())
    {
        sqlite3_stmt* stmt = nullptr;

        switch (Type)
        {
            case EXACT_MATCH:
            {
                rc = sqlite3_prepare_v2(db, DML_EXPLAIN_QUERY_PLAN_DML_SELECT_ID_FROM_WORDS_WHERE_WORD_EQUALS, -1, &stmt, 0);
                break;
            }
            case BEGINS_WITH:
            {
                rc = sqlite3_prepare_v2(db, DML_EXPLAIN_QUERY_PLAN_DML_SELECT_ID_FROM_WORDS_WHERE_WORD_LIKE, -1, &stmt, 0);
                break;
            }
            case ENDS_WITH:
            {
                rc = sqlite3_prepare_v2(db, DML_EXPLAIN_QUERY_PLAN_DML_SELECT_ID_FROM_WORDS_WHERE_WORD_LIKE, -1, &stmt, 0);
                break;
            }
            case CONTAINS:
            {
                rc = sqlite3_prepare_v2(db, DML_EXPLAIN_QUERY_PLAN_DML_SELECT_ID_FROM_WORDS_WHERE_WORD_LIKE, -1, &stmt, 0);
                break;
            }
        }
        if (rc != SQLITE_OK) 
        {
            cerr << "Err: " << rc << " Failed to prepare explain query plan statement for words: " << sqlite3_errmsg(db) << endl;
            sqlite3_finalize(stmt);
            return false;
        }


        switch (Type)
        {
            case EXACT_MATCH:
            {
                rc = sqlite3_bind_text(stmt, 1, normalized_word.c_str(), -1, SQLITE_STATIC);
                break;
            }
            case BEGINS_WITH:
            {
                rc = sqlite3_bind_text(stmt, 1, (normalized_word + "%").c_str(), -1, SQLITE_STATIC);
                break;
            }
            case ENDS_WITH:
            {
                rc = sqlite3_bind_text(stmt, 1, ("%" + normalized_word).c_str(), -1, SQLITE_STATIC);
                break;
            }
            case CONTAINS:
            {
                rc = sqlite3_bind_text(stmt, 1, ("%" + normalized_word + "%").c_str(), -1, SQLITE_STATIC);
                break;
            }
        }
        if (rc != SQLITE_OK) 
        {
            cerr << "Err: " << rc << " Failed to bind text to the explain query plan statement for words: " << sqlite3_errmsg(db) << endl;
            sqlite3_finalize(stmt);
            return false;
        }

#ifdef DATABASE_LOG_PREPARED_STATEMENTS
        const char* prepared_statement = sqlite3_expanded_sql(stmt);
        cout << prepared_statement << endl;
#endif

        do
        {
            rc = sqlite3_step(stmt);

            if (rc == SQLITE_ROW) 
            {
                // Extract and print the query plan details
                const char* opcode = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 0));  // The first column typically contains the opcode
                const char* detail = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 3)); // The fourth column often contains the detailed explanation
                
                cout << "Opcode: " << opcode << " | Detail: " << detail << endl;
            } else if (rc != SQLITE_DONE) 
            {
                cerr << "Err: " << rc << " Error while explaining query plan for word '" << normalized_word.c_str() << "': " << sqlite3_errmsg(db) << endl;
                break;
            }

        } while (rc == SQLITE_ROW);

        if (rc != SQLITE_DONE) 
        {
            cerr << "Err: " << rc << " Error while explaining query plan for word '" << normalized_word.c_str() << "': " << sqlite3_errmsg(db) << endl;
        }

        sqlite3_finalize(stmt);
        return true;
    }
    return false;
}

vector<int64_t> Database::QueryWordsTableReturnIds(const string& normalized_word, const TextQueryType Type)
{
    vector<int64_t> results = {};

    if (!normalized_word.empty())
    {
#ifdef DATABASE_LOG_EXECUTION_TIMES
        chrono::_V2::system_clock::time_point t0  = chrono::_V2::system_clock::time_point();
        chrono::_V2::system_clock::time_point t1  = chrono::_V2::system_clock::time_point();

        t0  = chrono::high_resolution_clock::now();
#endif
        sqlite3_stmt* stmt = nullptr;

        switch (Type)
        {
            case EXACT_MATCH:
            {
                rc = sqlite3_prepare_v2(db, DML_SELECT_ID_FROM_WORDS_WHERE_WORD_EQUALS, -1, &stmt, 0);
                break;
            }
            case BEGINS_WITH:
            {
                rc = sqlite3_prepare_v2(db, DML_SELECT_ID_FROM_WORDS_WHERE_WORD_LIKE, -1, &stmt, 0);
                break;
            }
            case ENDS_WITH:
            {
                rc = sqlite3_prepare_v2(db, DML_SELECT_ID_FROM_WORDS_WHERE_WORD_LIKE, -1, &stmt, 0);
                break;
            }
            case CONTAINS:
            {
                rc = sqlite3_prepare_v2(db, DML_SELECT_ID_FROM_WORDS_WHERE_WORD_LIKE, -1, &stmt, 0);
                break;
            }
        }
        if (rc != SQLITE_OK) 
        {
            cerr << "Err: " << rc << " Failed to prepare query statement for words: " << sqlite3_errmsg(db) << endl;
            sqlite3_finalize(stmt);
            return results;
        }

        switch (Type)
        {
            case EXACT_MATCH:
            {
                rc = sqlite3_bind_text(stmt, 1, normalized_word.c_str(), -1, SQLITE_STATIC);
                break;
            }
            case BEGINS_WITH:
            {
                rc = sqlite3_bind_text(stmt, 1, (normalized_word + "%").c_str(), -1, SQLITE_STATIC);
                break;
            }
            case ENDS_WITH:
            {
                rc = sqlite3_bind_text(stmt, 1, ("%" + normalized_word).c_str(), -1, SQLITE_STATIC);
                break;
            }
            case CONTAINS:
            {
                rc = sqlite3_bind_text(stmt, 1, ("%" + normalized_word + "%").c_str(), -1, SQLITE_STATIC);
                break;
            }
        }
        if (rc != SQLITE_OK) 
        {
            cerr << "Err: " << rc << " Failed to bind text to the query statement for words: " << sqlite3_errmsg(db) << endl;
            sqlite3_finalize(stmt);
            return results;
        }

#ifdef DATABASE_LOG_PREPARED_STATEMENTS
        const char* prepared_statement = sqlite3_expanded_sql(stmt);
        cout << prepared_statement << endl;
#endif

        do
        {
            rc = sqlite3_step(stmt);

            if (rc == SQLITE_ROW) 
            {
                int64_t id = sqlite3_column_int64(stmt, 0); 
                results.push_back(id);
            } else if (rc != SQLITE_DONE) 
            {
                cerr << "Err: " << rc << " Error while querying word '" << normalized_word.c_str() << "': " << sqlite3_errmsg(db) << endl;
            }

        } while (rc == SQLITE_ROW);

        if (rc != SQLITE_DONE) 
        {
            cerr << "Err: " << rc << " Error while querying word '" << normalized_word.c_str() << "': " << sqlite3_errmsg(db) << endl;
        }

#ifdef DATABASE_LOG_EXECUTION_TIMES
        t1  = chrono::high_resolution_clock::now();
        auto duration = chrono::duration_cast<chrono::microseconds>(t1 - t0);
        cout << "Time taken to query words table: " << duration.count() << " microseconds" << endl;
#endif
#ifdef DATABASE_EXPLAIN_QUERY_PLANS
        ExplainWordsTableQueryPlan(normalized_word, Type);
        const int scanStepsCt = sqlite3_stmt_status(stmt, SQLITE_STMTSTATUS_FULLSCAN_STEP, 0);
        const int sortCt = sqlite3_stmt_status(stmt, SQLITE_STMTSTATUS_SORT, 0);
        const int autoIdxCt = sqlite3_stmt_status(stmt, SQLITE_STMTSTATUS_AUTOINDEX, 0);

        cout << "Scan Steps: " << scanStepsCt << " Sort Count: " << sortCt << " Auto Index Count: " << autoIdxCt << endl;
#endif
        sqlite3_finalize(stmt);
    }
    return results;
}

vector<tuple<int64_t, string, int64_t, vector<int64_t>>> Database::GetAll_ParagraphId_ParagraphOriginalText_MatchedWordId_OrderedWordsInParagraphIds(const string& normalized_word, const TextQueryType Type)
{
    vector<tuple<int64_t, string, int64_t, vector<int64_t>>> results = {};

    if (normalized_word.empty())
    {
        return results;
    }

#ifdef DATABASE_LOG_EXECUTION_TIMES
    chrono::_V2::system_clock::time_point t0  = chrono::_V2::system_clock::time_point();
    chrono::_V2::system_clock::time_point t1  = chrono::_V2::system_clock::time_point();

    t0  = chrono::high_resolution_clock::now();
#endif
    sqlite3_stmt* stmt = nullptr;

    switch (Type)
    {
        case EXACT_MATCH:
        {
            rc = sqlite3_prepare_v2(db, DML_SELECT_COMPOUND_1_EQUALS, -1, &stmt, 0);
            break;
        }
        case BEGINS_WITH:
        {
            rc = sqlite3_prepare_v2(db, DML_SELECT_COMPOUND_1_LIKE, -1, &stmt, 0);
            break;
        }
        case ENDS_WITH:
        {
            rc = sqlite3_prepare_v2(db, DML_SELECT_COMPOUND_1_LIKE, -1, &stmt, 0);
            break;
        }
        case CONTAINS:
        {
            rc = sqlite3_prepare_v2(db, DML_SELECT_COMPOUND_1_LIKE, -1, &stmt, 0);
            break;
        }
    }
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " Failed to prepare query statement for words to paragraphs: " << sqlite3_errmsg(db) << endl;
        sqlite3_finalize(stmt);
        return results;
    }

    rc = sqlite3_bind_text(stmt, 1, normalized_word.c_str(), -1, SQLITE_STATIC);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " Failed to bind text to the query statement for words to paragraphs: " << sqlite3_errmsg(db) << endl;
        sqlite3_finalize(stmt);
        return results;
    }

    switch (Type)
    {
        case EXACT_MATCH:
        {
            break;
        }
        case BEGINS_WITH:
        {
            rc = sqlite3_bind_text(stmt, 2, (normalized_word + "%").c_str(), -1, SQLITE_STATIC);
            break;
        }
        case ENDS_WITH:
        {
            rc = sqlite3_bind_text(stmt, 2, ("%" + normalized_word).c_str(), -1, SQLITE_STATIC);
            break;
        }
        case CONTAINS:
        {
            rc = sqlite3_bind_text(stmt, 2, ("%" + normalized_word + "%").c_str(), -1, SQLITE_STATIC);
            break;
        }
    }
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " Failed to bind text to the query statement for words to paragraphs: " << sqlite3_errmsg(db) << endl;
        sqlite3_finalize(stmt);
        return results;
    }

#ifdef DATABASE_LOG_PREPARED_STATEMENTS
    const char* prepared_statement = sqlite3_expanded_sql(stmt);
    cout << prepared_statement << endl;
#endif

    int64_t paragraph_id = -1;
    string paragraph_text = "";
    int64_t word_id = 0;
    vector<int64_t> word_ids = {};

    do
    {
        rc = sqlite3_step(stmt);

        if (rc == SQLITE_ROW) 
        {
            int64_t p_id = sqlite3_column_int64(stmt, 0); 
            const char* p_text = reinterpret_cast<const char*>(sqlite3_column_text(stmt, 1));
            int64_t _w_id = sqlite3_column_int64(stmt, 2);
            int64_t w_id = sqlite3_column_int64(stmt, 3);

            if (paragraph_id == -1) 
            {
                paragraph_id = p_id;
                paragraph_text = p_text;
                word_id = _w_id;
                word_ids.push_back(w_id);
            } else
            {
                if (paragraph_id == p_id)
                {
                    word_ids.push_back(w_id);
                } else
                {
                    results.push_back(tuple(paragraph_id, paragraph_text, word_id, word_ids));
                    paragraph_id = p_id;
                    paragraph_text = p_text;
                    word_id = _w_id;
                    word_ids = {w_id};
                }
            }

        } else if (rc == SQLITE_DONE) 
        {

            if (paragraph_id != -1 && !word_ids.empty())
            {
                if (results.empty())
                {
                    results.push_back(tuple(paragraph_id, paragraph_text, word_id, word_ids));
                } else
                {
                    if (get<0>(results.back()) != paragraph_id)
                    {
                        results.push_back(tuple(paragraph_id, paragraph_text, word_id, word_ids));
                    }
                }
                
            }
        } else
        {
            cerr << "Err: " << rc << " Error while querying words to paragraphs: " << sqlite3_errmsg(db) << endl;
        }

    } while (rc == SQLITE_ROW);

    if (rc != SQLITE_DONE) 
    {
        cerr << "Err: " << rc << " Error while querying words to paragraphs: " << sqlite3_errmsg(db) << endl;
    }

#ifdef DATABASE_LOG_EXECUTION_TIMES
    t1  = chrono::high_resolution_clock::now();
    auto duration = chrono::duration_cast<chrono::microseconds>(t1 - t0);
    cout << "Time taken to query words to paragraphs table: " << duration.count() << " microseconds" << endl;
#endif
#ifdef DATABASE_EXPLAIN_QUERY_PLANS
    ExplainWordsTableQueryPlan(normalized_word, Type);
    const int scanStepsCt = sqlite3_stmt_status(stmt, SQLITE_STMTSTATUS_FULLSCAN_STEP, 0);
    const int sortCt = sqlite3_stmt_status(stmt, SQLITE_STMTSTATUS_SORT, 0);
    const int autoIdxCt = sqlite3_stmt_status(stmt, SQLITE_STMTSTATUS_AUTOINDEX, 0);

    cout << "Scan Steps: " << scanStepsCt << " Sort Count: " << sortCt << " Auto Index Count: " << autoIdxCt << endl;
#endif
    sqlite3_finalize(stmt);

    return results;
}

bool Database::BeginTransaction(const string TransactionName, const bool FailureUpsetsDatabaseValidity)
{
    rc = sqlite3_exec(db, "BEGIN TRANSACTION;", 0, 0, &errMsg);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " Failed to start transaction '" << TransactionName.c_str() << "': " << errMsg << endl;

        if (FailureUpsetsDatabaseValidity)
        {
            bIsValid = false;
        }

        sqlite3_free(errMsg);

        return false;
    } else
    {
        sqlite3_free(errMsg);

        return true;
    }
}

bool Database::FailTransaction(const string TransactionName, const string DeveloperErrorMessage, const bool FailureUpsetsDatabaseValidity, function<bool()> callback)
{
    cerr << "Err: " << rc << " Failing transaction '" << TransactionName << "' due to: " << DeveloperErrorMessage << " - " << sqlite3_errmsg(db) << endl;

    if (FailureUpsetsDatabaseValidity)
    {
        bIsValid = false;
    }

    if (callback)
    {
        return callback();
    } else
    {
        return true;
    }
}

bool Database::EndTransaction(const string TransactionName, const bool FailureUpsetsDatabaseValidity)
{
    rc = sqlite3_exec(db, "COMMIT TRANSACTION;", 0, 0, &errMsg);
    if (rc != SQLITE_OK) 
    {
        cerr << "Err: " << rc << " Failed to end transaction '" << TransactionName.c_str() << "': " << errMsg << endl;

        if (FailureUpsetsDatabaseValidity)
        {
            bIsValid = false;
        }

        sqlite3_free(errMsg);

        return false;
    } else
    {
        sqlite3_free(errMsg);

        return true;
    }
}

