#pragma once

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

struct WordMatch
{
    /*
    * For normal use
    */
    WordMatch(
        const string&                                  in_normalized_word, 
        const vector<int64_t>&                         in_partial_match_idxs,
        const vector<pair<int64_t, vector<int64_t>>>&  in_partial_match_data) noexcept;

    WordMatch(
        const string&                                  in_normalized_word, 
        const int64_t&                                 in_exact_match_idx, 
        const vector<pair<int64_t, vector<int64_t>>>&  in_exact_match_data, 
        const vector<int64_t>&                         in_partial_match_idxs, 
        const vector<pair<int64_t, vector<int64_t>>>&  in_partial_match_data) noexcept;

    /*
    * Added for vector opperations.
    * If the members were not marked const, these would not be needed.
    */
    WordMatch(WordMatch&& other) noexcept;
    WordMatch(const WordMatch& other) noexcept;
    WordMatch& operator=(const WordMatch& other) noexcept;

    bool foundExactMatch() const { return exact_match_idx >= 0; }
    bool foundPartialMatches() const { return !partial_match_idxs.empty(); }

    const string normalized_word;

    const int64_t exact_match_idx;

    const vector<pair<int64_t, vector<int64_t>>> exact_match_data; // in each pair, the first element is the paragraph id, and the second element is the ordered word id's within the paragraph

    const vector<int64_t> partial_match_idxs;

    const vector<pair<int64_t, vector<int64_t>>> partial_match_data; // in each pair, the first element is the paragraph id, and the second element is the ordered word id's within the paragraph
};