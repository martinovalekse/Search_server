#include "search_server.h"

using namespace std;

SearchServer::SearchServer(const string& stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {
}

SearchServer::SearchServer(string_view stop_words_text) : SearchServer(SplitIntoWords(stop_words_text)) {
}

void SearchServer::AddDocument(int document_id, string_view document, DocumentStatus status, const vector<int>& ratings) {
	if ((document_id < 0) || (documents_.count(document_id) > 0)) {
		throw invalid_argument("Invalid document_id"s);
	}
	const auto words = SplitIntoWordsNoStop(document);
	const double inv_word_count = 1.0 / words.size();
	for (string_view word : words) {
		string word_collected{word.begin(), word.end()};
		word_to_document_freqs_[word_collected][document_id] += inv_word_count;
		document_to_word_freqs_[document_id][word_to_document_freqs_.find(word_collected)->first] += inv_word_count;
	}
	documents_.emplace(document_id, DocumentData{ComputeAverageRating(ratings), status});
	document_ids_.insert(document_id);
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(std::execution::seq, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
	});
}

vector<Document> SearchServer::FindTopDocuments(string_view raw_query) const {
	return FindTopDocuments(std::execution::seq, raw_query, DocumentStatus::ACTUAL);
}

int SearchServer::GetDocumentCount() const {
	return documents_.size();
}

tuple<vector<string_view>, DocumentStatus> SearchServer::MatchDocument(string_view raw_query, int document_id) const {
	return MatchDocument(execution::seq, raw_query, document_id);

	//independent implementation of this function

	/*if (!document_ids_.count(document_id)) {
		throw std::out_of_range("invalid id");
	}
	if (raw_query.empty()) {
		throw std::invalid_argument("empty request");
	}
	vector<string_view> matched_words;
	Query processed_query = ParseQuery(raw_query);
	for (const string& word : processed_query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word_to_document_freqs_.find(word)->first);
		}
	}
	for (const string& word : processed_query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.clear();
			break;
		}
	}
	return {matched_words, documents_.at(document_id).status};*/
}

const map<string_view, double>& SearchServer::GetWordFrequencies(int document_id) const {
	return document_to_word_freqs_.at(document_id);
}

void SearchServer::RemoveDocument(int document_id) {
	RemoveDocument(std::execution::seq, document_id);
}

//   -----------------------private-----------------------

bool SearchServer::IsStopWord(string_view word) const {
	return stop_words_.count({word.begin(), word.end()}) > 0;
}

bool SearchServer::IsValidWord(string_view word) {
	return none_of(word.begin(), word.end(), [](auto c) { return c >= '\0' && c < ' '; });
}

vector<string_view> SearchServer::SplitIntoWordsNoStop(string_view text) const {
	vector<string_view> words;
	for (string_view word : SplitIntoWords(text)) {
		if (!IsValidWord(word)) {
			throw invalid_argument("Word "s + (word.begin(), word.end()) + " is invalid"s);
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

SearchServer::QueryWord SearchServer::ParseQueryWord(std::string_view text) const {
	if (text.empty()) {
		throw invalid_argument("Query word is empty"s);
	}
	string_view word = text;
	bool is_minus = false;
	if (word[0] == '-') {
		is_minus = true;
		word = word.substr(1);
	}
	if (word.empty() || word[0] == '-' || !IsValidWord(word)) {
		string collected_text{text.begin(), text.end()};
		throw invalid_argument("Query word "s + collected_text + " is invalid");
	}
	return {word, is_minus, IsStopWord(word)};
}

SearchServer::Query SearchServer::ParseQuery(std::string_view text) const {
	Query result;
	for (string_view word : SplitIntoWords(text)) {
		const auto query_word = ParseQueryWord(word);
		if (!query_word.is_stop) {
			if (query_word.is_minus) {
				string collected_word{query_word.data.begin(), query_word.data.end()};
				result.minus_words.insert(move(collected_word));
			} else {
				string collected_word{query_word.data.begin(), query_word.data.end()};
				result.plus_words.insert(move(collected_word));
			}
		}
	 }
	return result;
}

double SearchServer::ComputeWordInverseDocumentFreq(const string& word) const {
	return log(GetDocumentCount() * 1.0 / word_to_document_freqs_.at(word).size());
}
