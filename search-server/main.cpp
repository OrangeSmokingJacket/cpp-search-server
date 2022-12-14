#include <algorithm>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

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
} // Get interger from console
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

struct Document
{
    int id;
    double relevance;
}; // Not actual document (only id and relevance for searching)
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
    void AddDocument(int document_id, const string& document)
    {
        const vector<string> words = SplitIntoWordsNoStop(document);
        set<string> unicWords;

        for (const string& word : words)
        {
            unicWords.insert(word);
        }
        for (const string& word : unicWords)
        {
            if (stop_words_.count(word) == 0)
            {
                double tf = count(words.begin(), words.end(), word) / static_cast<double>(words.size());
                word_to_document_freqs_[word][document_id] = tf;
            }
        }

        document_count_++;
    }
    vector<Document> FindTopDocuments(const string& raw_query) const
    {
        const set<Word> query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words);

        sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {return lhs.relevance > rhs.relevance; });

        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT)
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);

        return matched_documents;
    } // Finds all matched documents, then returns top ones (determine by MAX_RESULT_DOCUMENT_COUNT const)

private:
    map<string, map<int, double>> word_to_document_freqs_;
    int document_count_ = 0;
    set<string> stop_words_;
    struct Word
    {
        string word;
        bool isMinus;
    };

    bool IsStopWord(const string& word) const
    {
        return stop_words_.count(word) > 0;
    } // check if it is a non relevant word
    bool isMinusWord(const string& word) const
    {
        if (word[0] == '-')
            return true;
        else
            false;
    }
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
    set<Word> ParseQuery(const string& text) const
    {
        set<Word> query_words;
        for (const string& query_word : SplitIntoWordsNoStop(text))
        {
            Word word;
            word.word = query_word;
            word.isMinus = isMinusWord(query_word);
            query_words.insert(word);
        }
        return query_words;
    }  // Get unic words in text
    double IDF(const string& word) const
    {
        double relevance = log(document_count_ / static_cast<double>(word_to_document_freqs_.at(word).size()));

        return relevance;
    } // Inverse Document Frequency for word
    vector<Document> FindAllDocuments(const set<Word>& query_words) const
    {
        //[id, relevance]
        map<int, double> docs_id;
        for (const Word& word : query_words)
        {
            if (!word.isMinus)
            {
                if (word_to_document_freqs_.count(word.word) == 0)
                    continue;
                double relevance = IDF(word.word);
                for (const auto& [id, tf] : word_to_document_freqs_.at(word.word))
                {
                    docs_id[id] += relevance * tf;
                }
            }
            else
            {
                if (word_to_document_freqs_.count(word.word) == 0)
                    continue;
                for (const auto& [id, tf] : word_to_document_freqs_.at(word.word))
                {
                    docs_id.erase(id);
                }
            }
        }
        vector<Document> result;
        for (const auto& [id, relevance] : docs_id)
        {
            result.push_back({ id, relevance });
        }
        return result;
    } // Finds all somewhat relevant documents
}; // main class
SearchServer CreateSearchServer()
{
    SearchServer search_server;
    search_server.SetStopWords(ReadLine());

    const int document_count = ReadLineWithNumber();
    for (int document_id = 0; document_id < document_count; document_id++)
    {
        search_server.AddDocument(document_id, ReadLine());
    }

    return search_server;
} // Search Server constructor

int main()
{
    const SearchServer search_server = CreateSearchServer();

    const string query = ReadLine();
    for (const auto& [document_id, relevance] : search_server.FindTopDocuments(query))
    {
        cout << "{ document_id = "s << document_id << ", " << "relevance = "s << relevance << " }"s << endl;
    }
}