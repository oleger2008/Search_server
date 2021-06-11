#include <string>
#include <iostream>
#include <vector>

#include "print_functions.h"

using namespace std;



void PrintMatchDocumentResult(int document_id, const vector<string_view>& words, DocumentStatus status) {
	cout << "{ "s
		 << "document_id = "s << document_id << ", "s
		 << "status = "s << static_cast<int>(status) << ", "s
		 << "words ="s;
	for (const string_view& word : words) {
		cout << ' ' << word;
	}
	cout << "}"s << endl;
}

void AddDocument(SearchServer& search_server, int document_id, const string_view& document, DocumentStatus status,
				 const vector<int>& ratings) {
	try {
		search_server.AddDocument(document_id, document, status, ratings);
	} catch (const invalid_argument& e) {
		cout << "Ошибка добавления документа "s << document_id << ": "s << e.what() << endl;
	}
}

void FindTopDocuments(const SearchServer& search_server, const string_view& raw_query) {
	LOG_DURATION_STREAM("Operation time", cout);
	cout << "Результаты поиска по запросу: "s << raw_query << endl;
	try {
		for (const Document& document : search_server.FindTopDocuments(raw_query)) {
			cout << document << endl;
		}
	} catch (const invalid_argument& e) {
		cout << "Ошибка поиска: "s << e.what() << endl;
	}
}

void MatchDocuments(const SearchServer& search_server, const string_view& query) {
	LOG_DURATION_STREAM("Operation time", cout);
	try {
		cout << "Матчинг документов по запросу: "s << query << endl;
		for (const auto& document_id : search_server) {
			const auto [words, status] = search_server.MatchDocument(query, document_id);
			PrintMatchDocumentResult(document_id, words, status);
		}

	} catch (const invalid_argument& e) {
		cout << "Ошибка матчинга документов на запрос "s << query << ": "s << e.what() << endl;
	}
}
