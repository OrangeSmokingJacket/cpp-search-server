#include <iostream>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <vector>
#include <set>
#include <map>
#include <queue>
#include <string>
#include <execution>
#include <random>

#include "string_processing.h"
#include "read_input_functions.h"
#include "document.h"
#include "search_server.h"
#include "paginator.h"
#include "request_queue.h"
#include "remove_duplicates.h"
#include "log_duration.h"
#include "tests.h"

using namespace std;

void AddDocument(SearchServer& search_server, int id, string text_doc, DocumentStatus status, vector<int> rating)
{
    search_server.AddDocument(id, text_doc, status, rating);
}

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> result(queries.size());
    std::transform
    (
        std::execution::par, queries.begin(), queries.end(), result.begin(),
        [&search_server](const std::string& q) { return search_server.FindTopDocuments(q); }
    );

    return result;
}
std::vector<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries)
{
    std::vector<std::vector<Document>> top_docs = ProcessQueries(search_server, queries);
    std::vector<Document> result;
    for (const std::vector<Document>& top : top_docs)
    {
        for (const Document& doc : top)
        {
            result.push_back(doc);
        }
    }

    return result;
}

using namespace std;

int main()
{
    SearchServer search_server("and with"s);
    int id = 0;
    for (
        const string& text : {
            "white cat and yellow hat"s,
            "curly cat curly tail"s,
            "nasty dog with big eyes"s,
            "nasty pigeon john"s,
        }
        ) {
        search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, { 1, 2 });
    }
    cout << "ACTUAL by default:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
        PrintDocument(document);
    }
    cout << "BANNED:"s << endl;
    // последовательная версия
    for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
        PrintDocument(document);
    }
    cout << "Even ids:"s << endl;
    // параллельная версия
    for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
        PrintDocument(document);
    }
    return 0;
}