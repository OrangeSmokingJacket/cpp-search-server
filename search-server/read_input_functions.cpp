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
ostream& operator<< (ostream& out, const Paginator::Page& page)
{
    for (int i = 0; i < page.pageSize; i++)
    {
        out << page.content[i];
    }
    return out;
} // Short way to output document data (whole page)