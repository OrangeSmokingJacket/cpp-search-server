#pragma once

#include <vector>
#include <set>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>
#include <execution>

#include "document.h"
#include "string_processing.h"
#include "concurrent_map.h"

const double EPSILON = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

bool IsValidWord(std::string_view word);
bool IsCorrectMinus(std::string_view word);
bool IsMinusWord(std::string_view word);
int ComputeIntegerAverage(const std::vector<int>& values);

class SearchServer
{
public:
#pragma region Constructors
    SearchServer() = default;
    template<template<typename...> typename Container>
    explicit SearchServer(Container<std::string> stop_words_to_add);
    template<template<typename...> typename Container>
    explicit SearchServer(Container<std::string_view> stop_words_to_add);
    //explicit SearchServer(const std::string_view& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {} -> For some reason, this one tries to call itself, but next one is working
    explicit SearchServer(std::string_view stop_words_text)
    {
        for (const std::string_view& word : SplitIntoWords(stop_words_text))
        {
            if (!word.empty())
            {
                if (IsValidWord(word))
                    stop_words.insert(static_cast<std::string>(word));
                else
                    throw std::invalid_argument("Word: " + static_cast<std::string>(word) + "; contains a special symbol.");
            }
        }
    }
#pragma endregion
    void AddDocument(int document_id, std::string_view text_document, DocumentStatus status, const std::vector<int>& ratings);
    void RemoveDocument(int document_id);
    void RemoveDocument(std::execution::sequenced_policy policy, int document_id);
    void RemoveDocument(std::execution::parallel_policy policy, int document_id);

    template <typename SortingFunction>
    std::vector<Document> FindTopDocuments(std::string_view raw_query, SortingFunction func) const;
    template <typename SortingFunction>
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy, std::string_view raw_query, SortingFunction func) const;
    template <typename SortingFunction>
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy, std::string_view raw_query, SortingFunction func) const;

    std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
    std::vector<Document> FindTopDocuments(std::execution::sequenced_policy, std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;
    std::vector<Document> FindTopDocuments(std::execution::parallel_policy, std::string_view raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;

    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::sequenced_policy policy, std::string_view raw_query, int document_id) const;
    std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::execution::parallel_policy policy, std::string_view raw_query, int document_id) const;

    int GetDocumentCount() const;
    const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;
    auto begin()
    {
        return ids.begin();
    }
    auto end()
    {
        return ids.end();
    }
private:
    struct Rating_Status
    {
        int rating;
        DocumentStatus status;
    };
    std::map<int, std::string> documents;
    std::map<int, std::map<std::string_view, double>> document_word_frequencies;
    std::map<std::string_view, std::map<int, double>> word_to_document_freqs;
    std::set<std::string, std::less<>> stop_words;
    std::map<int, Rating_Status> doc_rating_status;
    std::set<int> ids;

    struct Query
    {
        std::vector<std::string_view> plus_words;
        std::vector<std::string_view> minus_words;
    };
    enum class WordStatus
    {
        Plus = 0,
        Minus = 1
    };
    struct Word
    {
        std::string_view word;
        WordStatus status;
    };

    bool IsStopWord(std::string_view word) const; // check if it is a non relevant word

    Word ValidateWord(std::string_view word) const;
    std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const; // Parse text, excluding non important words

    Query ParseQuery(std::string_view text) const; // Returns 2 sets of plus and minus words separatly (in that order)
    Query ParseQuery(std::execution::sequenced_policy policy, std::string_view text) const;
    Query ParseQuery(std::execution::parallel_policy policy, std::string_view text) const;

    double CalculateIDF(std::string_view word) const; // Inverse Document Frequency for word

    template <typename SortingFunction>
    std::vector<Document> FindAllDocuments(Query query, SortingFunction func) const;
    template <typename SortingFunction>
    std::vector<Document> FindAllDocuments(std::execution::sequenced_policy, Query query, SortingFunction func) const;
    template <typename SortingFunction>
    std::vector<Document> FindAllDocuments(std::execution::parallel_policy, Query query, SortingFunction func) const;
}; // main class

template<template<typename...> typename Container>
SearchServer::SearchServer(Container<std::string> stop_words_to_add)
{
    for (const std::string& word : stop_words_to_add)
    {
        if (!word.empty())
        {
            if (IsValidWord(word))
                stop_words.insert(word);
            else
                throw std::invalid_argument("Word: " + word + "; contains a special symbol.");
        }
    }
}
template<template<typename...> typename Container>
SearchServer::SearchServer(Container<std::string_view> stop_words_to_add)
{
    for (const std::string_view& word : stop_words_to_add)
    {
        if (!word.empty())
        {
            if (IsValidWord(word))
                stop_words.insert(static_cast<std::string>(word));
            else
                throw std::invalid_argument("Word: " + static_cast<std::string>(word) + "; contains a special symbol.");
        }
    }
}

template <typename SortingFunction>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, SortingFunction func) const
{
    // exeptions are handled inside of ParseQuery() function
    Query query_words = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(query_words, func);

    std::sort(matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs)
        { return lhs.relevance > rhs.relevance || (std::abs(lhs.relevance - rhs.relevance) <= EPSILON && lhs.rating > rhs.rating); }
    );

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);

    return matched_documents;
} // Finds all matched documents (matching is determined by the function), then returns top ones (determine by MAX_RESULT_DOCUMENT_COUNT const)
template <typename SortingFunction>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::sequenced_policy, std::string_view raw_query, SortingFunction func) const
{
    return FindTopDocuments(raw_query, func);
}
template <typename SortingFunction>
std::vector<Document> SearchServer::FindTopDocuments(std::execution::parallel_policy, std::string_view raw_query, SortingFunction func) const
{
    // exeptions are handled inside of ParseQuery() function
    Query query_words = ParseQuery(raw_query);
    auto matched_documents = FindAllDocuments(std::execution::par, query_words, func);

    std::sort(std::execution::par, matched_documents.begin(), matched_documents.end(),
        [](const Document& lhs, const Document& rhs)
        { return lhs.relevance > rhs.relevance || (std::abs(lhs.relevance - rhs.relevance) <= EPSILON && lhs.rating > rhs.rating); }
    );

    if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
        matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);

    return matched_documents;
}

template <typename SortingFunction>
std::vector<Document> SearchServer::FindAllDocuments(Query query, SortingFunction func) const
{
    //[id, relevance]
    std::map<int, double> docs_id;
    for (const std::string_view& word : query.plus_words)
    {
        if (word_to_document_freqs.count(word) == 0)
            continue;
        double relevance = CalculateIDF(word);
        for (const auto& [id, tf] : word_to_document_freqs.at(word))
        {
            if (!func(id, doc_rating_status.at(id).status, doc_rating_status.at(id).rating))
                continue; // Don't even bother checking documents of other type
            docs_id[id] += relevance * tf;
        }
    }
    for (const std::string_view& word : query.minus_words)
    {
        if (word_to_document_freqs.count(word) == 0)
            continue;
        for (const auto& [id, tf] : word_to_document_freqs.at(word))
        {
            docs_id.erase(id);
        }
    }
    std::vector<Document> result;
    for (const auto& [id, relevance] : docs_id)
    {
        result.push_back({ id, relevance, doc_rating_status.at(id).rating });
    }
    return result;
} // Finds all somewhat relevant documents. Exeptance is regulated by the function with parameters: (id, status, rating)
template <typename SortingFunction>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::sequenced_policy, Query query, SortingFunction func) const
{
    return FindAllDocuments(query, func);
}
template <typename SortingFunction>
std::vector<Document> SearchServer::FindAllDocuments(std::execution::parallel_policy, Query query, SortingFunction func) const
{
    const int threads_num = 8;
    //[id, relevance]
    ConcurrentMap<int, double> docs_id(threads_num);
    for_each
    (
        std::execution::par, query.plus_words.begin(), query.plus_words.end(),
        [&](const std::string_view& word)
        {
            if (word_to_document_freqs.count(word) != 0)
            {
                double relevance = CalculateIDF(word);
                for_each
                (
                    std::execution::par, word_to_document_freqs.at(word).begin(), word_to_document_freqs.at(word).end(),
                    [&](const std::pair<int, double> item)
                    {
                        if (func(item.first, doc_rating_status.at(item.first).status, doc_rating_status.at(item.first).rating))
                            docs_id[item.first].ref_to_value += relevance * item.second;
                    }
                );
            }
        }
    );
    for_each
    (
        std::execution::par, query.minus_words.begin(), query.minus_words.end(),
        [&](const std::string_view& word)
        {
            if (word_to_document_freqs.count(word) != 0)
            {
                for (const auto& [id, tf] : word_to_document_freqs.at(word))
                {
                    docs_id.erase(id);
                }
            }
        }
    );
    std::vector<Document> result;
    for (const auto& [id, relevance] : docs_id.BuildOrdinaryMap())
    {
        result.push_back({ id, relevance, doc_rating_status.at(id).rating });
    }
    return result;
} // Finds all somewhat relevant documents. Exeptance is regulated by the function with parameters: (id, status, rating)