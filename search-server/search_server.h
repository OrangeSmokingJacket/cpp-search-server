#pragma once

#include <vector>
#include <set>
#include <map>
#include <string>
#include <cmath>
#include <algorithm>
#include <numeric>
#include <stdexcept>

#include "document.h"
#include "string_processing.h"

const double EPSILON = 1e-6;
const int MAX_RESULT_DOCUMENT_COUNT = 5;

class SearchServer
{
public:
#pragma region Constructors
    SearchServer() = default;
    template <typename Container>
    explicit SearchServer(Container stop_words_to_add)
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
    explicit SearchServer(const std::string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {} // oneliner doesn't hurt
#pragma endregion
    void AddDocument(int document_id, const std::string& document, DocumentStatus status, const std::vector<int>& ratings);

    template <typename SortingFunction>
    std::vector<Document> FindTopDocuments(const std::string& raw_query, SortingFunction func) const
    {
        // exeptions are handled inside of ParseQuery() function
        Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, func);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) { return lhs.relevance > rhs.relevance || (std::abs(lhs.relevance - rhs.relevance) <= EPSILON && lhs.rating > rhs.rating); });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);

        return matched_documents;
    } // Finds all matched documents (matching is determined by the function), then returns top ones (determine by MAX_RESULT_DOCUMENT_COUNT const)

    std::vector<Document> FindTopDocuments(const std::string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const;

    std::tuple<std::vector<std::string>, DocumentStatus> MatchDocument(const std::string& raw_query, int document_id) const;

    int GetDocumentCount() const;

    int GetDocumentId(int index);
private:
    struct Rating_Status
    {
        int rating;
        DocumentStatus status;
    };
    std::map<std::string, std::map<int, double>> word_to_document_freqs;
    std::set<std::string> stop_words;
    std::map<int, Rating_Status> doc_rating_status;
    std::vector<int> ids;

    struct Query
    {
        std::set<std::string> plus_words;
        std::set<std::string> minus_words;
    };
    enum class WordStatus
    {
        Plus = 0,
        Minus = 1
    };
    struct Word
    {
        std::string word;
        WordStatus status;
    };

    bool IsStopWord(const std::string& word) const; // check if it is a non relevant word
    static bool IsValidWord(const std::string& word)
    {
        return none_of(word.begin(), word.end(), [](char c) { return c >= 0 && c <= 31; });
    } // A valid word must not contain special characters
    static bool IsMinusWord(const std::string& word)
    {
        if (word[0] == '-')
        {
            if (IsCorrectMinus(word))
                return true;
            else
                throw std::invalid_argument("Word: " + word + "; incorrect minus word.");
        }
        else
            return false;
    } // For me it makes more sence to keep it here
    static bool IsCorrectMinus(const std::string& word)
    {
        if (word == "-")
            return false; // last char is not minus
        if (word[1] == '-') // couldn't be done in one if; will result in error if word has only ona character
            return false;
        return true;
    }
    static int ComputeIntegerAverage(const std::vector<int>& values)
    {
        int size = static_cast<int>(values.size());

        if (size == 0)
            return 0;
        else
            return std::accumulate(values.begin(), values.end(), 0) / size;
    } // Basic average, exept result will be integer (not sure why)

    Word ValidateWord(const std::string& word) const;
    std::vector<std::string> SplitIntoWordsNoStop(const std::string& text) const; // Parse text, excluding non important words
    Query ParseQuery(const std::string& text) const; // Returns 2 sets of plus and minus words separatly (in that order)

    double Calculate_IDF(const std::string& word) const; // Inverse Document Frequency for word

    template <typename SortingFunction>
    std::vector<Document> FindAllDocuments(Query query, SortingFunction func) const
    {
        //[id, relevance]
        std::map<int, double> docs_id;
        for (const std::string& word : query.plus_words)
        {
            if (word_to_document_freqs.count(word) == 0)
                continue;
            double relevance = Calculate_IDF(word);
            for (const auto& [id, tf] : word_to_document_freqs.at(word))
            {
                if (!func(id, doc_rating_status.at(id).status, doc_rating_status.at(id).rating))
                    continue; // Don't even bother checking documents of other type
                docs_id[id] += relevance * tf;
            }
        }
        for (const std::string& word : query.minus_words)
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
}; // main class