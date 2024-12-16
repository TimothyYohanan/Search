#include <tuple>

#include "NormalizedText.h"


NormalizedText::NormalizedText(const char* text, const size_t max_text_size) : original_text_size{0}, normalized_text_size{0}, normalized_text{""}
{
    for (size_t i = 0; i < max_text_size; ++i) 
    {
        if (text[i] == '\0') 
        {
            original_text_size = i;
            break;
        }
    }
    
    if (original_text_size == 0 || original_text_size > max_text_size)
    {
        cerr << "Attempted to normalize some text that was either empty, or too long." << endl;
        exit(EXIT_FAILURE);
    } else
    {
        original_text = text;
        F1();
    }
};

NormalizedText::NormalizedText(string text, const size_t max_text_size) : original_text_size{0}, normalized_text_size{0}, normalized_text{""}
{
    size_t size = text.size();
    
    if (size == 0 || size > max_text_size)
    {
        cerr << "Attempted to normalize some text that was either empty, or too long." << endl;
        exit(EXIT_FAILURE);
    } else
    {
        original_text = text;
        original_text_size = size;
        F1();
    }
};

inline void NormalizedText::F1()
{
    string normalized_word = "";

    for (size_t i = 0; i < original_text_size; ++i) 
    {
        if (original_text[i] == '&') 
        {
            normalized_text += "and";
            normalized_word += "and";
        } else if (original_text[i] == '-' || original_text[i] == '_') 
        {
            continue;
        } else if (original_text[i] == ' ') 
        {
            if (normalized_text.empty() || normalized_text.back() != ' ') 
            {
                normalized_text += ' ';
                normalized_words.push_back(normalized_word);
                normalized_word = "";
            }
        } else 
        {
            char normalized_character = static_cast<char>(tolower(original_text[i]));
            normalized_text += normalized_character;
            normalized_word += normalized_character;
        }
    }

    if (!normalized_word.empty() && normalized_word != " ") 
    {
        normalized_words.push_back(normalized_word);
    }

    normalized_text_size = normalized_text.size();
};

