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

ejdb::ejdb() : m_db(c_ejdb::newdb(), ejdb_deleter()) {}

ejdb::operator bool() const noexcept { return static_cast<bool>(m_db); }

std::error_code ejdb::error() const noexcept {
    assert(m_db);
    return make_error_code(c_ejdb::ecode(m_db.get()));
}

bool ejdb::open(const std::string& path, int mode, std::error_code& ec) noexcept {
    assert(m_db);
    const auto r = c_ejdb::open(m_db.get(), path.c_str(), mode);
    if(!r)
        ec = error();
    return r;
}

bool ejdb::is_open() const noexcept {
    assert(m_db);
    return c_ejdb::isopen(m_db.get());
}

bool ejdb::close(std::error_code& ec) noexcept {
    assert(m_db);
    const auto r = c_ejdb::closedb(m_db.get());
    if(!r)
        ec = error();
    return r;
}

collection ejdb::get_collection(const std::string& name, std::error_code& ec) const noexcept {
    assert(m_db);
    const auto r = c_ejdb::getcoll(m_db.get(), name.c_str());
    if(!r)
        ec = error();
    return {m_db, r};
}

collection ejdb::create_collection(const std::string& name, std::error_code& ec) noexcept {
    assert(m_db);
    const auto r = c_ejdb::createcoll(m_db.get(), name.c_str(), nullptr);
    if(!r)
        ec = error();
    return {m_db, r};
}

bool ejdb::remove_collection(const std::string& name, bool unlink_file, std::error_code& ec) noexcept {
    assert(m_db);
    const auto r = c_ejdb::rmcoll(m_db.get(), name.c_str(), unlink_file);
    if(!r)
        ec = error();
    return r;
}

const std::deque<collection> ejdb::get_collections() const noexcept {
    assert(m_db);
    auto colls = c_ejdb::getcolls(m_db.get());
    auto range = boost::adaptors::transform(colls, [this](EJCOLL* c) { return collection{m_db, c}; });
    return {range.begin(), range.end()};
}

query ejdb::create_query(const jbson::document& doc, std::error_code& ec) noexcept {
    assert(m_db);
    try {
        const auto r = c_ejdb::createquery(m_db.get(), doc.data().data());
        if(!r)
            ec = error();
        return {m_db, r};
    }
    catch(jbson::jbson_error&) {
        ec = make_error_code(9001);
        return query{};
    }
}

bool ejdb::sync(std::error_code& ec) noexcept {
    assert(m_db);
    const auto r = c_ejdb::syncdb(m_db.get());
    if(!r)
        ec = error();
    return r;
}

boost::optional<jbson::document> ejdb::metadata() noexcept {
    assert(m_db);
    auto r = c_ejdb::metadb(m_db.get());
    return r.empty() ? boost::optional<jbson::document>{boost::none} : jbson::document{std::move(r)};
}

collection::collection(std::weak_ptr<EJDB> db, EJCOLL* coll) noexcept : m_db(db), m_coll(coll) {}

collection::operator bool() const noexcept { return !m_db.expired() && m_coll != nullptr; }

boost::optional<std::array<char, 12>> collection::save_document(const jbson::document& data,
                                                                std::error_code& ec) noexcept {
    return save_document(data, false, ec);
}

boost::optional<std::array<char, 12>> collection::save_document(const jbson::document& doc, bool merge,
                                                                std::error_code& ec) noexcept {
    assert(m_coll);
    auto db = m_db.lock();
    if(!db && (ec = std::make_error_code(std::errc::owner_dead)))
        return {};
    std::array<char, 12> oid;
    const auto r = c_ejdb::savebson2(m_coll, doc.data().data(), oid.data(), merge);
    ec = make_error_code(!r ? c_ejdb::ecode(db.get()) : 0);
    return r ? oid : boost::optional<std::array<char, 12>>{boost::none};
}

boost::optional<jbson::document> collection::load_document(std::array<char, 12> oid, std::error_code& ec) const
    noexcept {
    assert(m_coll);
    auto r = c_ejdb::loadbson(m_coll, oid.data());
    auto db = m_db.lock();
    ec = make_error_code(r.empty() && db ? c_ejdb::ecode(db.get()) : 0);
    if(!ec)
        return boost::none;
    return jbson::document(std::move(r));
}

bool collection::remove_document(std::array<char, 12> oid, std::error_code& ec) noexcept {
    assert(m_coll);
    const auto r = c_ejdb::rmbson(m_coll, oid.data());
    auto db = m_db.lock();
    if(!r && db)
        ec = make_error_code(c_ejdb::ecode(db.get()));
    return r;
}

std::vector<jbson::document> collection::execute_query(const query& qry, int sm) noexcept {
    assert(m_coll);
    if(!qry)
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

std::vector<jbson::document> collection::get_all() noexcept {
    auto db = m_db.lock();
    auto vec = "{}"_json_doc.data();
    auto q = query{m_db, c_ejdb::createquery(db.get(), vec.data())};
    return execute_query(q);
}

bool collection::sync(std::error_code& ec) noexcept {
    assert(m_coll);
    const auto r = c_ejdb::syncoll(m_coll);
    auto db = m_db.lock();
    if(!r && db)
        ec = make_error_code(c_ejdb::ecode(db.get()));
    return r;
}

boost::string_ref collection::name() const noexcept {
    assert(m_coll);
    return c_ejdb::collection_name(m_coll);
}

query::query(std::weak_ptr<EJDB> db, EJQ* qry) noexcept : m_db(db), m_qry(qry) {}

query::~query() noexcept {}

query& query::operator|=(const jbson::document& obj) noexcept {
    assert(m_qry);
    auto db = m_db.lock();
    if(!db)
        return *this;

    auto q = c_ejdb::queryaddor(db.get(), m_qry.get(), obj.data().data());
    if(q != m_qry.get())
        m_qry.reset(q);

    return *this;
}

query& query::set_hints(const jbson::document& obj) noexcept {
    assert(m_qry);
    auto db = m_db.lock();
    if(!db)
        return *this;
    auto q = c_ejdb::queryhints(db.get(), m_qry.get(), obj.data().data());
    if(q != m_qry.get())
        m_qry.reset(q);

    return *this;
}

query::operator bool() const noexcept { return !m_db.expired() && m_qry != nullptr; }

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
