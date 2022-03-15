#pragma once

#include <iostream>

struct Document {

	Document() = default;

	Document(int id, double relevance, int rating);

	int id = 0;
	double relevance = 0.0;
	int rating = 0;
};

template <typename Stream>
Stream& operator<<(Stream& out, const Document& document) {

	using namespace std;

	out << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s;
	return out;
}

enum class DocumentStatus {
	ACTUAL,
	IRRELEVANT,
	BANNED,
	REMOVED,
};
