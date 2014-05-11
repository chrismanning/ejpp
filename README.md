[ejpp](http://chrismanning.github.io/ejpp) is a C++ wrapper for [EJDB](http://ejdb.org/).

\tableofcontents

# Rationale {#rationale}

EJDB is a C library, so why a C++ wrapper?

While the C library is indeed usable from C++, its usage is far from idiomatic.
ejpp provides a safe and modern C++ interface, taking advantage of many C++ language and library features such as exceptions, RAII, reference counted memory management, containers, and more.

# Usage {#usage}

ejpp has three main classes, `ejdb::db`, `ejdb::collection`, and `ejdb::query`, which correspond to the opaque types `EJDB`, `EJCOLL`, and `EJQ`, respectively.
Valid `ejdb::collection` and `ejdb::query` objects can only be created via `ejdb::db`; their default, or ill-constructed forms will give errors when attempting to perform operations on them.

ejpp does not impose a specific form of BSON data other than `std::vector<char>` so that it can be used with your favourite BSON library.

~~~cpp
#include <ejpp/ejdb.hpp>
using namespace ejdb;
#include <jbson/document.hpp>
#include <jbson/json_reader.hpp>
using namespace jbson::literal;

db my_db;
assert(!my_db.is_open());

// Open "my_db". Throws std::system_error containing an EJDB error code on error.
my_db.open("my_db", db_mode::read | db_mode::write | db_mode::create | db_mode::truncate);

collection my_coll;
assert(!my_coll);

// Open/create "my_coll" collection.
std::error_code ec;
my_coll = my_db.create_collection("my_coll", ec);
assert(!ec && my_coll);

// Save some BSON data to a collection. Throws std::system_error containing an EJDB error code on error.
auto oid = my_coll.save_document(R"({"some key": "some value"})"_json_doc.data());

// Load raw BSON data from collection into a jbson::document.
// Throws std::system_error containing an EJDB error code on error.
auto doc = jbson::document(my_coll.load_document(oid));

auto it = doc.find("some key");
assert(it != doc.end());
assert(it->value<std::string>() == "some value");
~~~
Many ejpp functions offer throwing and non-throwing overloads. Non-throwing overloads take an extra parameter, a non-const lvalue reference to a `std::error_code`.

## Queries {#qry}

Queries are handled in much the same way as EJDB. A `ejdb::query` object is created from a BSON document, and can then be manipulated (add hints, `$or` or `$and` conditions) and executed. Queries can be executed with several different flags which will change the behaviour and return type of the query result.

~~~cpp
#include <ejpp/ejdb.hpp>
using namespace ejdb;
#include <jbson/document.hpp>
#include <jbson/json_reader.hpp>
using namespace jbson::literal;

db my_db;
my_db.open("my_db", db_mode::read | db_mode::write);
assert(my_db.is_open());

collection my_coll = my_db.get_collection("my_coll");
assert(my_coll);

std::vector<char> bson_qry = R"({"some key": { "$begin": "some v" }})"_json_doc.data();

auto qry = my_db.create_query(bson_qry)
                .set_hints(R"({"$max": 4})"_json_doc.data()); // Return no more than 4 matches.

// Return all matches, limited by hints only.
std::vector<std::vector<char>> results = my_coll.execute_query(qry);
// same as 
// std::vector<std::vector<char>> results = my_coll.execute_query<query_search_mode::normal>(qry);

// Just count the number of matches, don't bother assembling the documents. Handy for update queries.
uint32_t no_results = my_coll.execute_query<query_search_mode::count_only>(qry);

// Just return the first match. Empty vector on no matches.
std::vector<char> first_result = my_coll.execute_query<query_search_mode::first_only>(qry);

// Count only up to a maximum of one.
uint32_t has_result = my_coll.execute_query<query_search_mode::first_only|query_search_mode::count_only>(qry);
~~~

## Transactions {#trans}

EJDB supports transactions at the collection level, and ejpp wraps this functionality via the class `ejdb::collection::transaction_t`, which is contained within each `ejdb::collection`.
This class alone provides no additional safety features, though with the help of `ejdb::transaction_guard` and `ejdb::unique_transaction` ejpp provides exception safety using RAII. `ejdb::transaction_guard` and `ejdb::unique_transaction` are mostly modelled after `std::lock_guard` and `std::unique_lock`, respectively.

~~~cpp
#include <ejpp/ejdb.hpp>
using namespace ejdb;
#include <jbson/document.hpp>
#include <jbson/json_reader.hpp>
using namespace jbson::literal;

db my_db;
my_db.open("my_db", db_mode::read | db_mode::write);
assert(my_db.is_open());

collection my_coll = my_db.get_collection("my_coll");
assert(my_coll);

using oid_t = std::array<char, 12>;

oid_t oid;

{
    // Commits at scope exit, or aborts when an exception is thrown.
    // Cannot be manually committed or aborted.
    transaction_guard gu(my_coll.transaction());
    oid = my_coll.save_document(R"({"some other key": "some other value"})"_json_doc.data());
}

assert(!my_coll.load_document(oid).empty());

try {
    // Commits at scope exit, or aborts when an exception is thrown.
    // Cannot be manually committed or aborted.
    transaction_guard gu(my_coll.transaction());
    oid = my_coll.save_document(R"({"some other key": "some other value"})"_json_doc.data());
    throw 0;
}
catch(...) {}

assert(my_coll.load_document(oid).empty());

// Can be manually committed or aborted.
unique_transaction ut(my_coll.transaction());
oid = my_coll.save_document(R"({"some other key": "some other value"})"_json_doc.data());
ut.commit();

assert(!my_coll.load_document(oid).empty());

// Rebindable.
ut = unique_transaction(my_coll.transaction());
oid = my_coll.save_document(R"({"some other key": "some other value"})"_json_doc.data());
ut.abort();

assert(my_coll.load_document(oid).empty());

~~~

# Installation {#install}

## Requirements {#reqs}
 - C++11 compliant compiler.
 - Boost headers. (Uses Boost.Optional, and Boost.Config).
 - [cmake](http://www.cmake.org/).
 - [EJDB](http://ejdb.org/).

## Building {#build}

~~~sh
mkdir build && cd build
cmake ..
make
make install
~~~

If EJDB is in a non-standard location, pass `EJDB_INCLUDE_DIR` and `EJDB_LIBRARY_DIR` options to cmake.
~~~sh
cmake .. -DEJDB_INCLUDE_DIR=<location-of-tcejdb-header-dir> -DEJDB_LIBRARY_DIR=<location-of-tcejdb-lib-dir>
~~~

### CMake Options {#cmake}

- `EJPP_ENABLE_TESTING` - set to `ON` to enable testing. `OFF` by default. See [Testing](@ref test).
- `EJPP_SANITIZE_ADDRESS` - set to `ON` to build with `-fsanitize=address`. `OFF` by default.
- `EJPP_LEAK_CHECKER` - set to `ON` to build with `-fsanitize=leak`. `OFF` by default.
- `EJPP_DOC_OUTPUT_DIR` - set to a path to output generated documentation. `./doc` by default. See [Documentation](@ref docs).
- `EJPP_LIBDIR_SUFFIX` - set to a string to add a suffix to the installation library directory, e.g. "64" to install to ${PREFIX}/lib64.

## Documentation {#docs}

API documentation can be viewed at <http://chrismanning.github.io/ejpp> or generated with doxygen.
~~~
make doc
~~~
It can then be viewed in a web browser by opening the generated `index.html`, or any other file.

## Testing {#test}

Unit testing (enabled via `EJPP_ENABLE_TESTING`) has some additional requirements.
 - Subversion is needed for downloading google test at build time.
 - [jbson](http://chrismanning.github.io/jbson).
   This does not have to be installed, its include directory can be specified explicitly via the cmake variable `JBSON_INCLUDE_DIR`.
