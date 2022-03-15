#pragma once

#include "search_server.h"

class RequestQueue {
public:
	explicit RequestQueue(const SearchServer& search_server);

	template <typename DocumentPredicate>
	void AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate);
	void AddFindRequest(const std::string& raw_query, DocumentStatus status);
	void AddFindRequest(const std::string& raw_query);

	int GetNoResultRequests() const;

private:
	struct QueryResult {
		int timer;
		bool empty;
		std::vector<Document> results;
	};

	std::deque<QueryResult> requests_;
	const static int sec_in_day_ = 1440;
	int timer_ = 0;
	int no_resultat_ = 0;
	const SearchServer &search_server_;
};

// ----- implement template methods -----

template <typename DocumentPredicate>
void RequestQueue::AddFindRequest(const std::string& raw_query, DocumentPredicate document_predicate) {
	const auto& search_results = search_server_.FindTopDocuments(std::execution::seq, raw_query, document_predicate);
	if (search_results.empty()) {
		++no_resultat_;
	}
	if ((!requests_.empty()) && (requests_.size() >= sec_in_day_)) {
		if(requests_.front().empty) {
			--no_resultat_;
		}
		requests_.pop_front();
		requests_.push_back({timer_, search_results.empty(), search_results});
	} else {
		requests_.push_back({timer_, search_results.empty(), search_results});
	}
}
