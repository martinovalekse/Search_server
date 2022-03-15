#pragma once

#include "search_server.h"

#include <execution>
#include <list>
#include <numeric>
#include <utility>

std::vector<std::vector<Document>> ProcessQueries(const SearchServer& search_server, const std::vector<std::string>& queries);
std::list<Document> ProcessQueriesJoined(const SearchServer& search_server, const std::vector<std::string>& queries);
