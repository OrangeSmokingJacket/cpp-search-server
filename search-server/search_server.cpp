#include "search_server.h"

using namespace std;

bool IsValidWord(string_view word)
{
    return none_of(word.begin(), word.end(), [](char c) { return c >= 0 && c <= 31; });
}
bool IsCorrectMinus(string_view word)
{
    if (word == "-")
        return false; // last char is not minus
    if (word[1] == '-') // couldn't be done in one if; will result in error if word has only ona character
        return false;
    return true;
}
bool IsMinusWord(string_view word)
{
    if (word[0] == '-')
    {
        if (IsCorrectMinus(word))
            return true;
        else
            throw invalid_argument("Word: " + static_cast<string>(word) + "; incorrect minus word.");
    }
    else
        return false;
}
int ComputeIntegerAverage(const vector<int>& values)
{
    int size = static_cast<int>(values.size());

    if (size == 0)
        return 0;
    else
        return accumulate(values.begin(), values.end(), 0) / size;
} // Basic average, exept result will be integer (not sure why)

void SearchServer::AddDocument(int document_id, string_view text_document, DocumentStatus status, const vector<int>& ratings)
{
    if (document_id < 0)
        throw invalid_argument("id can't be negative. Got: " + document_id + '.');
    if (doc_rating_status.count(document_id) > 0)
        throw invalid_argument("This id already exists: " + document_id + '.');

    documents[document_id] = static_cast<string>(text_document);

    const vector<string_view> words = SplitIntoWordsNoStop(documents[document_id]);
    for (const string_view& word : words)
    {
        if (!IsValidWord(word))
            throw invalid_argument("Word: " + static_cast<string>(word) + "; contains a special symbol.");
    }

    const double inv_word_count = 1.0 / words.size();
    for (const string_view& word : words)
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

    for (const auto& [word, frequency] : document_word_frequencies[document_id])
    {
        word_to_document_freqs[word].erase(document_id);
    }

    document_word_frequencies.erase(document_id);
}
void SearchServer::RemoveDocument(execution::sequenced_policy policy, int document_id)
{
    SearchServer::RemoveDocument(document_id);
}
void SearchServer::RemoveDocument(execution::parallel_policy policy, int document_id)
{
    ids.erase(find(ids.begin(), ids.end(), document_id));
    doc_rating_status.erase(document_id);

    vector<const string_view*> elements_to_remove(document_word_frequencies.at(document_id).size());
    transform
    (
        execution::par, document_word_frequencies.at(document_id).begin(), document_word_frequencies.at(document_id).end(), elements_to_remove.begin(),
        [](pair<const string_view, double>& item) { return &item.first; }
    );

    for_each
    (
        execution::par, elements_to_remove.begin(), elements_to_remove.end(),
        [this, &document_id](const string_view* item)
        {
            word_to_document_freqs[*item].erase(document_id);
        }
    );

    document_word_frequencies.erase(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query, [status](int document_id, DocumentStatus doc_status, int rating) { return doc_status == status; });
} // Finds all matched documents (matching is determined by the status), then returns top ones (determine by MAX_RESULT_DOCUMENT_COUNT const)
vector<Document> SearchServer::FindTopDocuments(execution::sequenced_policy, string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(raw_query, status);
} // Finds all matched documents (matching is determined by the status), then returns top ones (determine by MAX_RESULT_DOCUMENT_COUNT const)
vector<Document> SearchServer::FindTopDocuments(execution::parallel_policy, string_view raw_query, DocumentStatus status) const
{
    return FindTopDocuments(execution::par, raw_query, [status](int document_id, DocumentStatus doc_status, int rating) { return doc_status == status; });
} // Finds all matched documents (matching is determined by the status), then returns top ones (determine by MAX_RESULT_DOCUMENT_COUNT const)

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const
{
    vector<string_view> matched_words;

    if (!ids.contains(document_id))
        return tuple(matched_words, doc_rating_status.at(document_id).status);

    Query query = ParseQuery(raw_query);
    // Firstly, check if there are any stop words, so we won`t have to do the rest
    for (string_view word : query.minus_words)
    {
        if (!word_to_document_freqs.contains(word))
            return tuple(matched_words, doc_rating_status.at(document_id).status);

        if (word_to_document_freqs.at(word).count(document_id) != 0)
            return tuple(matched_words, doc_rating_status.at(document_id).status);
    }
    // If there is no minus words here, then find matches
    for (string_view word : query.plus_words)
    {
        if (word_to_document_freqs.count(word) == 0)
            continue;

        if (word_to_document_freqs.at(word).count(document_id) != 0)
        {
            matched_words.push_back(word);
        }
    }
    return tuple(matched_words, doc_rating_status.at(document_id).status);
}
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(execution::sequenced_policy policy, string_view raw_query, int document_id) const
{
    return SearchServer::MatchDocument(raw_query, document_id);
}
tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(execution::parallel_policy policy, string_view raw_query, int document_id) const
{
    vector<string_view> matched_words;

    if (!ids.contains(document_id))
        return tuple(matched_words, doc_rating_status.at(document_id).status);

    Query query = ParseQuery(policy, raw_query);
    // Firstly, check if there are any stop words, so we won`t have to do the rest

    if (any_of(policy, query.minus_words.begin(), query.minus_words.end(), [this, &document_id](const string_view& word) { return document_word_frequencies.at(document_id).contains(word); }))
        return tuple(matched_words, doc_rating_status.at(document_id).status);

    matched_words.resize(query.plus_words.size());

    matched_words.resize(
        copy_if
        (
            policy, query.plus_words.begin(), query.plus_words.end(), matched_words.begin(),
            [this, &document_id](const string_view& word)
            {
                return word_to_document_freqs.at(word).contains(document_id);
            }
        ) - matched_words.begin()
    );

    sort(policy, matched_words.begin(), matched_words.end());
    matched_words.erase(unique(policy, matched_words.begin(), matched_words.end()), matched_words.end());

    return tuple(matched_words, doc_rating_status.at(document_id).status);
}
int SearchServer::GetDocumentCount() const
{
    return static_cast<int>(doc_rating_status.size());
}
const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const
{
    const static map<string_view, double> result;

    //Tiny optimization
    if (ids.count(document_id) == 0)
        return result;

    return document_word_frequencies.at(document_id);
}

bool SearchServer::IsStopWord(string_view word) const
{
    return stop_words.count(word) > 0;
} // check if it is a non relevant word
SearchServer::Word SearchServer::ValidateWord(string_view word) const
{
    Word valid_word;
    valid_word.word = word;
    if (!IsValidWord(word))
        throw invalid_argument("Word: " + static_cast<string>(word) + "; contains a special symbol.");
    if (IsMinusWord(word))
    {
        valid_word.word.remove_prefix(1); // removes minus from the word
        valid_word.status = WordStatus::Minus;
        return valid_word;
    }
    valid_word.status = WordStatus::Plus;
    return valid_word;
}
vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const
{
    vector<string_view> words;
    for (const string_view& word : SplitIntoWords(text))
    {
        if (!IsStopWord(word))
            words.push_back(word);
    }
    return words;
} // Parse text, excluding non important words
SearchServer::Query SearchServer::ParseQuery(string_view text) const
{
    Query query;
    for (string_view& word : SplitIntoWords(text))
    {
        Word valid_word = ValidateWord(word);
        if (IsStopWord(valid_word.word))
            continue;

        if (valid_word.status == WordStatus::Minus)
            query.minus_words.push_back(valid_word.word);
        else
            query.plus_words.push_back(valid_word.word);
    }

    sort(query.plus_words.begin(), query.plus_words.end());
    query.plus_words.erase(unique(query.plus_words.begin(), query.plus_words.end()), query.plus_words.end());
    sort(query.minus_words.begin(), query.minus_words.end());
    query.minus_words.erase(unique(query.minus_words.begin(), query.minus_words.end()), query.minus_words.end());


    return query;
}
SearchServer::Query SearchServer::ParseQuery(execution::sequenced_policy policy, string_view text) const
{
    return SearchServer::ParseQuery(text);
}
SearchServer::Query SearchServer::ParseQuery(execution::parallel_policy policy, string_view text) const
{
    Query query;
    vector<string_view> parsed = SplitIntoWords(text);

    for (string_view& word : parsed)
    {
        Word valid_word = ValidateWord(word);
        if (IsStopWord(valid_word.word))
            continue;

        if (valid_word.status == WordStatus::Minus)
            query.minus_words.push_back(valid_word.word);
        else
            query.plus_words.push_back(valid_word.word);
    }

    return query;
}

double SearchServer::CalculateIDF(string_view word) const
{
    double relevance = log(static_cast<int>(doc_rating_status.size()) / static_cast<double>(word_to_document_freqs.at(word).size()));

    return relevance;
} // Inverse Document Frequency for word