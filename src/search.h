#pragma once

#define SEARCH_LOG_EXECUTION_TIMES    // uncomment this line to log execution times to the console
#define SEARCH_LOG_DEBUG_MESSAGES     // uncomment this line to log debug messages to the console

#include <vector>

#include "database.h"

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

    static unordered_map<int64_t, pair<int, int>> calculateParagraphScores(const vector<WordMatch>& matches);

    static bool rankParagraphs(const pair<int64_t, pair<int, int>>& a, const pair<int64_t, pair<int, int>>& b);

    static vector<int64_t> rankParagraphIds(const vector<WordMatch>& matches);

    static int searchBarInputCallback(ImGuiInputTextCallbackData* data);

    static char               SearchBarBuffer[MAX_PARAGRAPH_SIZE];
    static vector<string>     SearchResults;

protected:
    static Search*            Instance;
    static vector<WordMatch>  SearchProgress;
};