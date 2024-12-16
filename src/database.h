#pragma once

// #define DATABASE_EXPLAIN_QUERY_PLANS    // uncomment this line to log explainations of query plans to the console
#define DATABASE_LOG_EXECUTION_TIMES    // uncomment this line to log execution times to the console
#define MAX_WORD_SIZE (static_cast<size_t>(45))
#define MAX_PARAGRAPH_SIZE (static_cast<size_t>(200))
#define LOAD_TEST_DATA        // uncomment this line to load test data into the database upon initialization
#define ANALYZE_AFTER_LOAD    // uncomment this line to analyze the database to improve query speed after loading test data

#include <vector>
#include <filesystem>
#include <functional>

#include "../extern/sqlite3/sqlite3.h"


using namespace std;

static const char*             databaseFilepath  = "database.db";
static const filesystem::path  testDataFilepath  = "../config/database_test_data.yml"; 

typedef enum : uint8_t {
    EXACT_MATCH = 0,
    BEGINS_WITH = 1,
    ENDS_WITH = 2,
    CONTAINS = 3
} TextQueryType;


class Database {
private:
    Database();

    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

public:
    static Database* Get();

    static void Destroy();

    bool isValid() const;

    sqlite3* sqlite3Interface() const { return db; }

    bool ExplainWordsTableQueryPlan(const string& normalized_word, const TextQueryType Type);

    vector<int64_t> QueryWordsTableReturnIds(const string& normalized_word, const TextQueryType Type);

    pair<vector<int64_t>, vector<string>> QueryWordsTableReturnIdsWords(const string& normalized_word, const TextQueryType Type);

    vector<string> QueryParagraphsTableReturnOriginalText_Slow_Ordered(const vector<int64_t> paragraph_ids);

    vector<string> QueryParagraphsTableReturnOriginalText_Fast_Unordered(const vector<int64_t> paragraph_ids);

    /*
    * Haha.
    * I didn't know what else to call this function.
    * 
    * The output is a vector full of:
    *   pair.first: paragraph id (unique - each paragraph will only occur once)
    *   pair.second: a vector containing all of the word id's in that paragraph, in the order that they occur in that paragraph.
    */
    vector<pair<int64_t, vector<int64_t>>> QueryWordsToParagraphsTableReturnUniqueParagraphIdsAndAllWordIdsForTheParagraphInOrder(const int64_t word_id);
    vector<pair<int64_t, vector<int64_t>>> QueryWordsToParagraphsTableReturnUniqueParagraphIdsAndAllWordIdsForTheParagraphInOrder(const vector<int64_t> word_ids);

    bool BeginTransaction(const string TransactionName, const bool FailureUpsetsDatabaseValidity);

    bool FailTransaction(const string TransactionName, const string DeveloperErrorMessage, const bool FailureUpsetsDatabaseValidity, function<bool()> callback);

    bool EndTransaction(const string TransactionName, const bool FailureUpsetsDatabaseValidity);

protected:
    static Database* Instance;
    static sqlite3* db;
    static int rc;
    static char* errMsg;
    static bool bIsValid;
};

