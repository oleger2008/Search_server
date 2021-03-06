#include <iostream>
#include <random>
#include <string>
#include <vector>

#include "log_duration.h"
#include "paginator.h"
#include "print_functions.h"
#include "process_queries.h"
#include "read_input_functions.h"
#include "remove_duplicates.h"
#include "request_queue.h"
#include "test_example_functions.h"
#include "search_server.h"



using namespace std;

string GenerateWord(mt19937& generator, int max_length) {
    const int length = uniform_int_distribution(1, max_length)(generator);
    string word;
    word.reserve(length);
    for (int i = 0; i < length; ++i) {
        word.push_back(uniform_int_distribution('a', 'z')(generator));
    }
    return word;
}

vector<string> GenerateDictionary(mt19937& generator, int word_count, int max_length) {
    vector<string> words;
    words.reserve(word_count);
    for (int i = 0; i < word_count; ++i) {
        words.push_back(GenerateWord(generator, max_length));
    }
    sort(words.begin(), words.end());
    words.erase(unique(words.begin(), words.end()), words.end());
    return words;
}

string GenerateQuery(mt19937& generator, const vector<string>& dictionary, int word_count, double minus_prob = 0) {
    string query;
    for (int i = 0; i < word_count; ++i) {
        if (!query.empty()) {
            query.push_back(' ');
        }
        if (uniform_real_distribution<>(0, 1)(generator) < minus_prob) {
            query.push_back('-');
        }
        query += dictionary[uniform_int_distribution<int>(0, dictionary.size() - 1)(generator)];
    }
    return query;
}

vector<string> GenerateQueries(mt19937& generator, const vector<string>& dictionary, int query_count, int max_word_count) {
    vector<string> queries;
    queries.reserve(query_count);
    for (int i = 0; i < query_count; ++i) {
        queries.push_back(GenerateQuery(generator, dictionary, max_word_count));
    }
    return queries;
}

template <typename QueriesProcessor>
void TestProcessQueries(string_view mark, QueriesProcessor processor, const SearchServer& search_server, const vector<string>& queries) {
	LOG_DURATION(mark);
	const auto documents = processor(search_server, queries);
	cout << documents.size() << endl;
}

#define TEST_PROCESS_QUERIES(processor) TestProcessQueries(#processor, processor, search_server, queries)

template <typename ExecutionPolicy>
void TestRemoveDocument(string_view mark, SearchServer search_server, ExecutionPolicy&& policy) {
	LOG_DURATION(mark);
	const int document_count = search_server.GetDocumentCount();
	for (int id = 0; id < document_count; ++id) {
		search_server.RemoveDocument(policy, id);
	}
	cout << search_server.GetDocumentCount() << endl;
}
#define TEST_REMOVE_DOCUMENT(mode) TestRemoveDocument(#mode, search_server, execution::mode)

template <typename ExecutionPolicy>
void TestMatchDocument(string_view mark, SearchServer search_server, const string& query, ExecutionPolicy&& policy) {
	LOG_DURATION(mark);
	const int document_count = search_server.GetDocumentCount();
	int word_count = 0;
	for (int id = 0; id < document_count; ++id) {
		const auto [words, status] = search_server.MatchDocument(policy, query, id);
		word_count += words.size();
	}
	cout << word_count << endl;
}

#define TEST_MATCH_DOCUMENTS(policy) TestMatchDocument(#policy, search_server, query, execution::policy)

template <typename ExecutionPolicy>
void TestFindTopDocuments(string_view mark, const SearchServer& search_server, const vector<string>& queries, ExecutionPolicy&& policy) {
	LOG_DURATION(mark);
	double total_relevance = 0;
	for (const string_view query : queries) {
		for (const auto& document : search_server.FindTopDocuments(policy, query)) {
			total_relevance += document.relevance;
		}
	}
	cout << total_relevance << endl;
}

#define TEST_FIND_TOP_DOCUMENTS(policy) TestFindTopDocuments(#policy, search_server, queries, execution::policy)

int main() {
	TestSearchServer();

	{
		mt19937 generator;
		const auto dictionary = GenerateDictionary(generator, 10000, 25);
		const auto documents = GenerateQueries(generator, dictionary, 100'000, 10);

		SearchServer search_server(dictionary[0]);
		for (size_t i = 0; i < documents.size(); ++i) {
			search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
		}

		const auto queries = GenerateQueries(generator, dictionary, 10'000, 7);
		TEST_PROCESS_QUERIES(ProcessQueries);
	}

	{
		mt19937 generator;

		const auto dictionary = GenerateDictionary(generator, 10000, 25);
		const auto documents = GenerateQueries(generator, dictionary, 10'000, 100);

		SearchServer search_server(dictionary[0]);
		for (size_t i = 0; i < documents.size(); ++i) {
			search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
		}

		TEST_REMOVE_DOCUMENT(seq);
		TEST_REMOVE_DOCUMENT(par);
	}

	{
		mt19937 generator;

		const auto dictionary = GenerateDictionary(generator, 1000, 10);
		const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

		const string query = GenerateQuery(generator, dictionary, 500, 0.1);

		SearchServer search_server(dictionary[0]);
		for (size_t i = 0; i < documents.size(); ++i) {
			search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
		}

		TEST_MATCH_DOCUMENTS(seq);
		TEST_MATCH_DOCUMENTS(par);
	}

	{
		mt19937 generator;

		const auto dictionary = GenerateDictionary(generator, 1000, 10);
		const auto documents = GenerateQueries(generator, dictionary, 10'000, 70);

		SearchServer search_server(dictionary[0]);
		for (size_t i = 0; i < documents.size(); ++i) {
			search_server.AddDocument(i, documents[i], DocumentStatus::ACTUAL, {1, 2, 3});
		}

		const auto queries = GenerateQueries(generator, dictionary, 100, 70);

		TEST_FIND_TOP_DOCUMENTS(seq);
		TEST_FIND_TOP_DOCUMENTS(par);
	}

	cout << "Done" << endl;
	return 0;
}
