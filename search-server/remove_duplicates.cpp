#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server)
{
	if (search_server.GetDocumentCount() < 2)
		return;

	vector<int> removed_ids;
	set<vector<string>> unic_docs;

	for (int doc_id : search_server)
	{
		map<string, double> word_frequencies = search_server.GetWordFrequencies(doc_id);
		vector<string> unic_words;
		for (const auto& [key, value] : word_frequencies)
		{
			unic_words.push_back(key);
		}

		if (unic_docs.count(unic_words) > 0)
			removed_ids.push_back(doc_id);
		else
			unic_docs.insert(unic_words);
	}

	sort(removed_ids.begin(), removed_ids.end());

	if (removed_ids.size() != 0)
	{
		for (int doc_id : removed_ids)
		{
			search_server.RemoveDocument(doc_id);
			cout << "Found duplicate document id " << doc_id << endl;
		}
	}
}