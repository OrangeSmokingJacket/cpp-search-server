#pragma once

#include <map>
#include <vector>
#include <string>

#include "search_server.h"
using namespace std;

void RemoveDuplicates(SearchServer& search_server)
{
	if (search_server.GetDocumentCount() < 2)
		return;

	vector<int> removed_ids;

	// There will be a bunch of wierd things...
	for (auto i = search_server.begin(); i != search_server.end() - 1 && i != search_server.end(); i++) // in case of last and second-to-last docs, later could be removed, so thats why there is 2 != statements (and i guess we couldn't use < here)
	{
		for (auto j = i + 1; j != search_server.end(); j++)
		{
			if (!search_server.CompareDocs(*i, *j))
				continue;

			// The may we add new document, forces them to have different id's
			// In case that still happened, latest one (higher index in vector) will be delete

			// Can't remove document and then decriment iterator back (because there will be no such iterator)
			// And we are not decrementing it, documents will be skiped
			j--;
			if (*i > *(j + 1))
			{
				removed_ids.push_back(*i);
				search_server.RemoveDocument(*i);
			}
			else
			{
				removed_ids.push_back(*(j + 1));
				search_server.RemoveDocument(*(j + 1));
			}
		}
	}
	sort(removed_ids.begin(), removed_ids.end());
	if (removed_ids.size() != 0)
	{
		for (int id : removed_ids)
		{
			cout << "Found duplicate document id " << id << endl;
		}
	}
}