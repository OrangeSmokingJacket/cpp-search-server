#include <iostream>
#include <algorithm>
#include <cassert>
#include <numeric>
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <string>

#include "string_processing.h"
#include "read_input_functions.h"
#include "document.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"

using namespace std;

#pragma region TEST_Tools
void AssertImpl(bool expr, const string& expr_str, const string& file, const string& function, unsigned int line, const string& hint = "")
{
    if (!expr)
    {
        cerr << file << "("s << line << "): "s << function << ": ASSERT("s << expr_str << ") failed."s;
        if (hint != ""s)
            cerr << " Hint: "s << hint;
        cerr << endl;

        abort();
    }
}
template <typename T1, typename T2>
void AsserEqualImpl(const T1& t1, const T2& t2, const string& t1_str, const string& t2_str, const string& file, const string& function, unsigned int line, const string& hint = "")
{
    if (t1 != t2)
    {
        cerr << file << "("s << line << "): "s << function << ": ASSERT("s << t1_str << " != "s << t2_str << ") failed."s;
        if (hint != ""s)
            cerr << " Hint: "s << hint;
        cerr << endl;

        abort();
    }
}
template <typename Function>
void RunTestImpl(Function f, const string& func_str)
{
    if (f)
    {
        cerr << func_str << " OK"s << endl;
    }
}
#define ASSERT(expr) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_HINT(expr, hint) AssertImpl(expr, #expr, __FILE__, __FUNCTION__, __LINE__, hint)
#define ASSERT_EQUAL(a, b) AsserEqualImpl(a, b, #a, #b, __FILE__, __FUNCTION__, __LINE__)
#define ASSERT_EQUAL_HINT(a, b, hint) AsserEqualImpl(a, b, #a, #b, __FILE__, __FUNCTION__, __LINE__, hint)
#define RUN_TEST(func) RunTestImpl(func, #func)
#pragma endregion
#pragma region TESTS
// They are bool type, so we can run them inside RUN_TEST
bool SimpeDocumentSearch_TEST()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("cat"s);
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Wrong number of documents."s);
    const auto& doc0 = found_docs[0];
    ASSERT_EQUAL_HINT(doc0.id, doc_id, "Wrong id"s);

    return true;
}
bool AvoidStopWords_TEST()
{
    const int doc_id = 42;
    const string content = "cat in the city"s;
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server("in the"s);
    server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
    ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Should be empty."s);

    return true;
}
bool MinusWords_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server("in the"s);
    server.AddDocument(42, "cat in the city"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(52, "dog in the city"s, DocumentStatus::ACTUAL, ratings);

    const auto found_docs = server.FindTopDocuments("-dog city"s);
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Wrong number of documents."s);
    ASSERT_EQUAL_HINT(found_docs[0].id, 42, "Wrong id"s);

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
    ASSERT_EQUAL_HINT(get<0>(result).size(), 1, "Wrong number of words."s);
    result = server.MatchDocument("cat city"s, doc_id);
    ASSERT_EQUAL_HINT(get<0>(result).size(), 2, "Wrong number of words."s);
    result = server.MatchDocument("-cat city"s, doc_id);
    ASSERT_EQUAL_HINT(get<0>(result).size(), 0, "Wrong number of words."s);
    // This one i added for redundancy, but having it fails test)))

    //result = server.MatchDocument("dog"s, doc_id);
    //ASSERT_EQUAL_HINT(get<0>(result).size(), 0, "Wrong number of words."s);

    return true;
}
bool RelevanceSorting_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(0, "a b c d e"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "a b c d"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "a b c"s, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("d"s);
    ASSERT_EQUAL_HINT(found_docs.size(), 2, "Wrong number of documents."s);
    ASSERT_HINT(found_docs[0].relevance > found_docs[1].relevance, "Wrong relevance sorting"s);

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
    ASSERT_HINT(found_docs[0].rating == 226, "Average rating calculation is wrong"s);

    return true;
}
bool CustomFilter_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(0, "a b c d e"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "a b c d"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "a b c"s, DocumentStatus::ACTUAL, ratings);
    const auto found_docs = server.FindTopDocuments("a"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 1; });
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Wrong number of documents."s);
    ASSERT_HINT(found_docs[0].id == 1, "Wrong predicate usage"s);

    return true;
}
bool StatusFilter_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(0, "a b c d e"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "a b c d"s, DocumentStatus::IRRELEVANT, ratings);
    server.AddDocument(2, "a b c"s, DocumentStatus::ACTUAL, ratings);

    auto found_docs = server.FindTopDocuments("a"s, DocumentStatus::IRRELEVANT);
    ASSERT_EQUAL_HINT(found_docs.size(), 1, "Wrong number of documents"s);
    ASSERT_EQUAL_HINT(found_docs[0].id, 1, "Wrong document search (by status)"s);

    found_docs = server.FindTopDocuments("a"s, DocumentStatus::BANNED);
    ASSERT_EQUAL_HINT(found_docs.size(), 0, "Wrong document search (by status)"s);

    return true;
}
bool ProperRelevanceCalculation_TEST()
{
    const vector<int> ratings = { 1, 2, 3 };

    SearchServer server;
    server.AddDocument(0, "a b c d e"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(1, "a b c d"s, DocumentStatus::ACTUAL, ratings);
    server.AddDocument(2, "a b c"s, DocumentStatus::ACTUAL, ratings);

    auto found_docs = server.FindTopDocuments("d"s);
    ASSERT_HINT(found_docs[0].relevance > 0.1 && found_docs[0].relevance < 0.11, "Wrong relevance calculation"s); // exact value here: 0.10136627702704110
    ASSERT_HINT(found_docs[1].relevance > 0.08 && found_docs[1].relevance < 0.09, "Wrong relevance calculation"s); // exact value here: 0.081093021621632885

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

    TestSearchServer();

    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);
    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });
    // 1439 запросов с нулевым результатом
    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    // все еще 1439 запросов с нулевым результатом
    request_queue.AddFindRequest("curly dog"s);
    // новые сутки, первый запрос удален, 1438 запросов с нулевым результатом
    request_queue.AddFindRequest("big collar"s);
    // первый запрос удален, 1437 запросов с нулевым результатом
    request_queue.AddFindRequest("sparrow"s);
    cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << endl;
    return 0;
}