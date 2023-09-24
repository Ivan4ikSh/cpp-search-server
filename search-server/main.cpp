#include <algorithm>
#include <numeric>
#include <iostream>
#include <set>
#include <string>
#include <utility>
#include <vector>
#include <map>
#include <cmath>
#include <cassert>
#include <optional>

using namespace std;

const int MAX_RESULT_DOCUMENT_COUNT = 5;

string ReadLine() {
    string s;
    getline(cin, s);
    return s;
}

int ReadLineWithNumber() {
    int result = 0;
    cin >> result;
    ReadLine();
    return result;
}

vector<string> SplitIntoWords(const string& text) {
    vector<string> words;
    string word;
    for (const char c : text) {
        if (c == ' ') {
            if (!word.empty()) {
                words.push_back(word);
                word.clear();
            }
        }
        else {
            word += c;
        }
    }
    if (!word.empty()) {
        words.push_back(word);
    }
    return words;
}

struct Document {
    Document() = default;

    Document(int id_, double relevance_, int rating_)
        : id(id_), relevance(relevance_), rating(rating_)
    {}

    int id = 0;
    double relevance = 0.0;
    int rating = 0;
};

enum class DocumentStatus {
    ACTUAL,
    IRRELEVANT,
    BANNED,
    REMOVED
};

class SearchServer {
public:
    static constexpr int INVALID_DOCUMENT_ID = -1;

    SearchServer() = default;

    explicit SearchServer(const string& stop_words) {
        for (const string& word : SplitIntoWords(stop_words)) {
            if (!IsValidWord(word)) throw invalid_argument("Using invalid characters");
            stop_words_.insert(word);
        }
    }

    template <typename StringCollection>
    explicit SearchServer(const StringCollection& stop_words) {
        string stop_words_text;
        for (const auto& text : stop_words) {
            stop_words_text += " "s;
            stop_words_text += text;
        }
        for (const auto& word : SplitIntoWords(stop_words_text)) {
            if (!IsValidWord(word)) throw invalid_argument("Using invalid characters");
            stop_words_.insert(word);
        }
    }

    void AddDocument(int document_id, const string& document, DocumentStatus status, const vector<int>& ratings) {
        if (document_id < 0) throw invalid_argument("Id cannot be less than zero");
        if (documents_.find(document_id) != documents_.end()) throw invalid_argument("Your id goes beyond the existing id's");

        const vector<string> words = SplitIntoWordsNoStop(document);
        double freq_num = 1.0 / words.size();
        for (auto& word : words) {
            word_to_document_freqs_[word][document_id] += freq_num;
        }
        documents_.emplace(document_id, DocumentData{ ComputeAverageRating(ratings), status });
        documents_index_.push_back(document_id);
    }

    int GetDocumentCount() const {
        return static_cast<int>(documents_.size());
    }

    template <typename DocumentPredicate>
    vector<Document> FindTopDocuments(const string& raw_query, DocumentPredicate document_predicate) const {
        if (raw_query.empty()) throw invalid_argument("Query is empty");

        for (const string& word : SplitIntoWords(raw_query))
            if (!IsValidWord(word) || !IsValidMinusWord(word)) throw invalid_argument("Using invalid characters");

        const Query query = ParseQuery(raw_query);
        auto matched_documents = FindAllDocuments(query, document_predicate);

        sort(matched_documents.begin(), matched_documents.end(),
            [](const Document& lhs, const Document& rhs) {
                const double EPSILON = 1e-6;
                if (abs(lhs.relevance - rhs.relevance) < EPSILON) {
                    return lhs.rating > rhs.rating;
                }
                else {
                    return lhs.relevance > rhs.relevance;
                }
            });
        if (matched_documents.size() > MAX_RESULT_DOCUMENT_COUNT) {
            matched_documents.resize(MAX_RESULT_DOCUMENT_COUNT);
        }

        return matched_documents;
    }

    vector<Document> FindTopDocuments(const string& raw_query, DocumentStatus status) const {
        return FindTopDocuments(raw_query,
            [&status](int document_id, DocumentStatus new_status, int rating) {
                return new_status == status;
            });
    }

    vector<Document> FindTopDocuments(const string& raw_query) const {
        return FindTopDocuments(raw_query, DocumentStatus::ACTUAL);
    }

    tuple<vector<string>, DocumentStatus> MatchDocument(const string& raw_query, int document_id) const {
        if (raw_query.empty()) throw invalid_argument("Query is empty");

        for (const string& word : SplitIntoWords(raw_query))
            if (!IsValidWord(word) || !IsValidMinusWord(word)) throw invalid_argument("Using invalid characters");

        const Query query = ParseQuery(raw_query);
        vector<string> matched_words;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.push_back(word);
            }
        }
        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) {
                continue;
            }
            if (word_to_document_freqs_.at(word).count(document_id)) {
                matched_words.clear();
                break;
            }
        }

        return { matched_words, documents_.at(document_id).status };
    }

    int GetDocumentId(int index) const {
        if ((index >= 0) && (index < static_cast<int>(documents_.size()))) return documents_index_.at(index);
        else throw out_of_range("Index out of range");
    }

private:
    struct Query {
        set<string> plus_words;
        set<string> minus_words;
    };

    struct DocumentData {
        int rating;
        DocumentStatus status;
    };

    map<string, map<int, double>> word_to_document_freqs_;
    map<int, DocumentData> documents_;
    set<string> stop_words_;
    vector<int> documents_index_;

    bool IsStopWord(const string& word) const {
        return stop_words_.count(word) > 0;
    }

    static bool IsValidWord(const string& word) {
        return none_of(word.begin(), word.end(), [](char c) {
            return c >= '\0' && c < ' ';
            });
    }

    static bool IsValidMinusWord(const string& word) {
        if (word.empty() || word == "-"s || (word[0] == '-' && word[1] == '-')) return false;
        else return true;
    }

    vector<string> SplitIntoWordsNoStop(const string& text) const {
        vector<string> words;
        for (const string& word : SplitIntoWords(text)) {
            if (!IsValidWord(word)) throw invalid_argument("Using invalid characters");
            if (!IsStopWord(word)) {
                words.push_back(word);
            }
        }
        return words;
    }

    static int ComputeAverageRating(const vector<int>& ratings) {
        if (ratings.empty()) return 0;
        return accumulate(ratings.begin(), ratings.end(), 0) / static_cast<int>(ratings.size());
    }

    Query ParseQuery(const string& text) const {
        Query query;
        for (string& word : SplitIntoWordsNoStop(text)) {
            if (word[0] == '-') {
                word = word.substr(1);
                if (!IsStopWord(word)) {
                    query.minus_words.insert(word);
                }
            }
            query.plus_words.insert(word);
        }
        return query;
    }

    const double IDFCount(size_t size) const {
        return log(GetDocumentCount() * 1.0 / size);
    }

    template<typename DocumentPredicate>
    vector<Document> FindAllDocuments(const Query& query, DocumentPredicate predicate) const {
        map<int, double> document_to_relevance;
        for (const string& word : query.plus_words) {
            if (word_to_document_freqs_.count(word) == 0) continue;

            for (const auto& document_id_term_freq : word_to_document_freqs_.at(word)) {
                const DocumentData documents_data = documents_.at(document_id_term_freq.first);
                if (predicate(document_id_term_freq.first, documents_data.status, documents_data.rating)) {
                    const double IDF = IDFCount(word_to_document_freqs_.at(word).size());
                    document_to_relevance[document_id_term_freq.first] += IDF * document_id_term_freq.second;
                }
            }
        }

        for (const string& word : query.minus_words) {
            if (word_to_document_freqs_.count(word) == 0) continue;

            for (const auto& doc : word_to_document_freqs_.at(word)) {
                document_to_relevance.erase(doc.first);
            }
        }

        vector<Document> matched_documents;
        for (const auto& document : document_to_relevance)
            matched_documents.push_back({ document.first, document.second, documents_.at(document.first).rating });

        return matched_documents;
    }
};

void PrintDocument(const Document& document) {
    cout << "{ "s
        << "document_id = "s << document.id << ", "s
        << "relevance = "s << document.relevance << ", "s
        << "rating = "s << document.rating
        << " }"s << endl;
}

void TestErrors(SearchServer ss) {
    try {
        ss.AddDocument(1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    }
    catch (const invalid_argument& e)
    {
        cout << "Error: " << e.what() << endl;
    }
    catch (...) {
        cout << "Unknown error" << endl;
    }
    //-----------------------------------------
    try {
        ss.AddDocument(-1, "пушистый пёс и модный ошейник"s, DocumentStatus::ACTUAL, { 1, 2 });
    }
    catch (const invalid_argument& e)
    {
        cout << "Error: " << e.what() << endl;
    }
    catch (...) {
        cout << "Unknown error" << endl;
    }
    //-----------------------------------------
    try {
        ss.AddDocument(3, "большой пёс скво\x12рец"s, DocumentStatus::ACTUAL, { 1, 2 });
    }
    catch (const invalid_argument& e)
    {
        cout << "Error: " << e.what() << endl;
    }
    catch (...) {
        cout << "Unknown error" << endl;
    }
    //-----------------------------------------
    try
    {
        for (const Document& document : ss.FindTopDocuments("--пушистый"s)) {
            PrintDocument(document);
        }
    }
    catch (const invalid_argument& e)
    {
        cout << "Error: " << e.what() << endl;
    }
    catch (...) {
        cout << "Unknown error" << endl;
    }
}

int main() {
    setlocale(LC_ALL, "Russian");

    SearchServer search_server("и в на"s);

    search_server.AddDocument(1, "пушистый кот пушистый хвост"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    TestErrors(search_server);
}