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
#include <vector>

#include <boost/optional/optional_fwd.hpp>

#include "bson-cpp/inc/bsonobj.h"

struct EJDB;
struct EJCOLL;
struct EJQ;

namespace ejdb {
struct collection;
struct query;

class error_category;
inline std::error_code make_error_code(int ecode);

struct db_mode {
    enum {                    /** Database open modes */
          JBOREADER = 1 << 0, /**< Open as a reader. */
          JBOWRITER = 1 << 1, /**< Open as a writer. */
          JBOCREAT = 1 << 2,  /**< Create if db file not exists. */
          JBOTRUNC = 1 << 3,  /**< Truncate db on open. */
          JBONOLCK = 1 << 4,  /**< Open without locking. */
          JBOLCKNB = 1 << 5,  /**< Lock without blocking. */
          JBOTSYNC = 1 << 6   /**< Synchronize every transaction. */
    };
};

struct ejdb {
    ejdb();
    explicit ejdb(const std::string& path, int mode);

    explicit operator bool() const noexcept;
    std::error_code error() const noexcept;

    bool open(const std::string& path, int mode, std::error_code& ec) noexcept;
    bool is_open() const noexcept;
    bool close(std::error_code& ec) noexcept;

    collection get_collection(const std::string& name, std::error_code& ec) const noexcept;
    collection create_collection(const std::string& name, std::error_code& ec) noexcept;
    bool remove_collection(const std::string& name, bool unlink_file, std::error_code& ec) noexcept;

    query create_query(const bson::BSONObj& doc, std::error_code& ec) noexcept;

    bool sync(std::error_code& ec) noexcept;

  private:
    std::shared_ptr<EJDB> m_db;
};

struct collection {
    collection() noexcept = default;

    explicit operator bool() const noexcept;

    boost::optional<bson::OID> save_document(const bson::BSONObj& data, std::error_code& ec) noexcept;
    boost::optional<bson::OID> save_document(const bson::BSONObj& data, bool merge, std::error_code& ec) noexcept;
    boost::optional<bson::BSONObj> load_document(bson::OID oid, std::error_code& ec) const noexcept;
    bool remove_document(bson::OID, std::error_code& ec) noexcept;

    bool set_index(const std::string& ipath, int flags) noexcept;
    std::vector<void> execute_query(const query&) noexcept;
    uint32_t count_query(const query&) noexcept;

    bool sync(std::error_code& ec) noexcept;

  private:
    friend struct ejdb;
    collection(std::weak_ptr<EJDB> m_db, EJCOLL* coll) noexcept;

    std::weak_ptr<EJDB> m_db;
    EJCOLL* coll{nullptr};
};

struct query {
    query() noexcept = default;
    ~query() noexcept;

    explicit operator bool() const noexcept;

    query& operator|=(const bson::BSONObj&) noexcept;

  private:
    friend struct ejdb;
    friend struct collection;
    query(std::weak_ptr<EJDB> m_db, EJQ* qry) noexcept;

    std::weak_ptr<EJDB> m_db;

    struct eqry_deleter {
        void operator()(EJQ* ptr) const;
    };
    std::unique_ptr<EJQ, eqry_deleter> qry;
};

} // namespace ejdb

#endif // EJDB_HPP
