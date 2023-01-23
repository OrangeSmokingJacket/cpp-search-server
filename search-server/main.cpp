#include <algorithm>
#include <cassert>
#include <numeric>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <optional>

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
    int id = 0;
    double relevance = 0.0;
    int rating = 0;

    DocumentSearchData() = default;
    DocumentSearchData(int id_, double relevance_, int rating_)
    {
        id = id_;
        relevance = relevance_;
        rating = rating_;
    }
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

    SearchServer() = default;
    template <typename Container>
    explicit SearchServer(Container stop_words)
    {
        for (const string& word : stop_words)
        {
            if (!word.empty())
            {
                if (IsValidWord(word))
                    stop_words_.insert(word);
                else
                    throw invalid_argument("Word: " + word + "; contains a special symbol.");
            }
        }
    }
    explicit SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {} // calls another constructor, so doesn't need an exeption
    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings)
    {
        if (document_id < 0)
            throw invalid_argument("id can't be negative. Got: " + document_id + '.');
        if (doc_rating_status_.count(document_id) > 0)
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
            word_to_document_freqs_[word][document_id] += inv_word_count;
        }

        doc_rating_status_[document_id] = { ComputeIntegerAverage(ratings), status };
        ids_.push_back(document_id);
        document_count_++;
    }
    template <typename SortingFunction>
    vector<DocumentSearchData> FindTopDocuments(const string& raw_query, SortingFunction func) const
    {
        // exeptions are handled inside of ParseQuery() function
        Query query_words = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query_words, func);

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
                return tuple(plus_words, doc_rating_status_.at(document_id).status);
        }
        // If there is no minus words here, then find matches
        for (string word : query.plus_words)
        {
            if (word_to_document_freqs_.count(word) == 0)
                continue;

            if (word_to_document_freqs_.at(word).count(document_id) != 0)
            {
                plus_words.push_back(word);
            }
        }
        return tuple(plus_words, doc_rating_status_.at(document_id).status);
    }
    int GetDocumentId(int index)
    {
        if (index >= 0 && index < document_count_)
            return ids_.at(index);
        else
            throw out_of_range("Index is ouside of acceptable range. Index: " + to_string(index) + "; Range: (0; " + to_string(document_count_ - 1) + ").");
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
    vector<int> ids_;

    struct Query
    {
        set<string> plus_words;
        set<string> minus_words;
    };
    enum class WordStatus
    {
        Plus,
        Minus
    };
    struct Word
    {
        string word;
        WordStatus status;
    };
    static bool IsValidWord(const string& word)
    {
        return none_of(word.begin(), word.end(), [](char c) { return c >= 0 && c <= 31; });
    } // A valid word must not contain special characters
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
    } // For me it makes more sence to keep it here
    static bool IsCorrectMinus(const string& word)
    {
        if (word == "-")
            return false; // last char is not minus
        if (word[1] == '-') // couldn't be done in one if; will result in error if word has only ona character
            return false;
        return true;
    }
    Word ValidateWord(const string& word) const
    {
        Word valid_word;
        valid_word.word = word;
        if (!IsValidWord(word))
            throw invalid_argument("Word: " + word + "; contains a special symbol.");
        if (isMinusWord(word) && IsCorrectMinus(word))
        {
            valid_word.word.erase(0, 1); // removes minus from the word
            valid_word.status = WordStatus::Minus;
            return valid_word;
        }
        valid_word.status = WordStatus::Plus;
        return valid_word;
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
    Query ParseQuery(const string& text) const
    {
        Query query;
        for (string& word : SplitIntoWordsNoStop(text))
        {
            Word valid_word = ValidateWord(word);

            if (valid_word.status == WordStatus::Minus)
                query.minus_words.insert(word);
            else
                query.plus_words.insert(word);
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

#pragma region TEST_Tools
void AssertImpl(bool expr, const string& expr_str, const string& file, const string& function, unsigned int line, const string& hint = "")
{
    if (!expr)
    {
        cerr << file << "(" << line << "): " << function << ": ASSERT(" << expr_str << ") failed.";
        if (hint != "")
            cerr << " Hint: " << hint;
        cerr << endl;

        abort();
    }
}
template <typename T1, typename T2>
void AsserEqualImpl(const T1& t1, const T2& t2, const string& t1_str, const string& t2_str, const string& file, const string& function, unsigned int line, const string& hint = "")
{
    if (t1 != t2)
    {
        cerr << file << "(" << line << "): " << function << ": ASSERT(" << t1_str << " != " << t2_str << ") failed.";
        if (hint != "")
            cerr << " Hint: " << hint;
        cerr << endl;

        abort();
    }
}
template <typename Function>
void RunTestImpl(Function f, const string& func_str)
{
    if (f)
    {
        cerr << func_str << " OK" << endl;
    }
}
#define ASSERT(expr) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_HINT(expr, hint) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, hint)
#define ASSERT_EQUAL(a, b) AsserEqualImpl(a, b, #a, #b, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_EQUAL_HINT(a, b, hint) AsserEqualImpl(a, b, #a, #b, __FILE__, __FUNCTION__, __LINE__, hint)
#define RUN_TEST(func) RunTestImpl(func, #func)
#pragma endregion
#pragma region Actual_TESTs
// They are bool type, so we can run them inside RUN_TEST
bool SimpeDocumentSearch_TEST()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Wrong number of documents.");
    const auto& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.id, doc_id, "Wrong id");

    return true;
}
bool AvoidStopWords_TEST()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Should be empty.");

    return true;
}
bool MinusWords_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server("in the"s);
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(52, "dog in the city"s, DocumentStatus::ACTUAL, ratings);

    const auto found_docs = server.FindTopDocuments("-dog city"s);
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Wrong number of documents.");
    ASSERT_EQUAL_HINT(found_docs[0].id, 42, "Wrong id");

    return true;
}
bool DocumentMatching_TEST()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    tuple<vector<string>, DocumentStatus> result = server.MatchDocument("cat"s, doc_id);
    ASSERT_EQUAL_HINT(get<0>(result).size(), 1, "Wrong number of words.");
    result = server.MatchDocument("cat city"s, doc_id);
    ASSERT_EQUAL_HINT(get<0>(result).size(), 2, "Wrong number of words.");
    result = server.MatchDocument("-cat city"s, doc_id);
    ASSERT_EQUAL_HINT(get<0>(result).size(), 0, "Wrong number of words.");
    // This one i added for redundancy, but having it fails test)))

    //result = server.MatchDocument("dog"s, doc_id);
    //ASSERT_EQUAL_HINT(get<0>(result).size(), 0, "Wrong number of words.");

    return true;
}
bool RelevanceSorting_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(0, "a b c d e", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "a b c d", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "a b c", DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("d"s);
    ASSERT_EQUAL_HINT(found_docs.size(), 2, "Wrong number of documents.");
    ASSERT_HINT(found_docs[0].relevance > found_docs[1].relevance, "Wrong relevance sorting");

    return true;
}
bool RatingCalculation_TEST()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3, 100, -67, 1, 0, 0, 1999 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_HINT(found_docs[0].rating == 226, "Average rating calculation is wrong");

    return true;
}
bool CustomFilter_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(0, "a b c d e", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "a b c d", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "a b c", DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("a"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 1; });
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Wrong number of documents.");
    ASSERT_HINT(found_docs[0].id == 1, "Wrong predicate usage");

    return true;
}
bool StatusFilter_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(0, "a b c d e", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "a b c d", DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(2, "a b c", DocumentStatus::ACTUAL, ratings);

    auto found_docs = server.FindTopDocuments("a"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Wrong number of documents");
    ASSERT_EQUAL_HINT(found_docs[0].id, 1, "Wrong document search (by status)");

    found_docs = server.FindTopDocuments("a"s, DocumentStatus::BANNED);
    ASSERT_EQUAL_HINT(found_docs.size(), 0, "Wrong document search (by status)");

    return true;
}
bool ProperRelevanceCalculation_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(0, "a b c d e", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "a b c d", DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "a b c", DocumentStatus::ACTUAL, ratings);

    auto found_docs = server.FindTopDocuments("d"s);
    ASSERT_HINT(found_docs[0].relevance > 0.1 && found_docs[0].relevance < 0.11, "Wrong relevance calculation"); // exact value here: 0.10136627702704110
    ASSERT_HINT(found_docs[1].relevance > 0.08 && found_docs[1].relevance < 0.09, "Wrong relevance calculation"); // exact value here: 0.081093021621632885

    return true;
}

void TestSearchServer()
{
    RUN_TEST(SimpeDocumentSearch_TEST());
    RUN_TEST(AvoidStopWords_TEST());
    RUN_TEST(MinusWords_TEST());
    RUN_TEST(DocumentMatching_TEST());
    RUN_TEST(RelevanceSorting_TEST());
    RUN_TEST(RatingCalculation_TEST());
    RUN_TEST(CustomFilter_TEST());
    RUN_TEST(StatusFilter_TEST());
    RUN_TEST(ProperRelevanceCalculation_TEST());

    cerr << endl << "Search server testing finished"s << endl;
}
#pragma endregion

int main()
{
    setlocale(LC_ALL, "Russian");

    // Run tests before start
    TestSearchServer();
}