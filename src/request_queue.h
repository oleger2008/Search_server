#pragma once

#include <deque>
#include <string>
#include <vector>

#include "search_server.h"

class RequestQueue {
public:
	RequestQueue(const SearchServer& search_server);

	template <typename DocumentPredicate>
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
	std::vector<Document> AddFindRequest(const std::string& raw_query, DocumentStatus status);
	std::vector<Document> AddFindRequest(const std::string& raw_query);

	int GetNoResultRequests() const;

private:
	struct QueryResult {
		std::string raw_query;
		size_t found_docs_amount;
	};
	std::deque<QueryResult> requests_;
	const static int sec_in_day_ = 1440; // это кол-во минут в дне
	int seconds = 0;
	int no_result_requests_ = 0;
	const SearchServer& search_server_;
};

template <typename DocumentPredicate>
std::vector<Document> RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
	++seconds;
	const auto& result = RequestQueue::search_server_.FindTopDocuments(raw_query, document_predicate);
	if (result.empty()) {
		++no_result_requests_;
	}
	requests_.push_back(QueryResult{raw_query, result.size()});
	if (requests_.size() > sec_in_day_) {
		if (requests_.front().found_docs_amount == 0){
			--no_result_requests_;
		}
		requests_.pop_front();
	}
	return result;

}
