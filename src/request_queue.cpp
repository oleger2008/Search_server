#include <string>
#include <vector>

#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) :
	search_server_(search_server)
{
}

vector<Document> RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
	++seconds;
	const auto& result = search_server_.FindTopDocuments(raw_query, status);
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

vector<Document> RequestQueue::AddFindRequest(const string& raw_query) {
	++seconds;
	const auto& result = search_server_.FindTopDocuments(raw_query);
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

int RequestQueue::GetNoResultRequests() const {
	return no_result_requests_;
}
