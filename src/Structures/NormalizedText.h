#pragma once

#include <iostream>
#include <vector>

using namespace std;

struct NormalizedText
{
    NormalizedText() = delete;

    NormalizedText(const char* text, const size_t max_text_size);

    NormalizedText(string text, const size_t max_text_size);

    size_t original_text_size;

    string original_text;

    size_t normalized_text_size;

    string normalized_text;

    vector<string> normalized_words;

    inline void F1();
};
