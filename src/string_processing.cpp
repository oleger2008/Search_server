#include <iostream>
#include <string_view>
#include <vector>

#include "string_processing.h"

using namespace std;

vector<string_view> SplitIntoWordsView(string_view str) {
	vector<string_view> result;

	const int64_t pos_end = str.npos;

	while (true) {
		int64_t space = str.find(' ');

		result.push_back(space == pos_end ? str.substr(0) : str.substr(0, space));

		if (space == pos_end) {
			break;
		} else {
			str.remove_prefix(space + 1);
		}
	}

	return result;
}

