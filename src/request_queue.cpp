#include "request_queue.h"

using namespace std;

RequestQueue::RequestQueue(const SearchServer& search_server) : search_server_(search_server){
}

void RequestQueue::AddFindRequest(const string& raw_query, DocumentStatus status) {
	AddFindRequest(raw_query, [status](int document_id, DocumentStatus document_status, int rating) { return document_status == status; });
}

void RequestQueue::AddFindRequest(const string& raw_query) {
	AddFindRequest(raw_query, DocumentStatus::ACTUAL);
}

int RequestQueue::GetNoResultRequests() const {
	return no_resultat_;
}
