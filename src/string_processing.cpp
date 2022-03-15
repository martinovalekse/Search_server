#include "string_processing.h"

using namespace std;

std::vector<std::string_view> SplitIntoWords(std::string_view str) {
	std::vector<std::string_view> result;
	const int64_t pos_end = str.npos;
	while (true) {
		int64_t space = str.find(' ');
		string_view thing = (space == pos_end ? str.substr() : str.substr(0, space));
		result.push_back(thing);
		if (space == pos_end) {
			break;
		} else {
			str.remove_prefix(space + 1);
		}
	}
	return result;
}
