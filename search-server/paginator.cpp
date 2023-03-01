#include "paginator.h"

using namespace std;

ostream& operator<< (ostream& out, const Paginator::Page& page)
{
    for (int i = 0; i < page.pageSize; i++)
    {
        out << page.content[i];
    }
    return out;
} // Short way to output document data (whole page)