#include "request_queue.h"

using namespace std;

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status)
{
    vector<Document> result = search_server_reference.FindTopDocuments(raw_query, status);

    requests.push_back(Paginate(result, PAGE_LENGTH));
    if (requests.size() > min_in_day)
        requests.pop_front();

    return result;
}
vector<Document> RequestQueue::AddFindRequest(const string& raw_query)
{
    vector<Document> result = search_server_reference.FindTopDocuments(raw_query);

    requests.push_back(Paginate(result, PAGE_LENGTH));
    if (requests.size() > min_in_day)
        requests.pop_front();

    return result;
}

int RequestQueue::GetNoResultRequests() const
{
    int result = 0;
    for (Paginator q : requests)
    {
        if (q.size() == 0)
            result++;
    }
    return result;
}