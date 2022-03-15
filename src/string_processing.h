#pragma once

#include <set>
#include <string>
#include <vector>

std::vector<std::string_view> SplitIntoWords(std::string_view text);

template <typename StringContainer>
std::set<std::string> MakeUniqueNonEmptyStrings(const StringContainer& strings) {
	using namespace std;
	set<string> non_empty_strings;
	for (string_view str : strings) {
		if (!str.empty()) {
			string temp(str.begin(), str.end());
			non_empty_strings.insert(move(temp));
		}
	}
	return non_empty_strings;
}
