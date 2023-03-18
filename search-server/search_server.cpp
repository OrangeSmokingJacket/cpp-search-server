#include "search_server.h"

using namespace std;

bool IsValidWord(const std::string& word)
{
    return none_of(word.begin(), word.end(), [](char c) { return c >= 0 && c <= 31; });
}
bool IsCorrectMinus(const std::string& word)
{
    if (word == "-")
        return false; // last char is not minus
    if (word[1] == '-') // couldn't be done in one if; will result in error if word has only ona character
        return false;
    return true;
}
bool IsMinusWord(const std::string& word)
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
}
int ComputeIntegerAverage(const std::vector<int>& values)
{
    int size = static_cast<int>(values.size());

    if (size == 0)
        return 0;
    else
        return std::accumulate(values.begin(), values.end(), 0) / size;
} // Basic average, exept result will be integer (not sure why)

void SearchServer::AddDocument(int document_id, const string& text_document, DocumentStatus status, const vector<int>& ratings)
{
    if (document_id < 0)
        throw invalid_argument("id can't be negative. Got: " + document_id + '.');
    if (doc_rating_status.count(document_id) > 0)
        throw invalid_argument("This id already exists: " + document_id + '.');

    const vector<string> words = SplitIntoWordsNoStop(text_document);
    for (const string& word : words)
    {
        if (!IsValidWord(word))
            throw invalid_argument("Word: " + word + "; contains a special symbol.");
    }

    const double inv_word_count = 1.0 / words.size();
    for (const string& word : words)
    {
        word_to_document_freqs[word][document_id] += inv_word_count;
        document_word_frequencies[document_id][word] += inv_word_count;
    }

    doc_rating_status[document_id] = { ComputeIntegerAverage(ratings), status };
    ids.insert(document_id);
}
void SearchServer::RemoveDocument(int document_id)
{
    ids.erase(find(ids.begin(), ids.end(), document_id));
    doc_rating_status.erase(document_id);

    for (const auto [word, frequency] : document_word_frequencies[document_id])
    {
        word_to_document_freqs[word].erase(document_id);
    }

    document_word_frequencies.erase(document_id);
}
vector<Document> SearchServer::FindTopDocuments(const string& raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus doc_status, int rating) { return doc_status == status; });
} // Finds all matched documents (matching is determined by the status), then returns top ones (determine by MAX_RESULT_DOCUMENT_COUNT const)
tuple<vector<string>, DocumentStatus> SearchServer::MatchDocument(const string& raw_query, int document_id) const
{
    //LOG_DURATION_STREAM("Operation time", cout);
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
    return static_cast<int>(doc_rating_status.size());
}
const map<string, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    const static map<string, double> result;

    //Tiny optimization
    if (ids.count(document_id) == 0)
        return result;

    return document_word_frequencies.at(document_id);
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
    if (IsMinusWord(word))
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

double SearchServer::CalculateIDF(const std::string& word) const
{
    double relevance = log(static_cast<int>(doc_rating_status.size()) / static_cast<double>(word_to_document_freqs.at(word).size()));

    return relevance;
} // Inverse Document Frequency for word