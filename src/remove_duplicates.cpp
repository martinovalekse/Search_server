#include "remove_duplicates.h"

using namespace std;

void RemoveDuplicates(SearchServer& search_server) {
	set<int> ids_for_deleting;
	set<set<string>> words_unique_sets;
	for (const int document_id : search_server) {
		set<string> tmp_words;
		for (const auto& [word, freq] : search_server.GetWordFrequencies(document_id)) {
			tmp_words.emplace(word.begin(), word.end());
		}
		if (words_unique_sets.count(tmp_words) == 1) {
			ids_for_deleting.insert(document_id);
		} else {
			words_unique_sets.insert(tmp_words);
		}
	}
	if (!ids_for_deleting.empty()) {
		for (auto id : ids_for_deleting) {
			cout << "Found duplicate document id "s << id << endl;
			search_server.RemoveDocument(id);
		}
	}
}
