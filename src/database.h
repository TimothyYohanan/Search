#pragma once

// #define DATABASE_EXPLAIN_QUERY_PLANS    // uncomment this line to log explainations of query plans to the console
#define DATABASE_LOG_EXECUTION_TIMES    // uncomment this line to log execution times to the console
// #define DATABASE_LOG_PREPARED_STATEMENTS    // uncomment this line to log prepared statements to the console before they are executed
#define MAX_WORD_SIZE (static_cast<size_t>(45))
#define MAX_PARAGRAPH_SIZE (static_cast<size_t>(200))
#define LOAD_TEST_DATA        // uncomment this line to load test data into the database upon initialization
#define ANALYZE_AFTER_LOAD    // uncomment this line to analyze the database to improve query speed after loading test data

#include <vector>
#include <filesystem>
#include <functional>

#include "../extern/sqlite3/sqlite3.h"


using namespace std;

static const char              databaseFilepath[12]  = "database.db";
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

    sqlite3* sqlite3Interface() const { return db_mainThread; }

    bool ExplainWordsTableQueryPlan(const string& normalized_word, const TextQueryType Type);

    vector<int64_t> QueryWordsTableReturnIds(const string& normalized_word, const TextQueryType Type);

    /*
    * get<0>(vector[i]) = paragraph id (unique - each paragraph will only occur once)
    * get<1>(vector[i]) = paragraph original text
    * get<2>(vector[i]) = the matched word id
    * get<3>(vector[i]) = a vector containing all of the word ids in the paragraph, in the order that they occur in that paragraph
    */
    vector<tuple<int64_t, string, int64_t, vector<int64_t>>> GetAll_ParagraphId_ParagraphOriginalText_MatchedWordId_OrderedWordsInParagraphIds(const string& normalized_word, const TextQueryType Type, const bool UseBackgroundThread);

    /*
    * get<0>(vector[i]) = paragraph id (unique - each paragraph will only occur once)
    * get<1>(vector[i]) = paragraph original text
    * get<2>(vector[i]) = the matched word id
    * get<3>(vector[i]) = a vector containing all of the word ids in the paragraph, in the order that they occur in that paragraph
    * get<4>(vector[i]) = indicates wether the paragraph contains an exact match to the word, or a partial match to the word
    */
    // vector<tuple<int64_t, string, int64_t, vector<int64_t>, bool>> GetAll_ParagraphId_ParagraphOriginalText_MatchedWordId_OrderedWordsInParagraphIds(const string& normalized_word);

    bool BeginTransaction(const string TransactionName, const bool FailureUpsetsDatabaseValidity);

    bool FailTransaction(const string TransactionName, const int rc, const string DeveloperErrorMessage, const bool FailureUpsetsDatabaseValidity, function<bool()> callback);

    bool EndTransaction(const string TransactionName, const bool FailureUpsetsDatabaseValidity);

protected:
    static char* errMsg;
    static Database* Instance;
    static sqlite3* db_mainThread;
    static sqlite3* db_backgroundThread;
    static bool bIsValid;
};

