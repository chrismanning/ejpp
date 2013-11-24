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

#include <tcejdb/ejdb.h>

#include <boost/optional.hpp>

#include "ejdb.hpp"

namespace ejdb {

struct ejdb_deleter {
    void operator()(EJDB* ptr) const { ejdbdel(ptr); }
};

void query::eqry_deleter::operator()(EJQ* ptr) const
{ ejdbquerydel(ptr); }

ejdb::ejdb() : db(ejdbnew(), ejdb_deleter()) {}

ejdb::ejdb(const std::string& path, int mode) : ejdb() {
    std::error_code ec;
    auto b = open(path, mode, ec);
    assert(b ? !ec : !!ec);
}

ejdb::operator bool() const noexcept { return static_cast<bool>(db); }

std::error_code ejdb::error() const noexcept { return make_error_code(ejdbecode(db.get())); }

bool ejdb::open(const std::string& path, int mode, std::error_code& ec) noexcept {
    const auto r = ejdbopen(db.get(), path.c_str(), mode);
    ec = make_error_code(!r ? ejdbecode(db.get()) : 0);
    return r;
}

bool ejdb::is_open() const noexcept { return ejdbisopen(db.get()); }

bool ejdb::close(std::error_code& ec) noexcept {
    const auto r = ejdbclose(db.get());
    ec = make_error_code(!r ? ejdbecode(db.get()) : 0);
    return r;
}

collection ejdb::get_collection(const std::string& name, std::error_code& ec) const noexcept {
    const auto r = ejdbgetcoll(db.get(), name.c_str());
    ec = make_error_code(!r ? ejdbecode(db.get()) : 0);
    return { db, r };
}

collection ejdb::create_collection(const std::string& name, std::error_code& ec) noexcept {
    const auto r = ejdbcreatecoll(db.get(), name.c_str(), nullptr);
    ec = make_error_code(!r ? ejdbecode(db.get()) : 0);
    return { db, r };
}

bool ejdb::remove_collection(const std::string& name, bool unlink_file, std::error_code& ec) noexcept {
    const auto r = ejdbrmcoll(db.get(), name.c_str(), unlink_file);
    ec = make_error_code(!r ? ejdbecode(db.get()) : 0);
    return r;
}

query ejdb::create_query(const std::string& json, std::error_code& ec) noexcept {
    const auto r = ejdbcreatequery(db.get(), json2bson(json.c_str()), nullptr, 0, nullptr);
    ec = make_error_code(!r ? ejdbecode(db.get()) : 0);
    return {db, r};
}

collection::collection(std::shared_ptr<EJDB> db, EJCOLL* coll) noexcept : db(db), coll(coll) {}

collection::operator bool() const noexcept {
    return db && coll != nullptr;
}

boost::optional<bson_oid_t> collection::save_json(const std::string& json, std::error_code& ec) {
    return save_json(json, false, ec);
}

boost::optional<bson_oid_t> collection::save_json(const std::string& json, bool merge, std::error_code& ec) {
    assert(coll);
    bson_oid_t oid;
    const auto r = ejdbsavebson2(coll, json2bson(json.c_str()), &oid, merge);
    ec = make_error_code(!r ? ejdbecode(db.get()) : 0);
    return r ? oid : boost::optional<bson_oid_t>{ boost::none };
}

query::query(std::shared_ptr<EJDB> db, EJQ* qry) noexcept : db(db), qry(qry) {}

query::~query() noexcept {}

query::operator bool() const noexcept {
     return qry != nullptr;
}

class error_category : public std::error_category {
  public:
    constexpr error_category() noexcept = default;

    const char* name() const noexcept override { return "EJDB"; }

    std::string message(int ecode) const noexcept override { return ejdberrmsg(ecode); }
};

inline std::error_code make_error_code(int ecode) {
    static const error_category ecat{};
    return std::error_code{ ecode, ecat };
}

}// namespace ejdb
