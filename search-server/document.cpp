#include "document.h"

using namespace std;

void PrintDocument(const Document& document)
{
    cout << "{ document_id = "s << document.id << ", relevance = "s << document.relevance << ", rating = "s << document.rating << " }"s << endl;
} // Specified version of simple document output
ostream& operator<< (ostream& out, const Document& document)
{
    out << "{ document_id = "s << document.id << ", relevance = "s << document.relevance << ", rating = "s << document.rating << " }"s;

    return out;
} // Short way to output document data