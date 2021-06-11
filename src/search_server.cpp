#include <cmath>
#include <execution>
#include <algorithm>
#include <exception>
#include <execution>
#include <list>
#include <map>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <utility>

#include "search_server.h"

using namespace std;

SearchServer::SearchServer(string_view stop_words_text)
: SearchServer(SplitIntoWordsView(stop_words_text))
{
}

SearchServer::SearchServer(const string& stop_words_text)
: SearchServer(SplitIntoWordsView(stop_words_text))
{
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
	if ((document_id < 0) || (documents_.count(document_id) > 0)) {
		throw invalid_argument("Invalid document_id"s);
	}
	document_ids_.push_back(document_id);

	const auto& [document_id_to_data, _] = documents_.emplace(document_id,
			SearchServer::DocumentData{string(document), ComputeAverageRating(ratings), status});
	const auto words = SplitIntoWordsNoStop(document_id_to_data->second.document_data);

	const double inv_word_count = 1.0 / words.size();
	for (const string_view word : words) {
		word_to_document_freqs_[string(word)][document_id] += inv_word_count;
		document_to_word_freqs_[document_id][word] += inv_word_count;
	}

}
vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(execution::seq, raw_query, status);
}
vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
	return FindTopDocuments(execution::seq, raw_query);
}

vector<Document> SearchServer::FindTopDocuments(const execution::sequenced_policy&, string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(execution::seq, raw_query, [status]([[maybe_unused]] int document_id,
			DocumentStatus document_status, [[maybe_unused]] int rating) {
		return document_status == status;
	});
}
vector<Document> SearchServer::FindTopDocuments(const execution::sequenced_policy&, string_view raw_query) const {
	return FindTopDocuments(execution::seq, raw_query, DocumentStatus::ACTUAL);
}

vector<Document> SearchServer::FindTopDocuments(const execution::parallel_policy&, string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(execution::par, raw_query, [status]([[maybe_unused]] int document_id,
			DocumentStatus document_status, [[maybe_unused]] int rating) {
		return document_status == status;
	});
}
vector<Document> SearchServer::FindTopDocuments(const execution::parallel_policy&, string_view raw_query) const {
	return FindTopDocuments(execution::par, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
	return documents_.size();
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
	static const map<string_view, double> empty_map;

	if (!document_to_word_freqs_.count(document_id)) {
		return empty_map;
	}

	return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
	SearchServer::RemoveDocument(execution::seq, document_id);
}

void SearchServer::RemoveDocument(const execution::sequenced_policy&, int document_id) {
	// если пытаемся удалить ID, который не добавляли на сервер
	if (!count(document_ids_.begin(), document_ids_.end(), document_id) ) {
		return;
	}

	for (auto& [word, freqs] : document_to_word_freqs_.at(document_id)) {
		auto& document_freqs = word_to_document_freqs_.at(string(word));
		document_freqs.erase(document_id);
	}

	document_to_word_freqs_.erase(document_id);
	documents_.erase(document_id);
	document_ids_.remove(document_id);
}

void SearchServer::RemoveDocument(const execution::parallel_policy&, int document_id) {
	// если пытаемся удалить ID, который не добавляли на сервер
	if (!count(document_ids_.begin(), document_ids_.end(), document_id) ) {
		return;
	}

	const auto& word_freqs = document_to_word_freqs_.at(document_id);
	vector<string_view> words(word_freqs.size());
	transform(
			execution::par,
			word_freqs.begin(), word_freqs.end(),
			words.begin(),
			[](const auto& item) { return item.first; }
			);
	for_each(
			execution::par,
			words.begin(), words.end(),
			[this, document_id] (string_view word) {
					word_to_document_freqs_.at(string(word)).erase(document_id);
				}
			);

	document_to_word_freqs_.erase(document_id);
	documents_.erase(document_id);
	document_ids_.remove(document_id);
}

tuple<vector<string_view>, DocumentStatus>
SearchServer::MatchDocument(string_view raw_query, int document_id) const {
	return SearchServer::MatchDocument(execution::seq, raw_query, document_id);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
		const execution::sequenced_policy&, string_view raw_query, int document_id) const {
	if (!count(document_ids_.begin(), document_ids_.end(), document_id) ) {
		throw out_of_range("No documents with id " + document_id);
	}
	auto& status = documents_.at(document_id).status;
	const auto query = ParseQuery(raw_query);

	vector<string_view> matched_words;
	for (const string_view word_view : query.minus_words) {
		string word(word_view);
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			return make_tuple(matched_words, status);
		}
	}
	for (const string_view word_view : query.plus_words) {
		string word(word_view);
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word_view);
		}
	}
	return make_tuple(matched_words, status);
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(
		const execution::parallel_policy&, string_view raw_query, int document_id) const {
	if (!count(document_ids_.begin(), document_ids_.end(), document_id) ) {
		throw out_of_range("No documents with id " + document_id);
	}
	auto& status = documents_.at(document_id).status;
	const auto query = ParseQuery(raw_query);

	vector<string_view> matched_words;

	const auto word_checker = [this, document_id] (string_view word_view) {
		string word(word_view);
		const auto& item = word_to_document_freqs_.find(word);
		return item != word_to_document_freqs_.end() && item->second.count(document_id);
	};

	if (any_of(query.minus_words.begin(), query.minus_words.end(), word_checker)) {
		return make_tuple(matched_words, status);
	}

	matched_words.resize(query.plus_words.size());
	auto matched_words_end = copy_if(
			execution::par,
			query.plus_words.begin(), query.plus_words.end(),
			matched_words.begin(),
			word_checker
			);
	matched_words.erase(matched_words_end, matched_words.end());

	return make_tuple(matched_words, status);
}

bool SearchServer::IsStopWord(string_view word) const {
	return stop_words_.count(word) > 0;
}

bool SearchServer::IsValidWord(string_view word) const {
	// A valid word must not contain special characters
	return none_of(word.begin(), word.end(), [](char c) {
		return c >= '\0' && c < ' ';
	});
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
	vector<string_view> words;
	for (const string_view word : SplitIntoWordsView(text)) {
		if (!IsValidWord(word)) {
			throw invalid_argument("Word "s + string(word) + " is invalid"s);
		}
		if (!IsStopWord(word)) {
			words.push_back(word);
		}
	}
	return words;
}

int SearchServer::ComputeAverageRating(const vector<int>& ratings) {
	if (ratings.empty()) {
		return 0;
	}
	int rating_sum = 0;
	for (const int rating : ratings) {
		rating_sum += rating;
	}
	return rating_sum / static_cast<int>(ratings.size());
}

SearchServer::QueryWord SearchServer::ParseQueryWord(string_view text) const {
	if (text.empty()) {
		throw invalid_argument("Query word is empty"s);
	}

	bool is_minus = false;
	if (text[0] == '-') {
		is_minus = true;
		text.remove_prefix(1);
	}
	if (text.empty() || text[0] == '-' || !IsValidWord(text)) {
		throw invalid_argument("Query word "s + string(text) + " is invalid");
	}

	return {text, is_minus, IsStopWord(text)};
}

SearchServer::Query SearchServer::ParseQuery(string_view text) const {
	SearchServer::Query result;
	for (string_view word : SplitIntoWordsView(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				result.minus_words.emplace(query_word.data);
			} else {
				result.plus_words.emplace(query_word.data);
			}
		}
	}
	return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}

