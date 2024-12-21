#pragma once

#define SEARCH_LOG_EXECUTION_TIMES                  // uncomment this line to log execution times to the console
#define SEARCH_LOG_DEBUG_MESSAGES                   // uncomment this line to log debug messages to the console
// #define SEARCH_CHECK_FOR_ASSUMED_IMPOSSIBLE_ERRORS  // checks for errors that should, theoretically, never happen

#include <vector>

#include "database.h"
#include "linux_threadpool.h"

#include "Structures/NormalizedText.h"
#include "Structures/WordMatch.h"

#include "../extern/imgui/imgui.h"


using namespace std;

class Search {
private:
    Search();

    Search(const Search&) = delete;
    Search& operator=(const Search&) = delete;

public:
    static Search* Get();

    static void Destroy();

    static inline const WordMatch getMatches(Database* db_prechecked, const string& normalized_word);

    static inline unordered_map<int64_t, pair<pair<int, int>, string>> calculateParagraphScores(const vector<WordMatch>& matches);

    static inline bool rankParagraphs(const pair<int64_t, pair<pair<int, int>, string>>& a, const pair<int64_t, pair<pair<int, int>, string>>& b);

    static inline vector<pair<int64_t, string>> rankParagraphIds(const vector<WordMatch>& matches);

    static int searchBarInputCallback(ImGuiInputTextCallbackData* data);

    static char               SearchBarBuffer[MAX_PARAGRAPH_SIZE];
    static vector<string>     SearchResults;

protected:
    static Search*            Instance;
    static vector<WordMatch>  SearchProgress;

    static ThreadPool*        Pool;
};