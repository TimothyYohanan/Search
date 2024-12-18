#pragma once

#include <string>
#include <vector>
#include <unordered_map>

using namespace std;

struct WordMatch
{
    WordMatch() = delete;

    /*
    * For normal use
    */
    WordMatch(
        const string&                                           in_normalized_word, 
        const vector<int64_t>&                                  in_partial_match_idxs,
        const vector<tuple<int64_t, string, vector<int64_t>>>&  in_partial_match_data) noexcept;

    WordMatch(
        const string&                                           in_normalized_word, 
        const int64_t&                                          in_exact_match_idx, 
        const vector<tuple<int64_t, string, vector<int64_t>>>&  in_exact_match_data, 
        const vector<int64_t>&                                  in_partial_match_idxs, 
        const vector<tuple<int64_t, string, vector<int64_t>>>&  in_partial_match_data) noexcept;

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

    /*
    * get<0>(vector[i]) - paragraph id
    * get<1>(vector[i]) - paragraph original text
    * get<2>(vector[i]) - word ids, in the order that they appear in the paragraph
    */
    const vector<tuple<int64_t, string, vector<int64_t>>> exact_match_data;

    const vector<int64_t> partial_match_idxs;

    /*
    * get<0>(vector[i]) - paragraph id
    * get<1>(vector[i]) - paragraph original text
    * get<2>(vector[i]) - word ids, in the order that they appear in the paragraph
    */
    const vector<tuple<int64_t, string, vector<int64_t>>> partial_match_data;
};