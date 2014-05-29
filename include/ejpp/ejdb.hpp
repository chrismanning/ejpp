/******************************************************************************
 *
 * C++11 wrapper for EJDB (http://ejdb.org)
 * Copyright (C) 2013 Christian Manning
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301
 * USA
 *****************************************************************************/

#ifndef EJDB_HPP
#define EJDB_HPP

#include <memory>
#include <string>
#include <system_error>
#include <vector>
#include <array>
#include <experimental/optional>

#include <boost/config.hpp>

struct EJDB;
struct EJCOLL;
struct EJQ;

#ifndef EJPP_EXPORTS
#define EJPP_EXPORT BOOST_SYMBOL_IMPORT
#else
#define EJPP_EXPORT BOOST_SYMBOL_EXPORT
#endif

#ifndef EJPP_LOCAL
#if defined(__GNUC__)
#define EJPP_LOCAL __attribute__((__visibility__("hidden")))
#else
#define EJPP_LOCAL
#endif
#endif

/*!
 * \brief Contains the main EJPP classes, etc.
 * \namespace ejdb
 */
namespace ejdb {
struct collection;
struct query;

//! Database open modes
enum class db_mode {
    read = 1 << 0,      //!< Open as a reader.
    write = 1 << 1,     //!< Open as a writer.
    create = 1 << 2,    //!< Create if db file not exists.
    truncate = 1 << 3,  //!< Truncate db on open.
    nolock = 1 << 4,    //!< Open without locking.
    noblock = 1 << 5,   //!< Lock without blocking.
    trans_sync = 1 << 6 //!< Synchronize every transaction.
};

//! Allow bitwise-OR of db_mode.
constexpr inline db_mode operator|(db_mode lhs, db_mode rhs) noexcept {
    return (db_mode)((std::underlying_type<db_mode>::type)lhs | (std::underlying_type<db_mode>::type)rhs);
}

//! Allow bitwise-OR-assign of db_mode.
constexpr inline db_mode& operator|=(db_mode& lhs, db_mode rhs) noexcept { return lhs = lhs | rhs; }

//! Index modes, index types.
enum class index_mode {
    drop = 1 << 0,     //!< Drop index.
    drop_all = 1 << 1, //!< Drop index for all types.
    optimize = 1 << 2, //!< Optimize indexes.
    rebuild = 1 << 3,  //!< Rebuild index.
    number = 1 << 4,   //!< Number index.
    string = 1 << 5,   //!< String index.
    array = 1 << 6,    //!< Array token index.
    istring = 1 << 7   //!< Case insensitive string index.
};

//! Allow bitwise-OR of index_mode.
constexpr inline index_mode operator|(index_mode lhs, index_mode rhs) noexcept {
    return (index_mode)((std::underlying_type<index_mode>::type)lhs | (std::underlying_type<index_mode>::type)rhs);
}

//! Allow bitwise-OR-assign of index_mode.
constexpr inline index_mode& operator|=(index_mode& lhs, index_mode rhs) noexcept { return lhs = lhs | rhs; }

//! Query search mode flags
enum class query_search_mode {
    normal = 0,         //!< Default search mode
    count_only = 1,     //!< Query only count(*)
    first_only = 1 << 1 //!< Fetch first record only
};

//! Allow bitwise-OR of query_search_mode.
constexpr inline query_search_mode operator|(query_search_mode lhs, query_search_mode rhs) noexcept {
    return (query_search_mode)((std::underlying_type<query_search_mode>::type)lhs |
                               (std::underlying_type<query_search_mode>::type)rhs);
}

//! Allow bitwise-OR-assign of query_search_mode.
constexpr inline query_search_mode& operator|=(query_search_mode& lhs, query_search_mode rhs) noexcept {
    return lhs = lhs | rhs;
}

//! Allow bitwise-AND of query_search_mode.
constexpr inline query_search_mode operator&(query_search_mode lhs, query_search_mode rhs) noexcept {
    return (query_search_mode)((std::underlying_type<query_search_mode>::type)lhs &
                               (std::underlying_type<query_search_mode>::type)rhs);
}

//! Allow bitwise-AND-assign of query_search_mode.
constexpr inline query_search_mode& operator&=(query_search_mode& lhs, query_search_mode rhs) noexcept {
    return lhs = lhs & rhs;
}

//! Error codes. Tokyo Cabinet errors up to errc::miscellaneous.
enum class errc {
    // Tokyo cabinet error codes
    success,                   //!< success
    thread_error,              //!< threading error
    invalid_operation,         //!< invalid operation
    file_not_found,            //!< file not found
    no_permission,             //!< no permission
    invalid_tc_metadata,       //!< invalid meta data
    invalid_record_header,     //!< invalid record header
    open_error,                //!< open error
    close_error,               //!< close error
    trunc_error,               //!< trunc error
    sync_error,                //!< sync error
    stat_error,                //!< stat error
    seek_error,                //!< seek error
    read_error,                //!< read error
    write_error,               //!< write error
    mmap_error,                //!< mmap error
    lock_error,                //!< lock error
    unlink_error,              //!< unlink error
    rename_error,              //!< rename error
    mkdir_error,               //!< mkdir error
    rmdir_error,               //!< rmdir error
    keep,                      //!< existing record
    no_record_found,           //!< no record found
    illegal_transaction_state, //!< illegal transaction state
    miscellaneous = 9999,      //!< miscellaneous error
    // EJDB error codes
    invalid_collection_name = 9000,          //!< Invalid collection name.
    invalid_bson = 9001,                     //!< Invalid bson object.
    invalid_bson_oid = 9002,                 //!< Invalid bson object id.
    invalid_query_control_field = 9003,      //!< Invalid query control field starting with '$'.
    query_field_require_array = 9004,        //!< $strand, $stror, $in, $nin, $bt keys requires not empty array value.
    invalid_metadata = 9005,                 //!< Inconsistent database metadata.
    invalid_field_path = 9006,               //!< Invalid field path value.
    invalid_query_regex = 9007,              //!< Invalid query regexp value.
    query_result_sort_error = 9008,          //!< Result set sorting error.
    query_error = 9009,                      //!< Query generic error.
    query_update_failed = 9010,              //!< Updating failed.
    query_elemmatch_limit = 9011,            //!< Only one $elemMatch allowed in the fieldpath.
    query_cannot_mix_include_exclude = 9012, //!< $fields hint cannot mix include and exclude fields.
    query_invalid_action = 9013,             //!< action key in $do block can only be one of: $join.
    too_many_collections = 9014,             //!< Exceeded the maximum number of collections per database.
    import_export_error = 9015,              //!< EJDB export/import error.
    json_parse_failed = 9016,                //!< JSON parsing failed.
    bson_too_large = 9017,                   //!< BSON size is too big.
    invalid_command = 9018                   //!< Invalid ejdb command specified.
};

//! Makes an std::error_code from an ejdb::errc.
std::error_code make_error_code(errc ecode) noexcept;

} // namespace ejdb

namespace std {
//! Allows for implicit conversion from ejdb::errc to std::error_code.
template <> struct is_error_code_enum<ejdb::errc> : public true_type {};
}

namespace ejdb {

/*!
 * \brief Main point of access to EJDB.
 *
 * Uses a shared implementation, i.e. copies of a db object operate on the same EJDB handle.
 */
struct EJPP_EXPORT db final {
    //! Default constructor. Result is a null EJDB handle.
    db() noexcept = default;

    //! Returns whether a valid EJDB handle is contained within.
    explicit operator bool() const noexcept;

    //! Returns last error that occurred.
    std::error_code error() const noexcept;
    //! Returns last error that occurred.
    static std::error_code error(std::weak_ptr<EJDB>) noexcept;

    //! Opens EJDB database at \p path.
    bool open(const std::string& path, db_mode mode, std::error_code& ec);
    //! \copybrief open
    void open(const std::string& path, db_mode mode);

    //! Returns whether the db object refers to a valid and open EJDB database.
    bool is_open() const noexcept;

    //! Closes the currently open EJDB database.
    bool close(std::error_code& ec) noexcept;
    //! \copybrief close
    void close();

    //! Returns an existing collection named \p name, or a default constructed ejdb::collection.
    collection get_collection(const std::string& name, std::error_code& ec) const;
    //! \copybrief get_collection
    collection get_collection(const std::string& name) const;

    //! Returns an existing, or otherwise newly created, collection, named \p name.
    collection create_collection(const std::string& name, std::error_code& ec);
    //! \copybrief create_collection
    collection create_collection(const std::string& name);

    //! Removes a collection named \p name, or do nothing if \p name does not exist.
    bool remove_collection(const std::string& name, bool unlink_file, std::error_code& ec);
    //! \copybrief remove_collection
    void remove_collection(const std::string& name, bool unlink_file);

    //! Returns all existing collections.
    const std::vector<collection> get_collections() const;

    //! Create a query from a BSON document.
    query create_query(const std::vector<char>& doc, std::error_code& ec);
    //! \copybrief create_query
    query create_query(const std::vector<char>& doc);

    //! Synchronise the EJDB database to disk.
    bool sync(std::error_code& ec) noexcept;
    //! \copybrief sync
    void sync();

    //! Returns description of the EJDB database.
    std::vector<char> metadata(std::error_code& ec);
    //! \copybrief metadata
    std::vector<char> metadata();

  private:
    std::shared_ptr<EJDB> m_db;
};

/*!
 * \brief Implementation details.
 *
 * \namespace ejdb::detail
 */
namespace detail {

/*!
 * \brief Determine the correct return type for query flags.
 */
template <query_search_mode flags>
using query_return_type = typename std::conditional<
    (flags & query_search_mode::count_only) == query_search_mode::count_only, uint32_t,
    typename std::conditional<(flags & query_search_mode::first_only) == query_search_mode::first_only,
                              std::vector<char>, std::vector<std::vector<char>>>::type>::type;
}

/*!
 * \brief Class representing an EJDB collection.
 *
 * Valid collections can only be created via ejdb::db::create_collection or ejdb::db::get_collection.
 *
 * Should the parent ejdb::db object expire before the collection, all operations performed on or with the collection
 * will fail, with any `std::error_code`s set to `std::errc::operation_not_permitted` (`EPERM`).
 * The parent ejdb::db object is guaranteed to stay alive for the duration of an operation.
 */
struct EJPP_EXPORT collection final {
    //! Default constructor. Results in an invalid collection, not associated with a db.
    collection() noexcept = default;

    //! Returns whether the associated ejdb::db and represented EJDB collection are both valid.
    explicit operator bool() const noexcept;

    //! Saves a document to the collection, overwriting an existing, matching document.
    std::experimental::optional<std::array<char, 12>> save_document(const std::vector<char>& data, std::error_code& ec);
    //! Saves a document to the collection, optionally merging with an existing, matching document.
    std::experimental::optional<std::array<char, 12>> save_document(const std::vector<char>& data, bool merge,
                                                                    std::error_code& ec);
    //! \copybrief save_document(const jbson::document&,bool,std::error_code&)
    std::array<char, 12> save_document(const std::vector<char>& data, bool merge = false);

    //! Loads a matching document from the collection.
    std::vector<char> load_document(std::array<char, 12> oid, std::error_code& ec) const;
    //! \copybrief load_document
    std::vector<char> load_document(std::array<char, 12> oid) const;

    //! Removes a document from the collection.
    bool remove_document(std::array<char, 12>, std::error_code& ec) noexcept;
    //! \copybrief remove_document
    void remove_document(std::array<char, 12>);

    //! Sets the index for a BSON field in the collection.
    bool set_index(const std::string& ipath, index_mode flags, std::error_code& ec);
    //! \copybrief set_index
    void set_index(const std::string& ipath, index_mode flags);

    /*!
     * \brief Executes a query on the collection.
     *
     * \tparam flags The mode by which to execute the query. Determines return type.
     * \sa detail::query_return_type
     */
    template <query_search_mode flags = query_search_mode::normal>
    detail::query_return_type<flags> execute_query(const query&);

    //! Returns all documents in the collection.
    std::vector<std::vector<char>> get_all();

    //! Synchronises the EJDB database to disk.
    bool sync(std::error_code& ec) noexcept;
    //! \copybrief sync
    void sync();

    //! Returns the name of the collection.
    std::string name() const;

    struct transaction_t;

    //! Returns this collection's transaction_t.
    transaction_t& transaction() noexcept;

  private:
    friend struct db;
    EJPP_LOCAL collection(std::weak_ptr<EJDB> m_db, EJCOLL* m_coll) noexcept;

    std::weak_ptr<EJDB> m_db;
    EJCOLL* m_coll{nullptr};

  public:
    /*!
     * \brief Represents an EJDB transaction.
     *
     * A [transaction](http://en.wikipedia.org/wiki/Database_transaction)
     * is a series of operations that will either be completed all at once or not at all.
     *
     * A collection cannot have more than one transaction at a time.
     * A transaction must be committed or aborted before another can start.
     * Transactions cannot be nested.
     *
     * A transaction has no effect on the lifetimes of a parent db or collection object,
     * only references to a transaction_t can be obtained by users.
     *
     * Although none of the member functions throw, it is not exception-safe to use this class directly for transaction
     * functionality.
     */
    struct transaction_t {
        //! Starts a transaction.
        bool start() noexcept;
        //! Commits a transaction.
        bool commit() noexcept;
        //! Aborts a transaction.
        bool abort() noexcept;

        //! Returns whether a transaction is in progress.
        bool in_transaction() const noexcept;
        //! \copydoc in_transaction
        explicit operator bool() const noexcept;

      private:
        explicit transaction_t(collection*) noexcept;
        transaction_t(transaction_t&&) = default;
        transaction_t& operator=(transaction_t&&) = default;
        transaction_t(const transaction_t& b) = default;
        transaction_t& operator=(const transaction_t&) = default;

        friend struct collection;
        friend struct unique_transaction;
        collection* m_collection;
        std::weak_ptr<EJDB> m_db;
    };

  private:
    transaction_t m_transaction{this};
};

#ifndef DOXYGEN_SHOULD_SKIP_THIS
extern template EJPP_EXPORT detail::query_return_type<query_search_mode::normal>
collection::execute_query<query_search_mode::normal>(const query& qry);
extern template EJPP_EXPORT detail::query_return_type<query_search_mode::count_only>
collection::execute_query<query_search_mode::count_only>(const query& qry);
extern template EJPP_EXPORT detail::query_return_type<query_search_mode::first_only>
collection::execute_query<query_search_mode::first_only>(const query& qry);
extern template EJPP_EXPORT detail::query_return_type<query_search_mode::count_only | query_search_mode::first_only>
collection::execute_query<query_search_mode::count_only | query_search_mode::first_only>(const query& qry);
#endif // DOXYGEN_SHOULD_SKIP_THIS

/*!
 * \brief Class representing an EJDB query.
 *
 * Valid queries can only be created via ejdb::db::create_query.
 *
 * Should the parent ejdb::db object expire before the query, all operations performed on or with the query will fail,
 * with any `std::error_code`s set to `std::errc::operation_not_permitted` (`EPERM`).
 * The parent ejdb::db object is guaranteed to stay alive for the duration of an operation.
 */
struct EJPP_EXPORT query final {
    //! Default constructor. Results in an invalid query, not associated with a db.
    query() noexcept = default;

    //! Returns whether the associated ejdb::db and represented EJDB query are both valid.
    explicit operator bool() const noexcept;

    //! In-place `$and` operator. \warning Unimplemented.
    query& operator&=(const std::vector<char>&)&;
    //! In-place `$and` operator. \warning Unimplemented.
    query&& operator&=(const std::vector<char>&)&&;
    //! In-place `$and` operator. \warning Unimplemented.
    query& operator&=(query) & noexcept;
    //! In-place `$and` operator. \warning Unimplemented.
    query&& operator&=(query) && noexcept;

    //! In-place `$or` operator with BSON document as operand.
    query& operator|=(const std::vector<char>&)&;
    //! In-place `$or` operator with BSON document as operand. Rvalue overload.
    query&& operator|=(const std::vector<char>&)&&;
    //! In-place `$or` operator with ejdb::query as operand. \warning Unimplemented.
    query& operator|=(query) & noexcept;
    //! In-place `$or` operator with ejdb::query as operand. Rvalue overload. \warning Unimplemented.
    query&& operator|=(query) && noexcept;

    //! Sets hints for a query.
    query& set_hints(const std::vector<char>&)&;
    //! \copydoc set_hints
    query&& set_hints(const std::vector<char>&)&&;

  private:
    friend struct db;
    friend struct collection;
    EJPP_LOCAL query(std::weak_ptr<EJDB> m_db, EJQ* m_qry) noexcept;

    std::weak_ptr<EJDB> m_db;

    struct eqry_deleter {
        void operator()(EJQ* ptr) const noexcept;
    };
    std::unique_ptr<EJQ, eqry_deleter> m_qry;
};

//! Tag type for expressing an adopted transaction.
struct adopt_transaction_t {};
//! Tag type for expressing a transaction that only tries to start.
struct try_transaction_t {};

//! Tag constant for adopting a transaction.
constexpr adopt_transaction_t adopt_transaction{};
//! Tag constant for trying to start a transaction.
constexpr try_transaction_t try_transaction{};

/*!
 * \brief Defines an exception-safe wrapper around collection::transaction_t with unique ownership.
 *
 * Uses [RAII](http://en.wikipedia.com/wiki/Resource_Acquisition_Is_Initialization) to allow safe usage of a
 * collection::transaction_t in the presence of exceptions.
 * When a collection::transaction_t is owned, the destructor aborts transaction when an exception is thrown,
 * otherwise commits.
 *
 * The parent db object is guaranteed to remain alive throughout the lifetime of a unique_transaction.
 *
 * The parent collection object must stay alive at least as long as a unique_transaction when it owns or contains a
 * collection::transaction_t.
 */
struct EJPP_EXPORT unique_transaction {
    //! Default constructor. Does not own a collection::transaction_t.
    unique_transaction() noexcept = default;
    //! Constructs from a collection::transaction_t and starts transaction.
    explicit unique_transaction(collection::transaction_t& trans);

    //! Constructs from a collection::transaction_t and adopts its status.
    unique_transaction(collection::transaction_t& trans, adopt_transaction_t) noexcept;
    //! Constructs from a collection::transaction_t and attempts to start transaction.
    unique_transaction(collection::transaction_t& trans, try_transaction_t) noexcept;

    //! Move constructor.
    unique_transaction(unique_transaction&&) noexcept;
    //! Move assignment.
    unique_transaction& operator=(unique_transaction&&);

    ~unique_transaction();

    //! Starts a transaction.
    void start();
    //! Commits all changes since starting a transaction.
    void commit();
    //! Aborts all changes since starting a transaction.
    void abort();

    //! Releases ownership of current collection::transaction_t, if any, without terminating its transaction.
    collection::transaction_t* release() noexcept;

    //! Returns whether or not a collection::transaction_t is currently owned and has a transaction in progress.
    bool owns_transaction() const noexcept;
    //! \copybrief owns_transaction
    explicit operator bool() const noexcept;

  private:
    unique_transaction(const unique_transaction&) = delete;
    unique_transaction& operator=(const unique_transaction&) = delete;

    collection::transaction_t* m_trans{nullptr};
    bool m_owns{false};
    std::shared_ptr<EJDB> m_db;
};

/*!
 * \brief Simple collection::transaction_t wrapper for exception safe operation using
 * [RAII](http://en.wikipedia.com/wiki/Resource_Acquisition_Is_Initialization)
 *
 * Destructor aborts transaction when an exception is thrown, otherwise commits.
 */
struct EJPP_EXPORT transaction_guard : private unique_transaction {
    //! Starts on
    explicit transaction_guard(collection::transaction_t& trans);
};

} // namespace ejdb

#endif // EJDB_HPP
