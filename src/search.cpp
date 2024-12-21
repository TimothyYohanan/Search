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
ThreadPool*       Search::Pool = nullptr;

Search::Search() {
    Pool = new ThreadPool(1, 99);
}

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
    Pool->StopThreads();

    if (Pool)
    {
        delete Pool;
    }
    Pool = nullptr;

    if (Instance)
    {
        delete Instance;
    }
    Instance = nullptr;
}

inline const WordMatch Search::getMatches(Database* db_prechecked, const string& normalized_word)
{
    bool has_exact_matches;
    int64_t exact_match_idx;
    vector<tuple<int64_t, string, vector<int64_t>>> exact_match_data;
    bool has_partial_matches;
    vector<int64_t> partial_match_idxs;
    vector<tuple<int64_t, string, vector<int64_t>>> partial_match_data;

    future<void> exact_matches_future = Pool->Do([db_prechecked, normalized_word, &has_exact_matches, &exact_match_idx, &exact_match_data] {
        const vector<tuple<int64_t, string, int64_t, vector<int64_t>>> exact_matches = db_prechecked->GetAll_ParagraphId_ParagraphOriginalText_MatchedWordId_OrderedWordsInParagraphIds(normalized_word, TextQueryType::EXACT_MATCH, true);
        has_exact_matches = !exact_matches.empty();

        if (has_exact_matches)
        {
            exact_match_idx = get<2>(exact_matches[0]);
            exact_match_data.reserve(exact_matches.size());

            for (const tuple<int64_t, string, int64_t, vector<int64_t>>& data : exact_matches)
            {
                exact_match_data.push_back(tuple(get<0>(data), get<1>(data), get<3>(data)));
            }
        }
    });

    const vector<tuple<int64_t, string, int64_t, vector<int64_t>>> partial_matches = (normalized_word.size() == 1) ?
        db_prechecked->GetAll_ParagraphId_ParagraphOriginalText_MatchedWordId_OrderedWordsInParagraphIds(normalized_word, TextQueryType::BEGINS_WITH, false) :
        db_prechecked->GetAll_ParagraphId_ParagraphOriginalText_MatchedWordId_OrderedWordsInParagraphIds(normalized_word, TextQueryType::CONTAINS, false);
    has_partial_matches = !partial_matches.empty();

    if (has_partial_matches)
    {
        partial_match_idxs.reserve(partial_matches.size());
        partial_match_data.reserve(partial_matches.size());

        for (const tuple<int64_t, string, int64_t, vector<int64_t>>& data : partial_matches)
        {
            partial_match_idxs.push_back(get<2>(data));
            partial_match_data.push_back(tuple(get<0>(data), get<1>(data), get<3>(data)));
        }
    }
    
    exact_matches_future.get();

    if (!has_exact_matches)
    {
        if (has_partial_matches)
        {
            return WordMatch(normalized_word, partial_match_idxs, partial_match_data);
        } else
        {
            return WordMatch(normalized_word, {}, {});
        }
    } else
    {

#ifdef SEARCH_CHECK_FOR_ASSUMED_IMPOSSIBLE_ERRORS
    int64_t idx = get<2>(exact_matches[0]);
    for (const tuple<int64_t, string, int64_t, vector<int64_t>>& match : exact_matches)
    {
        if (get<2>(match) != idx)
        {
            cerr << "searchBarInputCallback() - ERROR: Found more than 1 word in the words database that was an exact match to the input '" << normalized_word <<"'" << endl;
            for (const tuple<int64_t, string, int64_t, vector<int64_t>>& match : exact_matches)
            {
                cerr << "   Note: matched paragraph id: " << get<0>(match) << ", word id: " << get<2>(match) << endl;
            }
            exit(EXIT_FAILURE);
        }
    }
#endif
        return WordMatch(normalized_word, exact_match_idx, exact_match_data, partial_match_idxs, partial_match_data);
    }
}

inline unordered_map<int64_t, pair<pair<int, int>, string>> Search::calculateParagraphScores(const vector<WordMatch>& matches) {
    unordered_map<int64_t, pair<pair<int, int>, string>> paragraphScores;

    for (const auto& wordMatch : matches) {
        for (const auto& exactMatch : wordMatch.exact_match_data) {
            int64_t paragraphId = get<0>(exactMatch);
            string originalText = get<1>(exactMatch);
            paragraphScores[paragraphId].first.first++;
            if (paragraphScores[paragraphId].second.empty()) {
                paragraphScores[paragraphId].second = originalText;
            }
        }

        for (const auto& partialMatch : wordMatch.partial_match_data) {
            int64_t paragraphId = get<0>(partialMatch);
            string originalText = get<1>(partialMatch);
            paragraphScores[paragraphId].first.second++;
            if (paragraphScores[paragraphId].second.empty()) {
                paragraphScores[paragraphId].second = originalText;
            }
        }
    }

    return paragraphScores;
}

inline bool Search::rankParagraphs(const pair<int64_t, pair<pair<int, int>, string>>& a, const pair<int64_t, pair<pair<int, int>, string>>& b) {
    int exactA = a.second.first.first;
    int partialA = a.second.first.second;
    int exactB = b.second.first.first;
    int partialB = b.second.first.second;

    if (exactA != exactB) {
        return exactA > exactB;
    }

    if (partialA != partialB) {
        return partialA > partialB;
    }

    return a.first < b.first;
}

inline vector<pair<int64_t, string>> Search::rankParagraphIds(const vector<WordMatch>& matches) {
    unordered_map<int64_t, pair<pair<int, int>, string>> paragraphScores = calculateParagraphScores(matches);

    vector<pair<int64_t, pair<pair<int, int>, string>>> paragraphScoreList;
    for (const auto& entry : paragraphScores) {
        paragraphScoreList.push_back(entry);
    }

    sort(paragraphScoreList.begin(), paragraphScoreList.end(), rankParagraphs);

    vector<pair<int64_t, string>> rankedParagraphsWithText;
    for (const auto& entry : paragraphScoreList) {
        rankedParagraphsWithText.push_back({entry.first, entry.second.second});
    }

    return rankedParagraphsWithText;
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
                            const WordMatch match = getMatches(db, normalized_word);
                            SearchProgress[i] = match;
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

            const vector<pair<int64_t, string>> rankedParagraphIds = rankParagraphIds(SearchProgress);
            SearchResults.clear();
            SearchResults.reserve(rankedParagraphIds.size());
            for (const auto& entry : rankedParagraphIds) {
                SearchResults.push_back(entry.second);
            }


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
