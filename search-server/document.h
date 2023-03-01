#pragma once

#include <iostream>

// There are only 2 functions, so addition of .cpp for them seems unnecessary
struct Document
{
    int id = 0;
    double relevance = 0.0;
    int rating = 0;
    Document() = default;
    Document(int id_, double relevance_, int rating_)
    {
        id = id_;
        relevance = relevance_;
        rating = rating_;
    }
}; // Not actual document (only id, relevance and rating for searching)
enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

void PrintDocument(const Document& document);
std::ostream& operator<< (std::ostream& out, const Document& document);