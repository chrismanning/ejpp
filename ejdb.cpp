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

#include <boost/optional.hpp>

#include "c_ejdb.hpp"
#include "ejdb.hpp"

namespace ejdb {

struct ejdb_deleter {
    void operator()(EJDB* ptr) const { c_ejdb::del(ptr); }
};

void query::eqry_deleter::operator()(EJQ* ptr) const { c_ejdb::querydel(ptr); }

ejdb::ejdb() : m_db(c_ejdb::newdb(), ejdb_deleter()) {}

ejdb::ejdb(const std::string& path, int mode) : ejdb() {
    std::error_code ec;
    auto b = open(path, mode, ec);
    assert(b ? !ec : !!ec);
}

ejdb::operator bool() const noexcept { return static_cast<bool>(m_db); }

std::error_code ejdb::error() const noexcept { return make_error_code(c_ejdb::ecode(m_db.get())); }

bool ejdb::open(const std::string& path, int mode, std::error_code& ec) noexcept {
    const auto r = c_ejdb::open(m_db.get(), path.c_str(), mode);
    ec = make_error_code(!r ? c_ejdb::ecode(m_db.get()) : 0);
    return r;
}

bool ejdb::is_open() const noexcept { return c_ejdb::isopen(m_db.get()); }

bool ejdb::close(std::error_code& ec) noexcept {
    const auto r = c_ejdb::closedb(m_db.get());
    ec = make_error_code(!r ? c_ejdb::ecode(m_db.get()) : 0);
    return r;
}

collection ejdb::get_collection(const std::string& name, std::error_code& ec) const noexcept {
    const auto r = c_ejdb::getcoll(m_db.get(), name.c_str());
    ec = make_error_code(!r ? c_ejdb::ecode(m_db.get()) : 0);
    return {m_db, r};
}

collection ejdb::create_collection(const std::string& name, std::error_code& ec) noexcept {
    const auto r = c_ejdb::createcoll(m_db.get(), name.c_str(), nullptr);
    ec = make_error_code(!r ? c_ejdb::ecode(m_db.get()) : 0);
    return {m_db, r};
}

bool ejdb::remove_collection(const std::string& name, bool unlink_file, std::error_code& ec) noexcept {
    const auto r = c_ejdb::rmcoll(m_db.get(), name.c_str(), unlink_file);
    ec = make_error_code(!r ? c_ejdb::ecode(m_db.get()) : 0);
    return r;
}

query ejdb::create_query(const bson::BSONObj& doc, std::error_code& ec) noexcept {
    const auto r = c_ejdb::createquery(m_db.get(), doc.objdata());
    ec = make_error_code(!r ? c_ejdb::ecode(m_db.get()) : 0);
    return {m_db, r};
}

bool ejdb::sync(std::error_code& ec) noexcept {
    const auto r = c_ejdb::syncdb(m_db.get());
    ec = make_error_code(!r ? c_ejdb::ecode(m_db.get()) : 0);
    return r;
}

collection::collection(std::weak_ptr<EJDB> db, EJCOLL* coll) noexcept : m_db(db), coll(coll) {}

collection::operator bool() const noexcept { return !m_db.expired() && coll != nullptr; }

boost::optional<bson::OID> collection::save_document(const bson::BSONObj& data, std::error_code& ec) noexcept {
    return save_document(data, false, ec);
}

boost::optional<bson::OID> collection::save_document(const bson::BSONObj& doc, bool merge,
                                                     std::error_code& ec) noexcept {
    assert(coll);
    auto db = m_db.lock();
    if (db && (ec = std::make_error_code(std::errc::owner_dead)))
        return {};
    std::array<char, 12> oid;
    const auto r = c_ejdb::savebson2(coll, doc.objdata(), oid.data(), merge);
    ec = make_error_code(!r ? c_ejdb::ecode(db.get()) : 0);
    return r ? *reinterpret_cast<bson::OID*>(&oid) : boost::optional<bson::OID>{boost::none};
}

boost::optional<bson::BSONObj> collection::load_document(bson::OID oid, std::error_code& ec) const noexcept {
    const auto r = c_ejdb::loadbson(coll, reinterpret_cast<const char*>(oid.getData()));
    auto db = m_db.lock();
    ec = make_error_code(!r && db ? c_ejdb::ecode(db.get()) : 0);
    if (!ec)
        return boost::none;
    return bson::BSONObj(reinterpret_cast<const char*>(r));
}

bool collection::remove_document(bson::OID oid, std::error_code& ec) noexcept {
    const auto r = c_ejdb::rmbson(coll, (char*)oid.getData());
    auto db = m_db.lock();
    if (!r && db)
        ec = std::make_error_code(std::errc::owner_dead);
    return r;
}

bool collection::sync(std::error_code& ec) noexcept {
    const auto r = c_ejdb::syncoll(coll);
    auto db = m_db.lock();
    if (!r && db)
        ec = std::make_error_code(std::errc::owner_dead);
    return r;
}

query::query(std::weak_ptr<EJDB> db, EJQ* qry) noexcept : m_db(db), qry(qry) {}

query::~query() noexcept {}

query& query::operator|=(const bson::BSONObj& obj) noexcept {
    auto db = m_db.lock();
    if (!db)
        return *this;

    auto q = c_ejdb::queryaddor(db.get(), qry.get(), obj.objdata());
    if (q != qry.get())
        qry.reset(q);

    return *this;
}

query::operator bool() const noexcept { return qry != nullptr; }

class error_category : public std::error_category {
  public:
    constexpr error_category() noexcept = default;

    const char* name() const noexcept override { return "EJDB"; }

    std::string message(int ecode) const noexcept override { return c_ejdb::errmsg(ecode); }
};

inline std::error_code make_error_code(int ecode) {
    static const error_category ecat{};
    return std::error_code{ecode, ecat};
}

} // namespace ejdb
