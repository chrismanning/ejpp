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

#include <array>
#include <string>
using namespace std::literals;

#include <boost/optional.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/utility/string_ref.hpp>

#include <ejpp/c_ejdb.hpp>
#include <ejpp/ejdb.hpp>

namespace ejdb {

//! Functor allowing for the deletion of opaque `EJDB` pointers.
struct ejdb_deleter {
    //! Function call operator.
    void operator()(EJDB* ptr) const noexcept { c_ejdb::del(ptr); }
};

void query::eqry_deleter::operator()(EJQ* ptr) const noexcept { c_ejdb::querydel(ptr); }

db::operator bool() const noexcept { return static_cast<bool>(m_db); }

/*!
 * \return std::error_code representing last error,
 *         or std::errc::operation_not_permitted (EPERM) if EJDB handle is invalid.
 */
std::error_code db::error() const noexcept {
    if(!m_db)
        return std::make_error_code(std::errc::operation_not_permitted);
    return static_cast<errc>(c_ejdb::ecode(m_db.get()));
}

/*!
 * \param db weak_ptr to an EJDB handle.
 * \return std::error_code representing last error, or std::errc::operation_not_permitted (EPERM) if db refers to
 * destroyed handle.
 */
std::error_code db::error(std::weak_ptr<EJDB> db) noexcept {
    auto ptr = db.lock();
    if(!ptr)
        return std::make_error_code(std::errc::operation_not_permitted);
    return static_cast<errc>(c_ejdb::ecode(ptr.get()));
}

/*!
 * \param path Location on filesystem where an EJDB databaes exists.
 * \param mode Bitwise-ORed flags to determine how to open the database.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return true on success, false on failure.
 */
bool db::open(const std::string& path, db_mode mode, std::error_code& ec) noexcept {
    m_db = {c_ejdb::newdb(), ejdb_deleter()};
    const auto r = m_db && c_ejdb::open(m_db.get(), path.c_str(), (std::underlying_type_t<db_mode>)mode);
    if(!r)
        ec = error();
    return r;
}

/*!
 * \param path Location on filesystem where an EJDB databaes exists.
 * \param mode Bitwise-ORed flags to determine how to open the database.
 *
 * \throws std::system_error with appropriate error code and message on failure.
 *
 * \sa db::open
 */
void db::open(const std::string& path, db_mode mode) {
    std::error_code ec;
    auto r = open(path, mode, ec);
    (void)r;
    assert(r == !ec);
    if(ec)
        throw std::system_error(ec, "could not open database");
}

bool db::is_open() const noexcept { return m_db && c_ejdb::isopen(m_db.get()); }

/*!
 * \param[out] ec Set to an appropriate error code on failure.
 * \return true on success, false on failure.
 */
bool db::close(std::error_code& ec) noexcept {
    const auto r = m_db && c_ejdb::closedb(m_db.get());
    if(!r)
        ec = error();
    m_db.reset();
    return r;
}

/*!
 * \throws std::system_error with appropriate error code and message on failure.
 *
 * \sa db::close
 */
void db::close() {
    std::error_code ec;
    auto r = close(ec);
    (void)r;
    assert(r == !ec);
    if(ec)
        throw std::system_error(ec, "could not close database");
}

/*!
 * \param name Name of collection to fetch.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return Valid collection on success, default collection on failure.
 */
collection db::get_collection(const std::string& name, std::error_code& ec) const noexcept {
    if(!m_db) {
        ec = error();
        return {};
    }
    const auto r = c_ejdb::getcoll(m_db.get(), name.c_str());
    if(r == nullptr)
        ec = error();
    return {m_db, r};
}

/*!
 * \param name Name of collection to fetch.
 * \return Valid collection on success, default constructed collection when none matching \p name found,
 * throws on failure.
 *
 * \throws std::system_error with appropriate error code and message on failure.
 */
collection db::get_collection(const std::string& name) const {
    std::error_code ec;
    auto coll = get_collection(name, ec);
    if(ec)
        throw std::system_error(ec, "could not get collection "s + name);
    return coll;
}

/*!
 * \param name Name of collection to fetch or create.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return Valid collection on success, invalid collection on failure.
 */
collection db::create_collection(const std::string& name, std::error_code& ec) noexcept {
    if(!m_db) {
        ec = error();
        return {};
    }
    const auto r = c_ejdb::createcoll(m_db.get(), name.c_str(), nullptr);
    if(r == nullptr)
        ec = error();
    return {m_db, r};
}

/*!
 * \param name Name of collection to fetch or create.
 * \return Valid collection on success, throws on failure.
 *
 * \throws std::system_error with appropriate error code and message on failure.
 */
collection db::create_collection(const std::string& name) {
    std::error_code ec;
    auto coll = create_collection(name, ec);
    assert(static_cast<bool>(coll) == !ec);
    if(ec)
        throw std::system_error(ec, "could not get/create collection "s + name);
    return coll;
}

/*!
 * \param name Name of collection to remove.
 * \param unlink_file Whether to remove associated files, i.e. db collection file, indexes, etc.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return true on success, false on failure.
 */
bool db::remove_collection(const std::string& name, bool unlink_file, std::error_code& ec) noexcept {
    const auto r = m_db && c_ejdb::rmcoll(m_db.get(), name.c_str(), unlink_file);
    if(!r)
        ec = error();
    return r;
}

/*!
 * \param name Name of collection to remove.
 * \param unlink_file Whether to remove associated files, i.e. db collection file, indexes, etc.
 *
 * \throws std::system_error with appropriate error code and message on failure.
 */
void db::remove_collection(const std::string& name, bool unlink_file) {
    std::error_code ec;
    remove_collection(name, unlink_file, ec);
    if(ec)
        throw std::system_error(ec, "could not remove collection "s + name);
}

const std::vector<collection> db::get_collections() const {
    if(!m_db)
        return {};
    auto colls = c_ejdb::getcolls(m_db.get());
    auto range = boost::adaptors::transform(colls, [this](EJCOLL* c) -> collection {
        return collection{m_db, c};
    });
    return {range.begin(), range.end()};
}

/*!
 * EJDB's query documentation follows.
 *
 * EJDB queries inspired by MongoDB (mongodb.org) and follows same philosophy.
 *
 *  - Supported queries:
 *      - Simple matching of String OR Number OR Array value:
 *          -   \code {'fpath' : 'val', ...} \endcode
 *      - $not Negate operation.
 *          -   \code {'fpath' : {'$not' : val}} //Field not equal to val \endcode
 *          -   \code {'fpath' : {'$not' : {'$begin' : prefix}}} //Field not begins with val \endcode
 *      - $begin String starts with prefix
 *          -   \code {'fpath' : {'$begin' : prefix}} \endcode
 *      - $gt, $gte (>, >=) and $lt, $lte for number types:
 *          -   \code {'fpath' : {'$gt' : 42}, ...} \endcode
 *      - $bt Between for number types:
 *          -   \code {'fpath' : {'$bt' : [num1, num2]}} \endcode
 *      - $in String OR Number OR Array val matches to value in specified array:
 *          -   \code {'fpath' : {'$in' : [val1, val2, val3]}} \endcode
 *      - $nin - Not IN
 *      - $strand String tokens OR String array val matches all tokens in specified array:
 *          -   \code {'fpath' : {'$strand' : [val1, val2, val3]}} \endcode
 *      - $stror String tokens OR String array val matches any token in specified array:
 *          -   \code {'fpath' : {'$stror' : [val1, val2, val3]}} \endcode
 *      - $exists Field existence matching:
 *          -   \code {'fpath' : {'$exists' : true|false}} \endcode
 *      - $icase Case insensitive string matching:
 *          -    \code {'fpath' : {'$icase' : 'val1'}} //icase matching \endcode
 *          Ignore case matching with '$in' operation:
 *          -    \code {'name' : {'$icase' : {'$in' : ['théâtre - театр', 'hello world']}}} \endcode
 *          For case insensitive matching you can create special index of type: `JBIDXISTR`
 *      - $elemMatch The $elemMatch operator matches more than one component within an array element.
 *          -    \code { 'some_array.fpath': { '$elemMatch': { 'value1' : 1, 'value2' : { '$gt': 1 } } } } \endcode
 *          Restriction: only one $elemMatch allowed in context of one array field.
 *      - $and, $or joining:
 *          -   \code {..., '$and' : [subq1, subq2, ...] } \endcode
 *          -   \code {..., '$or'  : [subq1, subq2, ...] } \endcode
 *          Example:
 *          \code
            {'z' : 33, '$and' : [ {'$or' : [{'a' : 1}, {'b' : 2}]}, {'$or' : [{'c' : 5}, {'d' : 7}]} ] }
            \endcode
 *
 *      - Mongodb $(projection) operator supported.
 *(http://docs.mongodb.org/manual/reference/projection/positional/#proj._S_)
 *      - Mongodb positional $ update operator supported.
 *(http://docs.mongodb.org/manual/reference/operator/positional/)
 *
 *  - Queries can be used to update records:
 *
 *      - $set Field set operation.
 *          - \code {.., '$set' : {'fpath1' : val1, 'fpathN' : valN}} \endcode
 *      - $upsert Atomic upsert. If matching records are found it will be '$set' operation,
 *              otherwise new record will be inserted
 *              with fields specified by argment object.
 *          - \code {.., '$upsert' : {'fpath1' : val1, 'fpathN' : valN}} \endcode
 *      - $inc Increment operation. Only number types are supported.
 *          - \code {.., '$inc' : {'fpath1' : 5, ...,  'fpath2' : 2} \endcode
 *      - $dropall In-place record removal operation.
 *          - \code {.., '$dropall' : true} \endcode
 *      - $addToSet Atomically adds value to the array only if its not in the array already.
 *                If containing array is missing it will be created.
 *          - \code {.., '$addToSet' : {'fpath' : val1, 'fpathN' : valN, ...}} \endcode
 *      - $addToSetAll Batch version if $addToSet
 *          - \code {.., '$addToSetAll' : {'fpath' : [values to add, ...], ...}} \endcode
 *      - $pull  Atomically removes all occurrences of value from field, if field is an array.
 *          - \code {.., '$pull' : {'fpath' : val1, 'fpathN' : valN, ...}} \endcode
 *      - $pullAll Batch version of $pull
 *          - \code {.., '$pullAll' : {'fpath' : [values to remove, ...], ...}} \endcode
 *
 *  - Collection joins supported in the following form:
 *      \code {..., '$do' : {'fpath' : {'$join' : 'collectionname'}} } \endcode
 *      Where 'fpath' value points to object's OIDs from 'collectionname'. Its value
 *      can be OID, string representation of OID or array of this pointers.
 *
 *  \note It is better to execute update queries with collection::execute_query<query_search_mode::count_only>
 *        to avoid unnecessarily fetching data.
 *
 *  \note Negate operations: $not and $nin do not use indexes so they can be slow in comparison to other matching
 *operations.
 *
 *  \note Only one index can be used in search query operation.
 *
 * \param doc BSON query object.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return Valid query on success, invalid query on failure.
 */
query db::create_query(const std::vector<char>& doc, std::error_code& ec) {
    if(!m_db) {
        ec = error();
        return {};
    }
    const auto r = c_ejdb::createquery(m_db.get(), doc.data());
    if(!r) {
        ec = error();
        return query{};
    }
    return query{m_db, r};
}

/*!
 * Same as create_query, throws exception instead of setting an std::error_code on failure.
 *
 * \param doc BSON query object.
 *
 * \throws std::system_error with appropriate error code and message on failure.
 * \sa create_query
 */
query db::create_query(const std::vector<char>& doc) {
    std::error_code ec;
    auto qry = create_query(doc, ec);
    assert(static_cast<bool>(qry) == !ec);
    if(ec)
        throw std::system_error(ec, "could not create query");
    return qry;
}

/*!
 * \param[out] ec Set to an appropriate error code on failure.
 * \return true on success, false on failure.
 */
bool db::sync(std::error_code& ec) noexcept {
    const auto r = m_db && c_ejdb::syncdb(m_db.get());
    if(!r)
        ec = error();
    return r;
}

/*!
 * \throws std::system_error with appropriate error code and message on failure.
 */
void db::sync() {
    std::error_code ec;
    auto r = sync(ec);
    (void)r;
    assert(r == !ec);
    if(ec)
        throw std::system_error(ec, "could not sync db");
}

/*!
 * \param[out] ec Set to an appropriate error code on failure.
 * \return Valid BSON document on success, empty vector on failure.
 */
std::vector<char> db::metadata(std::error_code& ec) {
    if(!m_db) {
        ec = error();
        return {};
    }
    auto vec = c_ejdb::metadb(m_db.get());
    if(vec.empty())
        ec = error();
    return vec;
}

/*!
 * \return Valid BSON document.
 * \throws std::system_error with appropriate error code and message on failure.
 */
std::vector<char> db::metadata() {
    std::error_code ec;
    auto meta = metadata(ec);
    assert(meta.empty() != !ec);
    if(ec)
        throw std::system_error(ec, "could not get metadata");
    return meta;
}

collection::collection(std::weak_ptr<EJDB> db, EJCOLL* coll) noexcept : m_db(db), m_coll(coll) {}

collection::operator bool() const noexcept { return !m_db.expired() && m_coll != nullptr; }

/*!
 * \brief collection::save_document
 * \param data BSON document to be saved.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return OID of saved document on success, boost::none on failure.
 */
boost::optional<std::array<char, 12>> collection::save_document(const std::vector<char>& data, std::error_code& ec) {
    return save_document(data, false, ec);
}

/*!
 * \param doc BSON document to be saved.
 * \param merge Whether or not to merge with an existing, matching document.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return OID of saved document on success, boost::none on failure.
 */
boost::optional<std::array<char, 12>> collection::save_document(const std::vector<char>& doc, bool merge,
                                                                std::error_code& ec) {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::operation_not_permitted);
        return boost::none;
    }

    std::array<char, 12> oid;
    int err{0};
    const auto r = c_ejdb::savebson(m_coll, doc, oid.data(), merge, &err);
    if(!r) {
        if(err)
            ec = make_error_code((ejdb::errc)err);
        else
            ec = db::error(m_db);
        return boost::none;
    }
    return oid;
}

/*!
 * \param data BSON document to be saved.
 * \param merge Whether or not to merge with an existing, matching document. Default = false.
 * \return OID of saved document.
 *
 * \throws std::system_error with appropriate error code and message on failure.
 */
std::array<char, 12> collection::save_document(const std::vector<char>& data, bool merge) {
    std::error_code ec;
    auto oid = save_document(data, merge, ec);
    assert(static_cast<bool>(oid) == !ec);
    if(ec)
        throw std::system_error(ec, "could not save document");
    return *oid;
}

/*!
 * \param oid OID of the document to fetch.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return Document corresponding to \p oid on success, empty vector on failure or if \p oid has no match.
 */
std::vector<char> collection::load_document(std::array<char, 12> oid, std::error_code& ec) const {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::operation_not_permitted);
        return {};
    }

    auto vec = c_ejdb::loadbson(m_coll, oid.data());
    if(vec.empty())
        ec = db::error(m_db);
    return vec;
}

/*!
 * \param oid OID of the document to fetch.
 * \return Document corresponding to \p oid. Or empty vector if \p oid has no match.
 *
 * \throws std::system_error with appropriate error code and message on failure.
 */
std::vector<char> collection::load_document(std::array<char, 12> oid) const {
    std::error_code ec;
    auto doc = load_document(oid, ec);
    if(ec)
        throw std::system_error(ec, "could not load document");
    return doc;
}

/*!
 * \param oid OID of the document to remove.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return true on success, false on failure.
 */
bool collection::remove_document(std::array<char, 12> oid, std::error_code& ec) noexcept {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::operation_not_permitted);
        return false;
    }
    const auto r = c_ejdb::rmbson(m_coll, oid.data());
    if(!r)
        ec = db::error(m_db);
    return r;
}

/*!
 * \param oid OID of the document to remove.
 *
 * \throws std::system_error with appropriate error code and message on failure.
 */
void collection::remove_document(std::array<char, 12> oid) {
    std::error_code ec;
    auto r = remove_document(oid, ec);
    (void)r;
    assert(static_cast<bool>(r) == !ec);
    if(ec)
        throw std::system_error(ec, "could not remove document");
}

/*!
 * EJDB's setindex documentation follows.
 *
 *  - Available index types:
 *      - `index_mode::string`: String index for JSON string values.
 *      - `index_mode::istring`: Case insensitive string index for JSON string values.
 *      - `index_mode::number`: Index for JSON number values.
 *      - `index_mode::array`: Token index for JSON arrays and string values.
 *
 *  - One JSON field can have several indexes for different types.
 *
 *  - Available index operations:
 *      - `index_mode::drop` Drop index of specified type.
 *              - Eg: flag = index_mode::drop | index_mode::number (Drop number index)
 *      - `index_mode::drop_all` Drop index for all types.
 *      - `index_mode::rebuild` Rebuild index of specified type.
 *      - `index_mode::optimize` Optimize index of specified type. (Optimize the B+ tree index file)
 *
 *  Examples:
 *      - Set index for JSON path `"addressbook.number"` for strings and numbers:
 *          \code coll.set_index("album.number", index_mode::string | index_mode::number) \endcode
 *      - Set index for array:
 *          \code coll.set_index("album.tags", index_mode::array) \endcode
 *      - Rebuild previous index:
 *          \code coll.set_index("album.tags", index_mode::array | index_mode::rebuild) \endcode
 *
 * \param ipath Field path to set index on.
 * \param flags ejdb::index_mode flags controlling the mode of operation of the index.
 * \param[out] ec Set to an appropriate error code on failure.
 * \return true on success, false on failure.
 */
bool collection::set_index(const std::string& ipath, index_mode flags, std::error_code& ec) noexcept {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::operation_not_permitted);
        return false;
    }
    const auto r = c_ejdb::setindex(m_coll, ipath.c_str(), (std::underlying_type_t<index_mode>)flags);
    if(!r)
        ec = db::error(m_db);
    return r;
}

/*!
 * \param ipath Field path to set index on.
 * \param flags ejdb::index_mode flags controlling the mode of operation of the index.
 *
 * \throws std::system_error with appropriate error code and message on failure.
 * \sa set_index
 */
void collection::set_index(const std::string& ipath, index_mode flags) {
    std::error_code ec;
    auto r = set_index(ipath, flags, ec);
    (void)r;
    assert(static_cast<bool>(r) == !ec);
    if(ec)
        throw std::system_error(ec, "could not set index for field "s + ipath);
}

/*!
 * \brief execute_query<query_search_mode::normal>. Executes a query in normal mode.
 *
 * \return All records which match the criteria in \p qry.
 *         If collection or \p qry is invalid, an empty vector is returned.
 */
template <> std::vector<std::vector<char>> collection::execute_query<query_search_mode::normal>(const query& qry) {
    if(m_coll == nullptr || !qry)
        return {};
    assert(qry.m_qry);

    auto db = m_db.lock();
    if(!db)
        return {};

    uint32_t s{0u};
    const auto list = c_ejdb::qryexecute(m_coll, qry.m_qry.get(), &s, 0);
    if(list == nullptr)
        return {};
    assert(s == static_cast<decltype(s)>(c_ejdb::qresultnum(list)));

    std::vector<std::vector<char>> vec;
    vec.reserve(s);
    int ns{0};
    for(uint32_t i = 0; i < s; i++) {
        auto data = reinterpret_cast<const char*>(c_ejdb::qresultbsondata(list, i, &ns));
        if(data == nullptr) {
            s--;
            continue;
        }
        vec.emplace_back(data, data + ns);
    }
    assert(vec.size() == s);
    c_ejdb::qresultdispose(list);
    return vec;
}

/*!
 * \brief execute_query<query_search_mode::count_only>. Executes a query in count-only mode.
 *
 * \return Number of records which match the criteria in \p qry.
 */
template <> uint32_t collection::execute_query<query_search_mode::count_only>(const query& qry) {
    if(m_coll == nullptr || !qry)
        return 0;

    auto db = m_db.lock();
    if(!db)
        return 0;

    assert(qry.m_qry);
    uint32_t s{0u};
    const auto list = c_ejdb::qryexecute(m_coll, qry.m_qry.get(), &s,
                                         (std::underlying_type_t<query_search_mode>)query_search_mode::count_only);
    if(list != nullptr)
        c_ejdb::qresultdispose(list);
    return s;
}

/*!
 * \brief execute_query<query_search_mode::first_only>. Executes a query in first-only mode.
 *
 * \return Only the first record which matches the criteria in \p qry, or boost::none on failure or if none match.
 */
template <> std::vector<char> collection::execute_query<query_search_mode::first_only>(const query& qry) {
    if(m_coll == nullptr || !qry)
        return {};
    assert(qry.m_qry);

    auto db = m_db.lock();
    if(!db)
        return {};

    uint32_t s{0u};
    const auto list = c_ejdb::qryexecute(m_coll, qry.m_qry.get(), &s,
                                         (std::underlying_type_t<query_search_mode>)query_search_mode::first_only);
    if(list == nullptr || s == 0)
        return {};
    assert(s == static_cast<decltype(s)>(c_ejdb::qresultnum(list)));
    assert(s == 1);

    int ns{0};
    auto data = reinterpret_cast<const char*>(c_ejdb::qresultbsondata(list, 0, &ns));
    auto doc = std::vector<char>(data, data + ns);
    c_ejdb::qresultdispose(list);

    return std::move(doc);
}

/*!
 * \brief execute_query<query_search_mode::count_only\|query_search_mode::first_only>.
 *        Executes a query in count-only mode, but only counts up to one (1).
 *
 * \return One (1) when at least one documents match the criteria in \p qry, otherwise zero (0).
 */
template <>
uint32_t collection::execute_query<query_search_mode::count_only | query_search_mode::first_only>(const query& qry) {
    if(m_coll == nullptr || !qry)
        return 0;
    assert(qry.m_qry);

    auto db = m_db.lock();
    if(!db)
        return 0;

    uint32_t s{0u};
    const auto list = c_ejdb::qryexecute(
        m_coll, qry.m_qry.get(), &s,
        (std::underlying_type_t<query_search_mode>)(query_search_mode::count_only | query_search_mode::first_only));
    if(list != nullptr)
        c_ejdb::qresultdispose(list);
    return s;
}

std::vector<std::vector<char>> collection::get_all() {
    auto db = m_db.lock();
    if(!db)
        return {};
    static constexpr std::array<char, 5> vec{{5, 0, 0, 0, 0}}; // empty doc
    auto q = query{m_db, c_ejdb::createquery(db.get(), vec.data())};
    return execute_query(q);
}

/*!
 * \param[out] ec Set to an appropriate error code on failure.
 * \return true on success, false on failure.
 */
bool collection::sync(std::error_code& ec) noexcept {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::operation_not_permitted);
        return false;
    }
    const auto r = c_ejdb::syncoll(m_coll);
    if(!r)
        ec = db::error(m_db);
    return r;
}

/*!
 * \throws std::system_error with appropriate error code and message on failure.
 */
void collection::sync() {
    std::error_code ec;
    auto r = sync(ec);
    (void)r;
    assert(r == !ec);
    if(ec)
        throw std::system_error(ec, "could not sync collection");
}

boost::string_ref collection::name() const noexcept {
    if(m_coll == nullptr)
        return {};
    return c_ejdb::collection_name(m_coll);
}

query::query(std::weak_ptr<EJDB> db, EJQ* qry) noexcept : m_db(db), m_qry(qry) {}

/*!
 * \throws std::system_error with std::errc::operation_not_permitted when query is null.
 */
query& query::operator|=(const std::vector<char>& obj) & {
    if(!m_qry)
        throw std::system_error(make_error_code(std::errc::operation_not_permitted), "null query");
    auto db = m_db.lock();
    if(!db)
        return *this;

    auto q = c_ejdb::queryaddor(db.get(), m_qry.get(), obj.data());
    if(q != m_qry.get())
        m_qry.reset(q);

    return *this;
}

/*!
 * \throws std::system_error with std::errc::operation_not_permitted when query is null.
 */
query&& query::operator|=(const std::vector<char>& obj) && { return std::move(*this |= obj); }

/*!
 * EJDB's hints documentation follows.
 *
 *  Available hints:
 *      - $max Maximum number in the result set
 *      - $skip Number of skipped results in the result set
 *      - $orderby Sorting order of query fields.
 *      - $fields Set subset of fetched fields
 *          If a field presented in $orderby clause it will be forced to include in resulting records.
 *          Example:
 * \code
        {
            "$orderby" : { //ORDER BY field1 ASC, field2 DESC
                "field1" : 1,
                "field2" : -1
            },
            "$fields" : { //SELECT ONLY {_id, field1, field2}
                "field1" : 1,
                "field2" : 1
            }
        }
 \endcode
 */
query& query::set_hints(const std::vector<char>& obj) & {
    assert(m_qry);
    auto db = m_db.lock();
    if(!db)
        return *this;
    auto q = c_ejdb::queryhints(db.get(), m_qry.get(), obj.data());
    if(q != m_qry.get())
        m_qry.reset(q);

    return *this;
}

query&& query::set_hints(const std::vector<char>& obj) && { return std::move(set_hints(obj)); }

query::operator bool() const noexcept { return !m_db.expired() && m_qry != nullptr; }

//! The category type used for all EJDB errors.
class error_category : public std::error_category {
  public:
    //! Default constructor.
    constexpr error_category() noexcept = default;

    //! Returns name of category (`"EJDB"`).
    const char* name() const noexcept override { return "EJDB"; }

    //! Returns message associated with \p ecode.
    std::string message(int ecode) const noexcept override { return c_ejdb::errmsg(ecode); }
};

std::error_code make_error_code(errc ecode) noexcept {
    static const ::ejdb::error_category ecat{};
    return std::error_code{static_cast<int>(ecode), ecat};
}

collection::transaction_t& collection::transaction() noexcept { return m_transaction; }

collection::transaction_t::transaction_t(collection* coll) noexcept : m_collection(coll), m_db(m_collection->m_db) {}

/*!
 * Any operation performed on the associated collection after a successful call will be part of the transaction.
 *
 * Must be accompanied by a call to transaction_t::commit or transaction_t::abort in order to end a transaction.
 *
 * Parent db object may have a related error code (via db::error) on failure.
 * \return true on success, false on failure.
 */
bool collection::transaction_t::start() noexcept {
    auto db = m_db.lock();
    return db && c_ejdb::isopen(db.get()) && m_collection && *m_collection && c_ejdb::tranbegin(m_collection->m_coll);
}

/*!
 * Abandons all changes made to the associated collection after a call to transaction_t::start.
 *
 * Parent db object may have a related error code (via db::error) on failure.
 * \return true on success, false on failure.
 */
bool collection::transaction_t::abort() noexcept {
    auto db = m_db.lock();
    return db && c_ejdb::isopen(db.get()) && m_collection && *m_collection && c_ejdb::tranabort(m_collection->m_coll);
}

/*!
 * Commits all changes made to the associated collection after a call to transaction_t::start.
 *
 * Parent db object may have a related error code (via db::error) on failure.
 * \return true on success, false on failure.
 */
bool collection::transaction_t::commit() noexcept {
    auto db = m_db.lock();
    return db && c_ejdb::isopen(db.get()) && m_collection && *m_collection && c_ejdb::trancommit(m_collection->m_coll);
}

/*!
 * \return true when parent db is alive, open, and the transaction is in progress,
 *         i.e. started, but neither committed nor aborted.
 */
bool collection::transaction_t::in_transaction() const noexcept {
    auto db = m_db.lock();
    bool ret{};
    if(db && c_ejdb::isopen(db.get()) && m_collection && *m_collection &&
       c_ejdb::transtatus(m_collection->m_coll, &ret))
        return ret;
    return false;
}

collection::transaction_t::operator bool() const noexcept { return in_transaction(); }

/*!
 * \throws std::system_error with an ejdb::errc when the transaction could not be started.
 */
unique_transaction::unique_transaction(collection::transaction_t& trans)
    : m_trans(&trans), m_owns(true), m_db(m_trans->m_db) {
    if(!m_trans->start())
        throw std::system_error((ejdb::errc)c_ejdb::ecode(m_db.get()), "could not start transaction");
}

unique_transaction::unique_transaction(collection::transaction_t& trans, adopt_transaction_t) noexcept
    : m_trans(&trans),
      m_owns(m_trans->in_transaction()),
      m_db(m_trans->m_db) {}

unique_transaction::unique_transaction(collection::transaction_t& trans, try_transaction_t) noexcept
    : m_trans(&trans),
      m_owns(m_trans->start()),
      m_db(m_trans->m_db) {}

/*!
 * \param other unique_transaction to construct from.
 *
 * Ensures \p other neither owns nor is associated with a collection::transaction_t,
 * avoiding erroneous transaction termination.
 */
unique_transaction::unique_transaction(unique_transaction&& other) noexcept : m_trans(other.m_trans),
                                                                              m_owns(other.m_owns),
                                                                              m_db(std::move(other.m_db)) {
    other.m_trans = nullptr;
    other.m_owns = false;
}

/*!
 * \param other unique_transaction to assign from.
 *
 * Aborts current transaction if one is owned.
 *
 * Ensures \p other neither owns nor is associated with a collection::transaction_t,
 * avoiding erroneous transaction termination.
 *
 * \throws whatever abort() may throw.
 */
unique_transaction& unique_transaction::operator=(unique_transaction&& other) {
    if(m_owns)
        abort();
    m_trans = other.m_trans;
    m_owns = other.m_owns;
    m_db = std::move(other.m_db);
    other.m_trans = nullptr;
    other.m_owns = false;

    return *this;
}

unique_transaction::~unique_transaction() {
    if(!m_owns)
        return;
    bool r{};
    if(std::uncaught_exception())
        r = m_trans->abort();
    else
        r = m_trans->commit();
    (void)r;
    assert(r);
}

/*!
 * \throws std::system_error with std::errc::operation_not_permitted when referenced collection::transaction_t is null.
 * \throws std::system_error with errc::illegal_transaction_state when the transaction has already been started.
 * \throws std::system_error with an ejdb::errc when the transaction could not be started.
 */
void unique_transaction::start() {
    if(m_trans == nullptr)
        throw std::system_error(make_error_code(std::errc::operation_not_permitted), "null transaction");
    if(m_owns || m_trans->in_transaction())
        throw std::system_error(ejdb::errc::illegal_transaction_state, "transaction already started");

    m_owns = m_trans->start();
    if(!m_owns)
        throw std::system_error((ejdb::errc)c_ejdb::ecode(m_db.get()), "could not start transaction");
}

/*!
 * \throws std::system_error with std::errc::operation_not_permitted when referenced collection::transaction_t is null.
 * \throws std::system_error with errc::illegal_transaction_state when the transaction has already been terminated.
 * \throws std::system_error with an ejdb::errc when the transaction could not be committed.
 */
void unique_transaction::commit() {
    if(m_trans == nullptr)
        throw std::system_error(make_error_code(std::errc::operation_not_permitted), "null transaction");
    if(!m_owns || !m_trans->in_transaction())
        throw std::system_error(ejdb::errc::illegal_transaction_state);
    m_owns = !m_trans->commit();
    if(m_owns)
        throw std::system_error((ejdb::errc)c_ejdb::ecode(m_db.get()), "could not commit transaction");
}

/*!
 * \throws std::system_error with std::errc::operation_not_permitted when referenced collection::transaction_t is null.
 * \throws std::system_error with errc::illegal_transaction_state when the transaction has already been terminated.
 * \throws std::system_error with an ejdb::errc when the transaction could not be aborted.
 */
void unique_transaction::abort() {
    if(m_trans == nullptr)
        throw std::system_error(make_error_code(std::errc::operation_not_permitted), "null transaction");
    if(!m_owns || !m_trans->in_transaction())
        throw std::system_error(ejdb::errc::illegal_transaction_state);
    m_owns = !m_trans->abort();
    if(m_owns)
        throw std::system_error((ejdb::errc)c_ejdb::ecode(m_db.get()), "could not abort transaction");
}

collection::transaction_t* unique_transaction::release() noexcept {
    auto ret = m_trans;
    m_trans = nullptr;
    m_owns = false;
    m_db.reset();

    return ret;
}

bool unique_transaction::owns_transaction() const noexcept { return m_owns && m_trans->in_transaction(); }

ejdb::unique_transaction::operator bool() const noexcept { return owns_transaction(); }

transaction_guard::transaction_guard(collection::transaction_t& trans) : unique_transaction(trans) {}

} // namespace ejdb
