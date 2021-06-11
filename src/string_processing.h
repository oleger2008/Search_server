#pragma once

#include <set>
#include <string>
#include <string_view>
#include <vector>

#include <iostream>

std::vector<std::string_view> SplitIntoWordsView(std::string_view str);

template <typename StringContainer>
std::set<std::string, std::less<>> MakeUniqueNonEmptyStrings(const StringContainer& container) {
	std::set<std::string, std::less<>> non_empty_words;
	for (const auto& word : container) {
		if (!word.empty()) {
			non_empty_words.emplace(std::string(word));
		}
	}
	return non_empty_words;
}
