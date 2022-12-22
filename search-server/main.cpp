#include <algorithm>
#include <numeric>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;
const double EPSILON = 1e-6;

enum class DocumentStatus
{
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};
struct DocumentSearchData
{
    int id;
    double relevance;
    int rating;
}; // Not actual document (only id, relevance and rating for searching)

vector<string> SplitIntoWords(const string& text)
{
    vector<string> words;
    string word;
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
int ComputeIntegerAverage(const vector<int>& values)
{
    int size = static_cast<int>(values.size());

    if (size == 0)
        return 0;
    else
        return accumulate(values.begin(), values.end(), 0) / size;
} // Basic average, exept result will be integer (not sure why)
#pragma region ReadFromConsole
string ReadLine()
{
    string s;
    getline(cin, s);

    return s;
} // Get line from console
int ReadLineWithNumber()
{
    int result = 0;
    cin >> result;
    ReadLine();

    return result;
} // Get an interger from console
vector<int> ReadLineOfNumbers()
{
    vector<string> values = SplitIntoWords(ReadLine());
    vector<int> result;

    for (const string value : values)
    {
        result.push_back(stoi(value));
    }
    result.erase(result.begin());

    return result;
} // Get multiple integers from console
#pragma endregion
#pragma region WriteToConsole
void PrintDocument(const DocumentSearchData& document)
{
    cout << "{ document_id = "s << document.id << ", relevance = "s << document.relevance << ", rating = "s << document.rating << " }"s << endl;
} // Specified version of simple document output
ostream& operator<< (ostream& out, const DocumentSearchData& document)
{
    out << "{ " << document.id << ", " << document.relevance << ", " << document.rating << " }";

    return out;
} // Short way to output document data
#pragma endregion

class SearchServer
{
public:
    void SetStopWords(const string& text)
    {
        for (const string& word : SplitIntoWords(text))
        {
            stop_words_.insert(word);
        }
    } // setup non important words
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        const double inv_word_count = 1.0 / words.size();
        for (const string& word : words)
        {
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }

        doc_rating_status_[document_id] = { ComputeIntegerAverage(ratings), status };
        document_count_++;
    }
    template <typename SortingFunction>
    vector<DocumentSearchData> FindTopDocuments(const string& raw_query, SortingFunction func) const
    {
        const Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, func);

        //sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) { return lhs.relevance > rhs.relevance; });
        sort(matched_documents.begin(), matched_documents.end(), [](const DocumentSearchData& lhs, const DocumentSearchData& rhs) { return lhs.relevance > rhs.relevance || (abs(lhs.relevance - rhs.relevance) <= EPSILON && lhs.rating > rhs.rating); });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);

        return matched_documents;
    } // Finds all matched documents (matching is determined by the function), then returns top ones (determine by MAX_RESULT_DOCUMENT_COUNT const)
    vector<DocumentSearchData> FindTopDocuments(const string& raw_query, DocumentStatus status = DocumentStatus::ACTUAL) const
    {
        return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus doc_status, int rating) { return doc_status == status; });
    } // Finds all matched documents (matching is determined by the status), then returns top ones (determine by MAX_RESULT_DOCUMENT_COUNT const)
    int GetDocumentCount() const
    {
        return document_count_;
    } // A way to access private field (as read only)
    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const
    {
        vector<string> plus_words;
        Query query = ParseQuery(raw_query);

        // Firstly, check if there are any stop words, so we won`t have to do the rest
        for (string word : query.minus_words)
        {
            if (word_to_document_freqs_.at(word).count(document_id) != 0)
            {
                return tuple(plus_words, doc_rating_status_.at(document_id).status);
            }
        }
        // If there is no minus words here, then find matches
        for (string word : query.plus_words)
        {
            if (word_to_document_freqs_.at(word).count(document_id) != 0)
            {
                plus_words.push_back(word);
            }
        }

        return tuple(plus_words, doc_rating_status_.at(document_id).status);
    }

private:
    struct Rating_Status
    {
        int rating;
        DocumentStatus status;
    };
    map<string, map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;
    set<string> stop_words_;
    map<int, Rating_Status> doc_rating_status_;

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };

    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    } // check if it is a non relevant word
    static bool isMinusWord(const string& word)
    {
        if (word[0] == '-')
            return true;
        else
            return false;
    } // For me it makes more sence to keep it neead IsStopWord() for convinience
    vector<string> SplitIntoWordsNoStop(const string& text) const
    {
        vector<string> words;
        for (const string& word : SplitIntoWords(text))
        {
            if (!IsStopWord(word))
                words.push_back(word);
        }
        return words;
    } // Parse text, excluding non important words
    Query ParseQuery(const string& text) const
    {
        Query query;

        for (string& word : SplitIntoWordsNoStop(text))
        {
            if (isMinusWord(word))
            {
                word.erase(0, 1); // removes minus from the word
                query.minus_words.insert(word);
            }
            else
            {
                query.plus_words.insert(word);
            }
        }
        return query;
    }  // Returns 2 sets of plus and minus words separatly (in that order)
    double IDF(const string& word) const
    {
        double relevance = log(document_count_ / static_cast<double>(word_to_document_freqs_.at(word).size()));

        return relevance;
    } // Inverse Document Frequency for word
    template <typename SortingFunction>
    vector<DocumentSearchData> FindAllDocuments(Query query, SortingFunction func) const
    {
        //[id, relevance]
        map<int, double> docs_id;
        for (const string& word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
                continue;
            double relevance = IDF(word);
            for (const auto& [id, tf] : word_to_document_freqs_.at(word))
            {
                if (!func(id, doc_rating_status_.at(id).status, doc_rating_status_.at(id).rating))
                    continue; // Don't even bother checking documents of other type
                docs_id[id] += relevance * tf;
            }
        }
        for (const string& word : query.minus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
                continue;
            for (const auto& [id, tf] : word_to_document_freqs_.at(word))
            {
                docs_id.erase(id);
            }
        }
        vector<DocumentSearchData> result;
        for (const auto& [id, relevance] : docs_id)
        {
            result.push_back({ id, relevance, doc_rating_status_.at(id).rating });
        }
        return result;
    } // Finds all somewhat relevant documents. Exeptance is regulated by the function with parameters: (id, status, rating)
}; // main class

int main()
{
    SearchServer search_server;

    search_server.SetStopWords("и в на"s);
    search_server.AddDocument(0, "белый кот и модный ошейник"s, DocumentStatus::ACTUAL, { 8, -3 });
    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "ухоженный пёс выразительные глаза"s, DocumentStatus::ACTUAL, { 5, -12, 2, 1 });
    search_server.AddDocument(3, "ухоженный скворец евгений"s, DocumentStatus::BANNED, { 9 });

    cout << "ACTUAL by default:"s << endl;
    for (const DocumentSearchData& document : search_server.FindTopDocuments("пушистый ухоженный кот"s))
    {
        PrintDocument(document);
    }
    cout << "ACTUAL:"s << endl;
    for (const DocumentSearchData& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; }))
    {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    for (const DocumentSearchData& document : search_server.FindTopDocuments("пушистый ухоженный кот"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; }))
    {
        PrintDocument(document);
    }

    return 0;
}