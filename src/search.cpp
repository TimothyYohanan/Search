#include <iostream>
#include <memory>
#include <algorithm>

#ifdef SEARCH_LOG_EXECUTION_TIMES
#include <chrono>
#endif

#include "search.h"

#include "../extern/sqlite3/sqlite3.h"

#include "DDL/table_words.h"
#include "DDL/table_paragraphs.h"
#include "DDL/table_words_to_paragraphs.h"

Search*           Search::Instance = nullptr;
char              Search::SearchBarBuffer[MAX_PARAGRAPH_SIZE] = "";
vector<WordMatch> Search::SearchProgress = {};
vector<string>    Search::SearchResults = {};

Search::Search() { }

Search* Search::Get() 
{
    if (!Instance)
    {
        Instance = new Search();
        return Instance;
    }
    return Instance;
}

void Search::Destroy() 
{
    if (Instance)
    {
        delete Instance;
    }
    Instance = nullptr;
}

inline const WordMatch Search::getMatches(Database* db_prechecked, const string& normalized_word)
{
    const vector<int64_t> exact_match = db_prechecked->QueryWordsTableReturnIds(normalized_word, TextQueryType::EXACT_MATCH);
    vector<int64_t> partial_matches = (normalized_word.size() == 1) ?
        db_prechecked->QueryWordsTableReturnIds(normalized_word, TextQueryType::BEGINS_WITH) :
        db_prechecked->QueryWordsTableReturnIds(normalized_word, TextQueryType::CONTAINS);
    if (exact_match.empty())
    {
        const vector<pair<int64_t, vector<int64_t>>> partial_match_data = db_prechecked->QueryWordsToParagraphsTableReturnUniqueParagraphIdsAndAllWordIdsForTheParagraphInOrder(partial_matches);
        return WordMatch(normalized_word, partial_matches, partial_match_data);
    } else if (exact_match.size() > 1)
    {
        cout << "searchBarInputCallback() - ERROR: found more than one exact matches. This should be impossible." << endl;
        exit(EXIT_FAILURE);
    } else
    {
        const int64_t exact_match_idx = exact_match[0];
        const vector<int64_t>::iterator it = find(partial_matches.begin(), partial_matches.end(), exact_match_idx);

        if (it != partial_matches.end()) {
            partial_matches.erase(it);
        }

        const vector<pair<int64_t, vector<int64_t>>> exact_match_data = db_prechecked->QueryWordsToParagraphsTableReturnUniqueParagraphIdsAndAllWordIdsForTheParagraphInOrder(exact_match_idx);
        const vector<pair<int64_t, vector<int64_t>>> partial_match_data = db_prechecked->QueryWordsToParagraphsTableReturnUniqueParagraphIdsAndAllWordIdsForTheParagraphInOrder(partial_matches);

        return WordMatch(normalized_word, exact_match_idx, exact_match_data, partial_matches, partial_match_data);
    }
}

unordered_map<int64_t, pair<int, int>> Search::calculateParagraphScores(const vector<WordMatch>& matches) {
    unordered_map<int64_t, pair<int, int>> paragraphScores;

    for (const auto& wordMatch : matches) {
        for (const auto& exactMatch : wordMatch.exact_match_data) {
            int64_t paragraphId = exactMatch.first;
            paragraphScores[paragraphId].first++;
        }

        for (const auto& partialMatch : wordMatch.partial_match_data) {
            int64_t paragraphId = partialMatch.first;
            paragraphScores[paragraphId].second++;
        }
    }

    return paragraphScores;
}

bool Search::rankParagraphs(const pair<int64_t, pair<int, int>>& a, const pair<int64_t, pair<int, int>>& b) {
    int exactA = a.second.first;
    int partialA = a.second.second;
    int exactB = b.second.first;
    int partialB = b.second.second;

    // First prioritize by the number of exact matches
    if (exactA != exactB) {
        return exactA > exactB;
    }

    // If exact matches are the same, prioritize by the number of partial matches
    if (partialA != partialB) {
        return partialA > partialB;
    }

    // If both exact and partial match counts are the same, prioritize by paragraph ID (earlier comes first)
    return a.first < b.first;
}

vector<int64_t> Search::rankParagraphIds(const vector<WordMatch>& matches) {
    unordered_map<int64_t, pair<int, int>> paragraphScores = calculateParagraphScores(matches);

    vector<pair<int64_t, pair<int, int>>> paragraphScoreList;
    for (const auto& entry : paragraphScores) {
        paragraphScoreList.push_back(entry);
    }

    sort(paragraphScoreList.begin(), paragraphScoreList.end(), rankParagraphs);

    vector<int64_t> rankedParagraphIds;
    for (const auto& entry : paragraphScoreList) {
        rankedParagraphIds.push_back(entry.first);
    }

    return rankedParagraphIds;
}

int Search::searchBarInputCallback(ImGuiInputTextCallbackData* data) {
    Database* db = Database::Get();

    if (db->isValid())
    {
#ifdef SEARCH_LOG_EXECUTION_TIMES
        chrono::_V2::system_clock::time_point t0  = chrono::_V2::system_clock::time_point();
        chrono::_V2::system_clock::time_point t1  = chrono::_V2::system_clock::time_point();

        t0  = chrono::high_resolution_clock::now();
#endif
        if (data->BufTextLen == 0)
        {
            /*
            * The search bar does not contain any text.
            */
            SearchProgress.clear();
            SearchResults.clear();
        } else
        {
            /*
            * There is text in the search bar.
            */
            if (SearchProgress.empty())
            {
                /*
                * This is the start of a new search.
                * Either the user typed the first character in their search, or pasted a string of text to the search bar.
                */
                const NormalizedText normalized_text(data->Buf, MAX_PARAGRAPH_SIZE);

                const size_t nWords = normalized_text.normalized_words.size();

                for (size_t i = 0; i < nWords; ++i)
                {
                    const string& normalized_word = normalized_text.normalized_words[i];
                    SearchProgress.push_back(getMatches(db, normalized_word));
                }
            } else
            {
                /*
                * The user is continuing their search.
                * Either they typed the next character in their search, or pasted a string of text to the search bar, or used autocorrect.
                */
                const NormalizedText normalized_text(data->Buf, MAX_PARAGRAPH_SIZE);

                const size_t nWords = normalized_text.normalized_words.size();
                const size_t nWordMatches = SearchProgress.size();

                for (size_t i = 0; i < nWords; ++i)
                {
                    const string& normalized_word = normalized_text.normalized_words[i];

                    if (i < nWordMatches)
                    {
                        if (normalized_word != SearchProgress[i].normalized_word)
                        {
                            SearchProgress[i] = getMatches(db, normalized_word);
                        }
                    } else
                    {
                        SearchProgress.push_back(getMatches(db, normalized_word));
                    }
                }

                for (size_t i = nWords; i < nWordMatches; ++i)
                {
                    SearchProgress.pop_back();
                }
            }

            vector<int64_t> exact_word_match_ids = {};
            for (const WordMatch& match : SearchProgress)
            {
                if (match.foundExactMatch())
                {
                    exact_word_match_ids.push_back(match.exact_match_idx);
                }
            }

            const vector<int64_t> rankedParagraphIds = rankParagraphIds(SearchProgress);
            SearchResults = db->QueryParagraphsTableReturnOriginalText_Slow_Ordered(rankedParagraphIds);


#ifdef SEARCH_LOG_EXECUTION_TIMES
            cout << "\nsearchBarInputCallback() - Query itteration report:" << endl;
            t1  = chrono::high_resolution_clock::now();
            auto duration = chrono::duration_cast<chrono::microseconds>(t1 - t0);
            cout << "Time taken to search the database: " << duration.count() << " microseconds" << endl;
#endif
#ifdef SEARCH_LOG_DEBUG_MESSAGES
            cout << "Words in query: " << endl;
            for(const WordMatch& match : SearchProgress)
            {
                cout << "  '" << match.normalized_word.c_str() << "' - Exact Word Matches: " << (match.foundExactMatch() ? "1" : "0") << ", Partial Word Matches: " << match.partial_match_idxs.size() << endl;
                cout << "     Number of paragraphs containing an exact match: " << match.exact_match_data.size() << endl;
                cout << "     Number of paragraphs containing a partial match: " << match.partial_match_data.size() << endl;
            }
            cout << "\n" <<endl;
# endif
        }
    } else
    {
        cerr << "searchBarInputCallback() - Did not run search because database is invalid." << endl;
    }

    return 0; // means "don't make any modifications to the input"
}
