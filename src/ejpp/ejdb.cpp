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
#include <boost/range/adaptor/transformed.hpp>

#include <jbson/document.hpp>
#include <jbson/json_reader.hpp>

#include <ejpp/c_ejdb.hpp>
#include <ejpp/ejdb.hpp>

namespace ejdb {

using namespace jbson::literal;

struct ejdb_deleter {
    void operator()(EJDB* ptr) const { c_ejdb::del(ptr); }
};

void query::eqry_deleter::operator()(EJQ* ptr) const { c_ejdb::querydel(ptr); }

db::operator bool() const noexcept { return static_cast<bool>(m_db); }

std::error_code db::error() const noexcept {
    if(!m_db)
        return std::make_error_code(std::errc::bad_address);
    return static_cast<errc>(c_ejdb::ecode(m_db.get()));
}

std::error_code db::error(std::weak_ptr<EJDB> db) noexcept {
    auto ptr = db.lock();
    if(!ptr)
        return std::make_error_code(std::errc::bad_address);
    return static_cast<errc>(c_ejdb::ecode(ptr.get()));
}

bool db::open(const std::string& path, int mode, std::error_code& ec) noexcept {
    m_db = {c_ejdb::newdb(), ejdb_deleter()};
    const auto r = m_db && c_ejdb::open(m_db.get(), path.c_str(), mode);
    if(!r)
        ec = error();
    return r;
}

bool db::is_open() const noexcept { return m_db && c_ejdb::isopen(m_db.get()); }

bool db::close(std::error_code& ec) noexcept {
    const auto r = m_db && c_ejdb::closedb(m_db.get());
    if(!r)
        ec = error();
    m_db.reset();
    return r;
}

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

bool db::remove_collection(const std::string& name, bool unlink_file, std::error_code& ec) noexcept {
    const auto r = m_db && c_ejdb::rmcoll(m_db.get(), name.c_str(), unlink_file);
    if(!r)
        ec = error();
    return r;
}

const std::vector<collection> db::get_collections() const noexcept {
    if(!m_db)
        return {};
    auto colls = c_ejdb::getcolls(m_db.get());
    auto range = boost::adaptors::transform(colls, [this](EJCOLL* c) -> collection {
        return collection{m_db, c};
    });
    return {range.begin(), range.end()};
}

query db::create_query(const jbson::document& doc, std::error_code& ec) noexcept {
    if(!m_db) {
        ec = error();
        return {};
    }
    const auto r = c_ejdb::createquery(m_db.get(), doc.data().data());
    if(!r) {
        ec = error();
        return query{};
    }
    return query{m_db, r};
}

bool db::sync(std::error_code& ec) noexcept {
    const auto r = m_db && c_ejdb::syncdb(m_db.get());
    if(!r)
        ec = error();
    return r;
}

boost::optional<jbson::document> db::metadata(std::error_code& ec) {
    if(!m_db) {
        ec = error();
        return boost::none;
    }
    auto r = c_ejdb::metadb(m_db.get());
    if(r.empty()) {
        ec = error();
        return boost::none;
    }
    return jbson::document{std::move(r)};
}

collection::collection(std::weak_ptr<EJDB> db, EJCOLL* coll) noexcept : m_db(db), m_coll(coll) {}

collection::operator bool() const noexcept { return !m_db.expired() && m_coll != nullptr; }

boost::optional<std::array<char, 12>> collection::save_document(const jbson::document& data, std::error_code& ec) {
    return save_document(data, false, ec);
}

boost::optional<std::array<char, 12>> collection::save_document(const jbson::document& doc, bool merge,
                                                                std::error_code& ec) {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::bad_address);
        return boost::none;
    }

    std::array<char, 12> oid;
    const auto r = c_ejdb::savebson2(m_coll, doc.data().data(), oid.data(), merge);
    if(!r) {
        ec = db::error(m_db);
        return boost::none;
    }
    return oid;
}

boost::optional<jbson::document> collection::load_document(std::array<char, 12> oid, std::error_code& ec) const {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::bad_address);
        return boost::none;
    }

    auto r = c_ejdb::loadbson(m_coll, oid.data());
    if(r.empty()) {
        ec = db::error(m_db);
        return boost::none;
    }
    return jbson::document{std::move(r)};
}

bool collection::remove_document(std::array<char, 12> oid, std::error_code& ec) noexcept {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::bad_address);
        return false;
    }
    const auto r = c_ejdb::rmbson(m_coll, oid.data());
    if(!r)
        ec = db::error(m_db);
    return r;
}

bool collection::set_index(const std::string& ipath, int flags, std::error_code& ec) noexcept {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::bad_address);
        return false;
    }
    const auto r = c_ejdb::setindex(m_coll, ipath.c_str(), flags);
    if(!r)
        ec = db::error(m_db);
    return r;
}

std::vector<jbson::document> collection::execute_query(const query& qry, int sm) {
    if(m_coll == nullptr || !qry)
        return {};
    assert(qry.m_qry);
    uint32_t s{0};
    const auto list = c_ejdb::qryexecute(m_coll, qry.m_qry.get(), &s, sm);
    if(list == nullptr)
        return {};
    assert(s == static_cast<decltype(s)>(c_ejdb::qresultnum(list)));
    std::vector<jbson::document> r;
    r.reserve(s);
    int ns{0};
    for(uint32_t i = 0; i < s; i++) {
        auto data = reinterpret_cast<const char*>(c_ejdb::qresultbsondata(list, i, &ns));
        if(data == nullptr) {
            s--;
            continue;
        }
        r.emplace_back(data, data + ns);
    }
    assert(r.size() == s);
    c_ejdb::qresultdispose(list);
    return std::move(r);
}

std::vector<jbson::document> collection::get_all() {
    auto db = m_db.lock();
    if(!db)
        return {};
    auto vec = "{}"_json_doc.data();
    auto q = query{m_db, c_ejdb::createquery(db.get(), vec.data())};
    return execute_query(q);
}

bool collection::sync(std::error_code& ec) noexcept {
    if(m_coll == nullptr) {
        ec = std::make_error_code(std::errc::bad_address);
        return false;
    }
    const auto r = c_ejdb::syncoll(m_coll);
    if(!r)
        ec = db::error(m_db);
    return r;
}

boost::string_ref collection::name() const noexcept {
    if(m_coll == nullptr)
        return {};
    return c_ejdb::collection_name(m_coll);
}

query::query(std::weak_ptr<EJDB> db, EJQ* qry) noexcept : m_db(db), m_qry(qry) {}

query::~query() noexcept {}

query& query::operator|=(const jbson::document& obj) & {
    assert(m_qry);
    auto db = m_db.lock();
    if(!db)
        return *this;

    auto q = c_ejdb::queryaddor(db.get(), m_qry.get(), obj.data().data());
    if(q != m_qry.get())
        m_qry.reset(q);

    return *this;
}

query&& query::operator|=(const jbson::document& obj) && {
    assert(m_qry);
    auto db = m_db.lock();
    if(!db)
        return std::move(*this);

    auto q = c_ejdb::queryaddor(db.get(), m_qry.get(), obj.data().data());
    if(q != m_qry.get())
        m_qry.reset(q);

    return std::move(*this);
}

query& query::set_hints(const jbson::document& obj) & {
    assert(m_qry);
    auto db = m_db.lock();
    if(!db)
        return *this;
    auto q = c_ejdb::queryhints(db.get(), m_qry.get(), obj.data().data());
    if(q != m_qry.get())
        m_qry.reset(q);

    return *this;
}

query&& query::set_hints(const jbson::document& obj) && {
    assert(m_qry);
    auto db = m_db.lock();
    if(!db)
        return std::move(*this);
    auto q = c_ejdb::queryhints(db.get(), m_qry.get(), obj.data().data());
    if(q != m_qry.get())
        m_qry.reset(q);

    return std::move(*this);
}

query::operator bool() const noexcept { return !m_db.expired() && m_qry != nullptr; }

class error_category : public std::error_category {
  public:
    constexpr error_category() noexcept = default;

    const char* name() const noexcept override { return "EJDB"; }

    std::string message(int ecode) const noexcept override { return c_ejdb::errmsg(ecode); }
};

std::error_code make_error_code(errc ecode) {
    static const ::ejdb::error_category ecat{};
    return std::error_code{static_cast<int>(ecode), ecat};
}

} // namespace ejdb
