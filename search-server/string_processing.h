#pragma once

#include <vector>
#include <string>

// if implementation is here, it fails platform tests
// if implementation is in .cpp, it fails to start on my end (Visual Studio 2019)
std::vector<std::string> SplitIntoWords(const std::string& text)
{
    std::vector<std::string> words;
    std::string word;
    for (const char c : text)
    {
        if (c == ' ')
        {
            if (!word.empty())
            {
                words.push_back(word);
                word.clear();
            }
        }
        else
        {
            word += c;
        }
    }
    if (!word.empty())
        words.push_back(word);

    return words;
} // parse text into vector of words