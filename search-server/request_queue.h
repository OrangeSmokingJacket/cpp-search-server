#pragma once

#include <vector>
#include <queue>
#include <string>

#include "document.h"
#include "search_server.h"

const int PAGE_LENGTH = 2;

class RequestQueue
{
public:
    explicit RequestQueue(SearchServer& search_server) : search_server_reference(search_server) {}
    template <typename DocumentPredicate>
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate)
    {
        std::vector<Document> result = search_server_reference.FindTopDocuments(raw_query, document_predicate);

        requests.push_back(Paginate(result, PAGE_LENGTH));
        if (requests.size() > min_in_day)
            requests.pop_front();

        return result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status)
    {
        std::vector<Document> result = search_server_reference.FindTopDocuments(raw_query, status);

        requests.push_back(Paginate(result, PAGE_LENGTH));
        if (requests.size() > min_in_day)
            requests.pop_front();

        return result;
    }
    std::vector<Document> AddFindRequest(const std::string& raw_query)
    {
        std::vector<Document> result = search_server_reference.FindTopDocuments(raw_query);

        requests.push_back(Paginate(result, PAGE_LENGTH));
        if (requests.size() > min_in_day)
            requests.pop_front();

        return result;
    }
    int GetNoResultRequests() const
    {
        int result = 0;
        for (Paginator q : requests)
        {
            if (q.size() == 0)
                result++;
        }
        return result;
    }
private:
    SearchServer& search_server_reference;
    std::deque<Paginator> requests;
    const static int min_in_day = 1440;
};