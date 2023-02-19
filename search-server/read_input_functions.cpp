#include "read_input_functions.h"

using namespace std;

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

void PrintDocument(const Document& document)
{
    cout << "{ document_id = "s << document.id << ", relevance = "s << document.relevance << ", rating = "s << document.rating << " }"s << endl;
} // Specified version of simple document output
ostream& operator<< (ostream& out, const Document& document)
{
    out << "{ document_id = "s << document.id << ", relevance = "s << document.relevance << ", rating = "s << document.rating << " }"s;

    return out;
} // Short way to output document data
ostream& operator<< (ostream& out, const Paginator::Page& page)
{
    for (int i = 0; i < page.pageSize; i++)
    {
        out << page.content[i];
    }
    return out;
} // Short way to output document data (whole page)
