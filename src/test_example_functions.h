#pragma once

#include <execution>

#include "search_server.h"

void PrintDocument(const Document& document) {
	using namespace std;
	cout << "{ "s
		<< "document_id = "s << document.id << ", "s
		<< "relevance = "s << document.relevance << ", "s
		<< "rating = "s << document.rating << " }"s << endl;
}

template <typename TIP_key, typename TIP_value>
std::ostream& operator<<(std::ostream& out, const std::pair<TIP_key, TIP_value>& container) {
	using namespace std;
	bool temp = false;
	if (temp) {
		out << container.first << ": "s <<  container.second << ", "s ;
	} else {
		out << container.first << ": "s <<  container.second;
		temp = true;
	}
	return out;
}

template <typename Potok, typename Container>
void Print(const Potok&, const Container& container) {
	using namespace std;
	bool temp = false;
	for (const auto& chast : container) {
		if (temp) {
			cout << ", "s << chast ;
		} else {
			cout << chast ;
			temp = true;
		}
	}
}

template <typename TIP>
std::ostream& operator<<(std::ostream& out, const std::vector<TIP>& container) {
	using namespace std;
	out << "["s;
	Print(out, container);
	out << "]"s;
	return out;
}

template <typename TIP>
std::ostream& operator<<(std::ostream& out, const std::set<TIP>& container) {
	using namespace std;
	out << "{"s;
	Print(out, container);
	out << "}"s;
	return out;
}

template <typename TIP_key, typename TIP_value>
std::ostream& operator<<(std::ostream& out, const std::map<TIP_key, TIP_value>& container) {
	using namespace std;
	out << "{"s;
	Print(out, container);
	out << "}"s;
	return out;
}

template <typename T, typename U>
void AssertEqualImpl(const T& t, const U& u, const std::string& t_str, const std::string& u_str, const std::string& file,
					const std::string& func, unsigned line, const std::string& hint) {
	if (t != u) {
		using namespace std;
		cout << boolalpha;
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT_EQUAL("s << t_str << ", "s << u_str << ") failed: "s;
		cout << t << " != "s << u << "."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

#define ASSERT_EQUAL(a, b) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_EQUAL_HINT(a, b, hint) AssertEqualImpl((a), (b), #a, #b, __FILE__, __FUNCTION__, __LINE__, (hint))

void AssertImpl(bool value, const std::string& expr_str, const std::string& file, const std::string& func, unsigned line,
				const std::string& hint) {
	if (!value) {
		using namespace std;
		cout << file << "("s << line << "): "s << func << ": "s;
		cout << "ASSERT("s << expr_str << ") failed."s;
		if (!hint.empty()) {
			cout << " Hint: "s << hint;
		}
		cout << endl;
		abort();
	}
}

#define ASSERT(expr) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, ""s)
#define ASSERT_HINT(expr, hint) AssertImpl(!!(expr), #expr, __FILE__, __FUNCTION__, __LINE__, (hint))

void TestExcludeStopWordsFromAddedDocumentContent() {
	using namespace std;
	const int doc_id = 42;
	const string content = "cat in the city"s;
	const vector<int> ratings = {1, 2, 3};
	{
	SearchServer server(""s);
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);
	const auto found_docs = server.FindTopDocuments("in"s);

	ASSERT_EQUAL(found_docs.size(), 1u);
	const Document& doc0 = found_docs[0];
	ASSERT_EQUAL(doc0.id, doc_id);
	}

	{
	SearchServer server("in the"s);
	server.AddDocument(doc_id, content, DocumentStatus::ACTUAL, ratings);

	ASSERT_HINT(server.FindTopDocuments("in"s).empty(), "Stop words must be excluded from documents"s);
	}
}

void TestAddDocument() {
	using namespace std;
	SearchServer server(""s);
	server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
	server.AddDocument(2, "cat"s, DocumentStatus::ACTUAL, {3});

	ASSERT_EQUAL(static_cast<size_t>(server.GetDocumentCount()), 2u);
}

void TestExcludeMinusWordFromFindTopDocuments() {
	using namespace std;
	SearchServer server(""s);
	server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});

	ASSERT_EQUAL(server.FindTopDocuments("city cat"s).size(),1u);
	ASSERT_EQUAL(server.FindTopDocuments("city -cat"s).size(), 0u);
}

void TestMatchDocument() {
	using namespace std;
	SearchServer server(""s);
	server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});

	string a("cat"s);
	string b("city"s);
	vector<string_view> test = {a, b};

	ASSERT_EQUAL(get<vector<string_view>>(server.MatchDocument("city cat slova_net"s, 1)), test);
	ASSERT(get<vector<string_view>>(server.MatchDocument("city -cat slova_net"s, 1)).empty());
	ASSERT(get<DocumentStatus>(server.MatchDocument("city cat slova_net"s, 1)) == DocumentStatus::ACTUAL);
}

void TestRelevanceSorting() {
	using namespace std;
	SearchServer server(""s);
	server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
	server.AddDocument(2, "sun cat"s, DocumentStatus::ACTUAL, {3});
	server.AddDocument(3, "dog hat"s, DocumentStatus::ACTUAL, {5, 2});
	server.AddDocument(4, "bad cat walk"s, DocumentStatus::ACTUAL, {6, 1});
	server.AddDocument(5, "cat war"s, DocumentStatus::ACTUAL, {7});

	for (size_t i=0; i + 1 < server.FindTopDocuments("cat"s).size(); ++i) {
		if (!(abs(server.FindTopDocuments("cat"s)[i].relevance - server.FindTopDocuments("cat"s)[i + 1].relevance) < 1e-6)) {
			ASSERT(server.FindTopDocuments("cat"s)[i].relevance > server.FindTopDocuments("cat"s)[i + 1].relevance);
		}
	}
}

void TestRatingCalculation() {
	using namespace std;
	SearchServer server(""s);
	server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});

	ASSERT_EQUAL(server.FindTopDocuments("cat"s)[0].rating, ((1+2+3)/3));
}

void  TestPredicate() {
	using namespace std;
	SearchServer server(""s);
	server.AddDocument(1, "cat in the city"s, DocumentStatus::BANNED, {1, 2, 3});
	server.AddDocument(2, "sun cat"s, DocumentStatus::ACTUAL, {3});
	server.AddDocument(3, "bad cat walk"s, DocumentStatus::ACTUAL, {6, 1});

	ASSERT_EQUAL((server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::ACTUAL; }).size()), 2u);
	ASSERT_EQUAL((server.FindTopDocuments("cat"s, [](int document_id, DocumentStatus status, int rating) { return status == DocumentStatus::BANNED; }).size()), 1u);
}

void TestStatus() {
	using namespace std;
	SearchServer server(""s);
	server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});
	server.AddDocument(2, "cat"s, DocumentStatus::BANNED, {3});

	ASSERT_EQUAL(server.FindTopDocuments("cat"s, DocumentStatus::ACTUAL).size(), 1u);
	ASSERT_EQUAL(server.FindTopDocuments("cat"s, DocumentStatus::BANNED).size(), 1u);
	ASSERT_EQUAL(server.FindTopDocuments("cat"s, DocumentStatus::IRRELEVANT).size(), 0u);
}

void TestRelevantCalculated() {
	using namespace std;
	SearchServer server(""s);
	server.AddDocument(1, "cat in the city"s, DocumentStatus::ACTUAL, {1, 2, 3});

	for (auto& doc : server.FindTopDocuments("cat"s)) {
		ASSERT_EQUAL(doc.relevance, (log(1) * 0.25));
	}
}

void TestRemoveDuplicates() {
	using namespace std;
	cout << endl;
	cout << "RemoveDuplicates test"s << endl;
	cout << "-------------------------------" << endl;
	SearchServer remove_server("and with"s);

	remove_server.AddDocument(1, "funny pet and nasty rat"s, DocumentStatus::ACTUAL, {7, 2, 7});
	remove_server.AddDocument( 2, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	// дубликат документа 2, будет удалён
	remove_server.AddDocument(3, "funny pet with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	// отличие только в стоп-словах, считаем дубликатом
	remove_server.AddDocument( 4, "funny pet and curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	// множество слов такое же, считаем дубликатом документа 1
	remove_server.AddDocument( 5, "funny funny pet and nasty nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

	// добавились новые слова, дубликатом не является
	remove_server.AddDocument( 6, "funny pet and not very nasty rat"s, DocumentStatus::ACTUAL, {1, 2});

	// множество слов такое же, как в id 6, несмотря на другой порядок, считаем дубликатом
	remove_server.AddDocument( 7, "very nasty rat and not very funny pet"s, DocumentStatus::ACTUAL, {1, 2});

	// есть не все слова, не является дубликатом
	remove_server.AddDocument( 8, "pet with rat and rat and rat"s, DocumentStatus::ACTUAL, {1, 2});

	// слова из разных документов, не является дубликатом
	remove_server.AddDocument( 9, "nasty rat with curly hair"s, DocumentStatus::ACTUAL, {1, 2});

	cout << "Before duplicates removed: "s << remove_server.GetDocumentCount() << endl;
	RemoveDuplicates(remove_server);
	cout << "After duplicates removed: "s << remove_server.GetDocumentCount() << endl;
	cout << "-------------------------------" << endl;

}

void TestExecutionPolicy() {
	using namespace std;
	cout << "ExecutionPolicy test"s << endl;
	cout << "-------------------------------" << endl;

	SearchServer search_server("and with"s);
	int id = 0;
	for (
		const string& text : {
		"white cat and yellow hat"s,
		"curly cat curly tail"s,
		"nasty dog with big eyes"s,
		"nasty pigeon john"s,
		}
	) {
		search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
	}

	cout << "ACTUAL by default:"s << endl;
		// последовательная версия
	for (const Document& document : search_server.FindTopDocuments("curly nasty cat"s)) {
		PrintDocument(document);
	}
	cout << "BANNED:"s << endl;
		// последовательная версия
	for (const Document& document : search_server.FindTopDocuments(execution::seq, "curly nasty cat"s, DocumentStatus::BANNED)) {
		PrintDocument(document);
	}

	cout << "Even ids:"s << endl;
		// параллельная версия
	for (const Document& document : search_server.FindTopDocuments(execution::par, "curly nasty cat"s, [](int document_id, DocumentStatus status, int rating) { return document_id % 2 == 0; })) {
		PrintDocument(document);
	}
}

void TestParallelRequests() {
	using namespace std;
	std::cout << "ParallelRequests tests" << std::endl;
	SearchServer search_server("and with"s);
	int id = 0;
	for (
		const string& text : {
			"funny pet and nasty rat"s,
			"funny pet with curly hair"s,
			"funny pet and not very nasty rat"s,
			"pet with rat and rat and rat"s,
			"nasty rat with curly hair"s,
		}
	) {
		search_server.AddDocument(++id, text, DocumentStatus::ACTUAL, {1, 2});
	}

	const vector<string> queries = {
		"nasty rat -not"s,
		"not very funny nasty pet"s,
		"curly hair"s
	};
	for (const Document& document : ProcessQueriesJoined(search_server, queries)) {
		cout << "Document "s << document.id << " matched with relevance "s << document.relevance << endl;
	}
	std::cout << "-------------------------------" << std::endl;
}

void TestSearchServer() {
	std::cout << "BasicOperations tests" << std::endl;
	std::cout << "-------------------------------" << std::endl;
	TestExcludeStopWordsFromAddedDocumentContent();
	TestAddDocument();
	TestExcludeMinusWordFromFindTopDocuments();
	TestRelevanceSorting();
	TestRatingCalculation();
	TestPredicate();
	TestStatus();
	TestRelevantCalculated();
	TestMatchDocument();
	std::cout << "Done." << std::endl;
	TestExecutionPolicy();
	TestRemoveDuplicates();
	TestParallelRequests();
	std::cout << "All Tests passed!" << std::endl;
	std::cout << "-------------------------------" << std::endl;
}
