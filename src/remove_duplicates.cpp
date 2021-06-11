#include <iostream>
#include <map>
#include <set>
#include <string>
#include <vector>

#include "remove_duplicates.h"

using namespace std;



void RemoveDuplicates(SearchServer& search_server) {
	map<set<string_view>, int> unique_words_to_document_id;
	list<int> garbage;

	for (const auto document_id : search_server) {
		const auto& word_freqs = search_server.GetWordFrequencies(document_id);
		set<string_view> unique_words;

		for (const auto& [word, freqs] : word_freqs) {
			unique_words.insert(word);
		}

		if (unique_words_to_document_id.count(unique_words)) {
			garbage.push_back(document_id);
		} else {
			unique_words_to_document_id[unique_words] = document_id;
		}
	}

	// если сервер пустой, то можно выйти
	if (unique_words_to_document_id.empty()) {
		return;
	}

	for (const auto& document_id : garbage) {
		cout << "Found duplicate document id " << document_id << endl;
		search_server.RemoveDocument(document_id);
	}
}
