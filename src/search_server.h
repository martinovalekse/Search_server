#pragma once

#include "concurrent_map.h"
#include "document.h"
#include "read_input_functions.h"
#include "string_processing.h"

#include <algorithm>
#include <cmath>
#include <deque>
#include <execution>
#include <list>
#include <map>
#include <stdexcept>
#include <utility>

class SearchServer {
public:
	template <typename StringContainer>
	explicit SearchServer(const StringContainer& stop_words);
	explicit SearchServer(const std::string& stop_words_text);
	explicit SearchServer(std::string_view stop_words_text);

	void AddDocument(int document_id, std::string_view document, DocumentStatus status, const std::vector<int>& ratings);

	template <typename ExecutionPolicy, typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const;
	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const;
	template <typename ExecutionPolicy>
	std::vector<Document> FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const;

	template <typename DocumentPredicate>
	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const;
	std::vector<Document> FindTopDocuments(std::string_view raw_query, DocumentStatus status) const;
	std::vector<Document> FindTopDocuments(std::string_view raw_query) const;

	int GetDocumentCount() const;

	auto begin() const {
		return document_ids_.begin();
	}

	auto end() const {
		return document_ids_.end();
	}

	const std::map<std::string_view, double>& GetWordFrequencies(int document_id) const;

	template <typename ExecutionPolicy>
	void RemoveDocument(ExecutionPolicy&& policy, int document_id) ;
	void RemoveDocument(int document_id);

	template <typename ExecutionPolicy>
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const;
	std::tuple<std::vector<std::string_view>, DocumentStatus> MatchDocument(std::string_view raw_query, int document_id) const;

private:
	struct DocumentData {
		int rating;
		DocumentStatus status;
	};

	const std::set<std::string> stop_words_;
	std::map<std::string, std::map<int, double>, std::less<>> word_to_document_freqs_;
	std::map<int, std::map<std::string_view, double>> document_to_word_freqs_;
	std::map<int, DocumentData> documents_;
	std::set<int> document_ids_;

	struct QueryWord {
		std::string_view data;
		bool is_minus;
		bool is_stop;
	};

	struct Query {
		std::set<std::string, std::less<>> plus_words;
		std::set<std::string, std::less<>> minus_words;
	};

	Query ParseQuery(std::string_view text) const;
	QueryWord ParseQueryWord(std::string_view text) const;

	bool IsStopWord(std::string_view word) const;
	static bool IsValidWord(std::string_view word);
	std::vector<std::string_view> SplitIntoWordsNoStop(std::string_view text) const;

	static int ComputeAverageRating(const std::vector<int>& ratings);
	double ComputeWordInverseDocumentFreq(const std::string& word) const;

	template <typename ExecutionPolicy, typename DocumentPredicate>
	std::vector<Document> FindAllDocuments(ExecutionPolicy policy, const Query& query, DocumentPredicate document_predicate) const ;

};

// ----- implement template methods -----

template <typename ExecutionPolicy>
	void SearchServer::RemoveDocument(ExecutionPolicy&& policy, int document_id) {
		if (!document_ids_.count(document_id)) {
			throw std::out_of_range("invalid id");
		}
		document_ids_.erase(document_id);
		documents_.erase(document_id);
		std::for_each(policy, std::make_move_iterator(document_to_word_freqs_.at(document_id).begin()), std::make_move_iterator(document_to_word_freqs_.at(document_id).end()),
				[this, document_id](const auto& pair) {
				std::string collected_word{pair.first.begin(), pair.first.end()};
				return word_to_document_freqs_.at(collected_word).erase(document_id);
		});
		document_to_word_freqs_.erase(document_id);
	}

template <typename StringContainer>
SearchServer::SearchServer(const StringContainer& stop_words) : stop_words_(MakeUniqueNonEmptyStrings(stop_words)) {
	if (!all_of(stop_words_.begin(), stop_words_.end(), IsValidWord)) {
		using namespace std;
		throw invalid_argument("Some of stop words are invalid"s);
	}
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentPredicate document_predicate) const {
	const auto query = ParseQuery(raw_query);
	auto matched_documents = FindAllDocuments(policy, query, document_predicate);
	sort(matched_documents.begin(), matched_documents.end(), [](const Document& lhs, const Document& rhs) {
		if (std::abs(lhs.relevance - rhs.relevance) < 1e-6) {
			return lhs.rating > rhs.rating;
		} else {
			return lhs.relevance > rhs.relevance;
		}
	});
	if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
		matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
	}
	return matched_documents;
}

template <typename DocumentPredicate>
std::vector<Document> SearchServer::FindTopDocuments(std::string_view raw_query, DocumentPredicate document_predicate) const {
	return FindTopDocuments(std::execution::seq, raw_query, document_predicate);
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query, DocumentStatus status) const {
	return FindTopDocuments(policy, raw_query, [status](int document_id, DocumentStatus document_status, int rating) {
		return document_status == status;
	});
}

template <typename ExecutionPolicy>
std::vector<Document> SearchServer::FindTopDocuments(ExecutionPolicy&& policy, std::string_view raw_query) const{
	return FindTopDocuments(policy, raw_query, DocumentStatus::ACTUAL);
}

template <typename ExecutionPolicy, typename DocumentPredicate>
std::vector<Document> SearchServer::FindAllDocuments(ExecutionPolicy policy, const Query& query, DocumentPredicate document_predicate) const {
	using namespace std;
	ConcurrentMap<int, double> document_to_relevance_protect(4);
	for (const string& word : query.plus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		const double inverse_document_freq = ComputeWordInverseDocumentFreq(word);
		for_each(policy, word_to_document_freqs_.at(word).begin(), word_to_document_freqs_.at(word).end(), [this, document_predicate,  &document_to_relevance_protect, &inverse_document_freq](const auto& element){
			const auto& document_data = documents_.at(element.first);
			if (document_predicate(element.first, document_data.status, document_data.rating)) {
				document_to_relevance_protect[element.first].ref_to_value += element.second * inverse_document_freq;
			}
		});
	}
	map<int, double> document_to_relevance = move(document_to_relevance_protect.BuildOrdinaryMap());
	for (const string& word : query.minus_words) {
		if (word_to_document_freqs_.count(word) == 0) {
			continue;
		}
		for (const auto [document_id, _] : word_to_document_freqs_.at(word)) {
			document_to_relevance.erase(document_id);
		}
	}
	vector<Document> matched_documents;
	for (const auto [document_id, relevance] : document_to_relevance) {
		matched_documents.push_back({document_id, relevance, documents_.at(document_id).rating});
	}
	return matched_documents;
}

template <typename ExecutionPolicy>
std::tuple<std::vector<std::string_view>, DocumentStatus> SearchServer::MatchDocument(ExecutionPolicy&& policy, std::string_view raw_query, int document_id) const {
	if (!document_ids_.count(document_id)) {
		throw std::out_of_range("invalid id");
	}
	if (raw_query.empty()) {
		throw std::invalid_argument("empty request");
	}
	std::vector<std::string_view> matched_words;
	Query processed_query = ParseQuery(raw_query);
	std::for_each(policy, processed_query.plus_words.begin(), processed_query.plus_words.end(), [this, document_id, &matched_words](const std::string& word){
		if (word_to_document_freqs_.count(word) == 0) {
			return;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.push_back(word_to_document_freqs_.find(word)->first);
		}
	});
	std::for_each(policy, processed_query.minus_words.begin(), processed_query.minus_words.end(), [this, document_id, &matched_words](const std::string& word){
		if (word_to_document_freqs_.count(word) == 0) {
			return;
		}
		if (word_to_document_freqs_.at(word).count(document_id)) {
			matched_words.clear();
			return;
		}
	});
	return {matched_words, documents_.at(document_id).status};
}
