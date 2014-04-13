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
#include <cassert>
#include <deque>

#include <boost/optional/optional_fwd.hpp>

#include <jbson/document.hpp>

struct EJDB;
struct EJCOLL;
struct EJQ;

namespace ejdb {
struct collection;
struct query;

struct db_mode final {
    enum {                     /** Database open modes */
           read = 1 << 0,      /**< Open as a reader. */
           write = 1 << 1,     /**< Open as a writer. */
           create = 1 << 2,    /**< Create if db file not exists. */
           truncate = 1 << 3,  /**< Truncate db on open. */
           nolock = 1 << 4,    /**< Open without locking. */
           noblock = 1 << 5,   /**< Lock without blocking. */
           trans_sync = 1 << 6 /**< Synchronize every transaction. */
    };
};

struct index_mode final {
    enum {                    /** Index modes, index types. */
           drop = 1 << 0,     /**< Drop index. */
           drop_all = 1 << 1, /**< Drop index for all types. */
           optimize = 1 << 2, /**< Optimize indexes. */
           rebuild = 1 << 3,  /**< Rebuild index. */
           number = 1 << 4,   /**< Number index. */
           string = 1 << 5,   /**< String index.*/
           array = 1 << 6,    /**< Array token index. */
           istring = 1 << 7   /**< Case insensitive string index */
    };
};

enum class errc {                                     /** Error codes */
                  invalid_collection_name = 9000,     /**< Invalid collection name. */
                  invalid_bson = 9001,                /**< Invalid bson object. */
                  invalid_bson_oid = 9002,            /**< Invalid bson object id. */
                  invalid_query_control_field = 9003, /**< Invalid query control field starting with '$'. */
                  query_field_require_array =
                      9004,                /**< $strand, $stror, $in, $nin, $bt keys requires not empty array value. */
                  invalid_metadata = 9005, /**< Inconsistent database metadata. */
                  invalid_field_path = 9006,               /**< Invalid field path value. */
                  invalid_query_regex = 9007,              /**< Invalid query regexp value. */
                  query_result_sort_error = 9008,          /**< Result set sorting error. */
                  query_error = 9009,                      /**< Query generic error. */
                  query_update_failed = 9010,              /**< Updating failed. */
                  query_elemmatch_limit = 9011,            /**< Only one $elemMatch allowed in the fieldpath. */
                  query_cannot_mix_include_exclude = 9012, /**< $fields hint cannot mix include and exclude fields */
                  query_invalid_action = 9013,             /**< action key in $do block can only be one of: $join */
                  too_many_collections = 9014, /**< Exceeded the maximum number of collections per database */
                  import_export_error = 9015,  /**< EJDB export/import error */
                  json_parse_failed = 9016,    /**< JSON parsing failed */
                  bson_too_large = 9017,       /**< BSON size is too big */
                  invalid_command = 9018       /**< Invalid ejdb command specified */
};

std::error_code make_error_code(errc ecode);

} // namespace ejdb

namespace std {
    template <>
    struct is_error_code_enum<ejdb::errc> : public true_type {};
}

namespace ejdb {

struct db final {
    db() noexcept = default;

    explicit operator bool() const noexcept;
    std::error_code error() const noexcept;
    static std::error_code error(std::weak_ptr<EJDB>) noexcept;

    bool open(const std::string& path, int mode, std::error_code& ec) noexcept;
    bool is_open() const noexcept;
    bool close(std::error_code& ec) noexcept;

    collection get_collection(const std::string& name, std::error_code& ec) const noexcept;
    collection create_collection(const std::string& name, std::error_code& ec) noexcept;
    bool remove_collection(const std::string& name, bool unlink_file, std::error_code& ec) noexcept;
    const std::vector<collection> get_collections() const noexcept;

    query create_query(const jbson::document& doc, std::error_code& ec) noexcept;

    bool sync(std::error_code& ec) noexcept;

    boost::optional<jbson::document> metadata(std::error_code& ec);

  private:
    std::shared_ptr<EJDB> m_db;
};

struct collection final {
    collection() noexcept = default;

    explicit operator bool() const noexcept;

    boost::optional<std::array<char, 12>> save_document(const jbson::document& data, std::error_code& ec);
    boost::optional<std::array<char, 12>> save_document(const jbson::document& data, bool merge, std::error_code& ec);
    boost::optional<jbson::document> load_document(std::array<char, 12> oid, std::error_code& ec) const;
    bool remove_document(std::array<char, 12>, std::error_code& ec) noexcept;

    bool set_index(const std::string& ipath, int flags, std::error_code& ec) noexcept;
    std::vector<jbson::document> execute_query(const query&, int flags = 0);

    std::vector<jbson::document> get_all();

    bool sync(std::error_code& ec) noexcept;

    boost::string_ref name() const noexcept;

  private:
    friend struct db;
    collection(std::weak_ptr<EJDB> m_db, EJCOLL* m_coll) noexcept;

    std::weak_ptr<EJDB> m_db;
    EJCOLL* m_coll{nullptr};
};

struct query final {
    query() noexcept = default;
    ~query() noexcept;

    query(query&&) noexcept = default;
    query& operator=(query&&)&noexcept = default;
    query&& operator=(query&&)&&noexcept;

    enum search_mode {
        /*< Query search mode flags */
        count_only = 1,
        first_only = 1 << 1
    };

    explicit operator bool() const noexcept;

    // $and
    query& operator&=(const jbson::document&)&;
    query&& operator&=(const jbson::document&)&&;
    query& operator&=(query) & noexcept;
    query&& operator&=(query) && noexcept;

    // $or
    query& operator|=(const jbson::document&)&;
    query&& operator|=(const jbson::document&)&&;
    query& operator|=(query) & noexcept;
    query&& operator|=(query) && noexcept;

    query& set_hints(const jbson::document&)&;
    query&& set_hints(const jbson::document&)&&;

  private:
    friend struct db;
    friend struct collection;
    query(std::weak_ptr<EJDB> m_db, EJQ* m_qry) noexcept;

    std::weak_ptr<EJDB> m_db;

    struct eqry_deleter {
        void operator()(EJQ* ptr) const;
    };
    std::unique_ptr<EJQ, eqry_deleter> m_qry;
};

} // namespace ejdb

#endif // EJDB_HPP
