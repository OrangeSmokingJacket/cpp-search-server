#include "search_server.h"

using namespace std;

void SearchServer::AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
{
    if (document_id < 0)
        throw invalid_argument("id can't be negative. Got: " + document_id + '.');
    if (doc_rating_status.count(document_id) > 0)
        throw invalid_argument("This id already exists: " + document_id + '.');

    const vector<string> words = SplitIntoWordsNoStop(document);
    for (const string& word : words)
    {
        if (!IsValidWord(word))
            throw invalid_argument("Word: " + word + "; contains a special symbol.");
    }

    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words)
    {
        word_to_document_freqs[word][document_id] += inv_word_count;
    }

    doc_rating_status[document_id] = { ComputeIntegerAverage(ratings), status };
    ids.push_back(document_id);
    document_count++;
}
tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const
{
    vector<string> plus_words;

    Query query = ParseQuery(raw_query);
    // Firstly, check if there are any stop words, so we won`t have to do the rest
    for (string word : query.minus_words)
    {
        if (word_to_document_freqs.at(word).count(document_id) != 0)
            return tuple(plus_words, doc_rating_status.at(document_id).status);
    }
    // If there is no minus words here, then find matches
    for (string word : query.plus_words)
    {
        if (word_to_document_freqs.count(word) == 0)
            continue;

        if (word_to_document_freqs.at(word).count(document_id) != 0)
        {
            plus_words.push_back(word);
        }
    }
    return tuple(plus_words, doc_rating_status.at(document_id).status);
}
int SearchServer::GetDocumentCount() const
{
    return document_count;
}
int SearchServer::GetDocumentId(int index)
{
    if (index >= 0 && index < document_count)
        return ids.at(index);
    else
        throw out_of_range("Index is ouside of acceptable range. Index: " + to_string(index) + "; Range: (0; " + to_string(document_count - 1) + ").");
}

bool SearchServer::IsStopWord(const string& word) const
{
    return stop_words.count(word) > 0;
} // check if it is a non relevant word
SearchServer::Word SearchServer::ValidateWord(const string& word) const
{
    Word valid_word;
    valid_word.word = word;
    if (!IsValidWord(word))
        throw invalid_argument("Word: " + word + "; contains a special symbol.");
    if (isMinusWord(word))
    {
        valid_word.word.erase(0, 1); // removes minus from the word
        valid_word.status = WordStatus::Minus;
        return valid_word;
    }
    valid_word.status = WordStatus::Plus;
    return valid_word;
}
vector<string> SearchServer::SplitIntoWordsNoStop(const string& text) const
{
    vector<string> words;
    for (const string& word : SplitIntoWords(text))
    {
        if (!IsStopWord(word))
            words.push_back(word);
    }
    return words;
} // Parse text, excluding non important words
SearchServer::Query SearchServer::ParseQuery(const string& text) const
{
    Query query;
    for (string& word : SplitIntoWords(text))
    {
        Word valid_word = ValidateWord(word);
        if (IsStopWord(valid_word.word))
            continue;

        if (valid_word.status == WordStatus::Minus)
            query.minus_words.insert(valid_word.word);
        else
            query.plus_words.insert(valid_word.word);
    }
    return query;
}  // Returns 2 sets of plus and minus words separatly (in that order)

double SearchServer::IDF(const std::string& word) const
{
    double relevance = log(document_count / static_cast<double>(word_to_document_freqs.at(word).size()));

    return relevance;
} // Inverse Document Frequency for word