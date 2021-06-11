#include <algorithm>
#include <functional>
#include <execution>
#include <vector>
#include <utility>

#include "process_queries.h"

using namespace std;

vector<vector<Document>> ProcessQueries(
	const SearchServer& search_server,
	const vector<string>& queries) {
	vector<vector<Document>> result(queries.size());

	transform(execution::par,
			queries.cbegin(), queries.cend(),
			result.begin(),
			[&search_server](const auto& query) { return search_server.FindTopDocuments(query); }
			);

	return result;
}

vector<Document> ProcessQueriesJoined(
	const SearchServer& search_server,
	const vector<string>& queries) {
	vector<Document> result;
	for (const auto& local_documents : ProcessQueries(search_server, queries)) {
		result.insert(result.end(), local_documents.begin(), local_documents.end());
	}
	return result;
}
