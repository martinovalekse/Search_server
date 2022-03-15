#include "process_queries.h"

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::vector<std::vector<Document>> result(queries.size());
	std::transform( std::make_move_iterator(queries.begin()), std::make_move_iterator(queries.end()), result.begin(),
			[&search_server](const std::string& words){ return search_server.FindTopDocuments(words); });
	return result;
}

//std::execution::par,

std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries) {
	std::list<Document> result;
	for (const std::vector<Document>& items : ProcessQueries(search_server, queries)) {
		std::transform(std::make_move_iterator(items.begin()), std::make_move_iterator(items.end()), std::back_inserter(result),
				[](auto &document){ return std::move(document); });
	}
	return result;
}
