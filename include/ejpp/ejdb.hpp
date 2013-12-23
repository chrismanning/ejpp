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

class error_category;
inline std::error_code make_error_code(int ecode);

struct db_mode final {
    enum {                    /** Database open modes */
          read = 1 << 0,      /**< Open as a reader. */
          write = 1 << 1,     /**< Open as a writer. */
          create = 1 << 2,    /**< Create if db file not exists. */
          truncate = 1 << 3,  /**< Truncate db on open. */
          nolock = 1 << 4,    /**< Open without locking. */
          noblock = 1 << 5,   /**< Lock without blocking. */
          trans_sync = 1 << 6 /**< Synchronize every transaction. */
    };
};

struct ejdb final {
    ejdb();

    explicit operator bool() const noexcept;
    std::error_code error() const noexcept;

    bool open(const std::string& path, int mode, std::error_code& ec) noexcept;
    bool is_open() const noexcept;
    bool close(std::error_code& ec) noexcept;

    collection get_collection(const std::string& name, std::error_code& ec) const noexcept;
    collection create_collection(const std::string& name, std::error_code& ec) noexcept;
    bool remove_collection(const std::string& name, bool unlink_file, std::error_code& ec) noexcept;
    const std::deque<collection> get_collections() const noexcept;

    query create_query(const jbson::document& doc, std::error_code& ec) noexcept;

    bool sync(std::error_code& ec) noexcept;

    boost::optional<jbson::document> metadata() noexcept;

  private:
    std::shared_ptr<EJDB> m_db;
};

struct query final {
    query() noexcept = default;
    ~query() noexcept;

    query(query&&) noexcept = default;
    query& operator=(query&&) noexcept = default;

    enum search_mode {
        /*< Query search mode flags */
        count_only = 1,
        first_only = 1 << 1
    };

    explicit operator bool() const noexcept;

    query& operator|=(const jbson::document&) noexcept;
    query& set_hints(const jbson::document&) noexcept;

  private:
    friend struct ejdb;
    friend struct collection;
    query(std::weak_ptr<EJDB> m_db, EJQ* m_qry) noexcept;

    std::weak_ptr<EJDB> m_db;

    struct eqry_deleter {
        void operator()(EJQ* ptr) const;
    };
    std::unique_ptr<EJQ, eqry_deleter> m_qry;
};

struct collection final {
    collection() noexcept = default;

    explicit operator bool() const noexcept;

    boost::optional<std::array<char,12>> save_document(const jbson::document& data, std::error_code& ec) noexcept;
    boost::optional<std::array<char,12>> save_document(const jbson::document& data, bool merge, std::error_code& ec) noexcept;
    boost::optional<jbson::document> load_document(std::array<char,12> oid, std::error_code& ec) const noexcept;
    bool remove_document(std::array<char,12>, std::error_code& ec) noexcept;

    bool set_index(const std::string& ipath, int flags) noexcept;
    std::vector<jbson::document> execute_query(const query&, int flags = 0) noexcept;

    std::vector<jbson::document> get_all() noexcept;

    bool sync(std::error_code& ec) noexcept;

  private:
    friend struct ejdb;
    collection(std::weak_ptr<EJDB> m_db, EJCOLL* m_coll) noexcept;

    std::weak_ptr<EJDB> m_db;
    EJCOLL* m_coll{nullptr};
};

} // namespace ejdb

#endif // EJDB_HPP
