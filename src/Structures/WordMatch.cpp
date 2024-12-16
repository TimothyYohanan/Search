#include "WordMatch.h"

WordMatch::WordMatch(
    const string&                                  in_normalized_word, 
    const vector<int64_t>&                         in_partial_match_idxs,
    const vector<pair<int64_t, vector<int64_t>>>&  in_partial_match_data) noexcept :
        normalized_word(in_normalized_word), 
        exact_match_idx(-1), 
        exact_match_data({}), 
        partial_match_idxs(in_partial_match_idxs),
        partial_match_data(in_partial_match_data) {}

WordMatch::WordMatch(
    const string&                                  in_normalized_word, 
    const int64_t&                                 in_exact_match_idx, 
    const vector<pair<int64_t, vector<int64_t>>>&  in_exact_match_data, 
    const vector<int64_t>&                         in_partial_match_idxs, 
    const vector<pair<int64_t, vector<int64_t>>>&  in_partial_match_data) noexcept :
        normalized_word(in_normalized_word), 
        exact_match_idx(in_exact_match_idx), 
        exact_match_data(in_exact_match_data), 
        partial_match_idxs(in_partial_match_idxs),
        partial_match_data(in_partial_match_data) {}

WordMatch::WordMatch(
    WordMatch&& other) noexcept : 
        normalized_word(move(other.normalized_word)), 
        exact_match_idx(move(other.exact_match_idx)), 
        exact_match_data(move(other.exact_match_data)), 
        partial_match_idxs(move(other.partial_match_idxs)),
        partial_match_data(move(other.partial_match_data)) {}

WordMatch::WordMatch(
    const WordMatch& other) noexcept : 
    normalized_word(move(other.normalized_word)), 
    exact_match_idx(move(other.exact_match_idx)), 
    exact_match_data(move(other.exact_match_data)), 
    partial_match_idxs(move(other.partial_match_idxs)),
    partial_match_data(move(other.partial_match_data)) {}

WordMatch& WordMatch::operator=(const WordMatch& other) noexcept 
{ 
    // https://stackoverflow.com/questions/11601998/assignment-of-class-with-const-member
    if(this == &other) return *this;
    this->~WordMatch();
    return *new(this) WordMatch(other);
};
