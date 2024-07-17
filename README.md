# Search server
This project provides for the implementation of a search engine. It includes searching for relevant data in the system by the keys required by the queries. The main logic is to parse strings when adding data to the search server and find the most relevant words by keys.

## Features
- **AddDocument**: Adds data to the "Server"
- **AddFindRequest**: Conducts a search on request
- **GetNoResultRequests**: Retrieves data for queries that have never been searched for

## Installation and Usage
To compile and run the tests for the Single-Linked List implementation, follow these steps:

1. **Clone the repository**:
   ```sh
   git clone <repository-url>
   cd <repository-directory>
2. **Compile the program:**
   ```g++ -o test_program test_program.cpp -std=c++17```

3. **Run the tests:**
   ```./test_program```

## System Requirements
- C++ Compiler: GCC 7.3 or later, or any other C++17 compatible compiler.
- Dependencies: No external dependencies are required.

## Future Plans
- Enhanced Iterator Support: Add reverse iterators for backward traversal.
- Additional Utility Functions: Add more utility functions like Find, Reverse, and Sort.
- Performance Optimizations: Optimize memory usage and performance for large lists.
  
## Technology Stack
- Language: C++17
- Compiler: GCC (or any C++17 compatible compiler)
- Build Tools: None required (simple g++ command used for compilation)
  
## License
This project is licensed under the MIT License.

## Contribution
Contributions are welcome! Please submit a pull request or open an issue to discuss any changes.

## Example Test Case Breakdown
```cpp
#include "document.h"
#include "paginator.h"
#include "read_input_functions.h"
#include "string_processing.h"
#include "search_server.h"
#include "request_queue.h"

int main() {
    setlocale(LC_ALL, "Russian");

    SearchServer search_server("and in at"s);
    RequestQueue request_queue(search_server);

    search_server.AddDocument(1, "curly cat curly tail"s, DocumentStatus::ACTUAL, { 7, 2, 7 });
    search_server.AddDocument(2, "curly dog and fancy collar"s, DocumentStatus::ACTUAL, { 1, 2, 3 });
    search_server.AddDocument(3, "big cat fancy collar "s, DocumentStatus::ACTUAL, { 1, 2, 8 });
    search_server.AddDocument(4, "big dog sparrow Eugene"s, DocumentStatus::ACTUAL, { 1, 3, 2 });
    search_server.AddDocument(5, "big dog sparrow Vasiliy"s, DocumentStatus::ACTUAL, { 1, 1, 1 });

    for (int i = 0; i < 1439; ++i) {
        request_queue.AddFindRequest("empty request"s);
    }
    
    request_queue.AddFindRequest("curly dog"s);
    
    request_queue.AddFindRequest("big collar"s);
    
    request_queue.AddFindRequest("sparrow"s);
    std::cout << "Total empty requests: "s << request_queue.GetNoResultRequests() << std::endl;
    return 0;
}
```
