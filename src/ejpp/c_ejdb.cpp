/**************************************************************************
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
**************************************************************************/

#include <system_error>

#include <tcejdb/ejdb.h>
#include <tcejdb/ejdb_private.h>

#include <ejpp/c_ejdb.hpp>

namespace c_ejdb {

const char* version() { return ejdbversion(); }

bool isvalidoidstr(const char* oid) { return ejdbisvalidoidstr(oid); }

const char* errmsg(int ecode) { return ejdberrmsg(ecode); }

int ecode(EJDB* jb) { return ejdbecode(jb); }

EJDB* newdb(void) { return ejdbnew(); }

void del(EJDB* jb) { ejdbdel(jb); }

bool closedb(EJDB* jb) { return ejdbclose(jb); }

bool open(EJDB* jb, const char* path, int mode) { return ejdbopen(jb, path, mode); }

bool isopen(EJDB* jb) { return ejdbisopen(jb); }

EJCOLL* getcoll(EJDB* jb, const char* colname) { return ejdbgetcoll(jb, colname); }

std::vector<EJCOLL*> getcolls(EJDB* jb) {
    return {jb->cdbs, jb->cdbs + jb->cdbsnum};
}

EJCOLL* createcoll(EJDB* jb, const char* colname, void* opts) {
    return ejdbcreatecoll(jb, colname, reinterpret_cast<EJCOLLOPTS*>(opts));
}

bool rmcoll(EJDB* jb, const char* colname, bool unlinkfile) { return ejdbrmcoll(jb, colname, unlinkfile); }

bool savebson(EJCOLL* jcoll, const std::vector<char>& bsdata, char oid[12], bool merge, int* err) {
    if(bsdata.size() < 5 ||
       le32toh(*reinterpret_cast<const int32_t*>(bsdata.data())) != static_cast<int32_t>(bsdata.size())) {
        assert(err != nullptr);
        *err = JBEINVALIDBSON;
        return false;
    }
    return ejdbsavebson3(jcoll, bsdata.data(), reinterpret_cast<bson_oid_t*>(oid), merge);
}

bool rmbson(EJCOLL* coll, char oid[12]) { return ejdbrmbson(coll, reinterpret_cast<bson_oid_t*>(oid)); }

std::vector<char> loadbson(EJCOLL* coll, const char oid[12]) {
    auto bs = ejdbloadbson(coll, reinterpret_cast<const bson_oid_t*>(oid));
    if(bs == nullptr)
        return {};
    assert(bs != nullptr);
    assert(bs->data != nullptr);

    size_t s = bs->dataSize;
    if(bs->dataSize <= 0)
        s = le32toh(*reinterpret_cast<int32_t*>(bs->data));
    assert(s >= 4);
    std::vector<char> ret{bs->data, bs->data + s};
    bson_del(bs);
    return std::move(ret);
}

EJQ* createquery(EJDB* jb, const void* qbsdata) { return ejdbcreatequery2(jb, qbsdata); }

EJQ* queryaddor(EJDB* jb, EJQ* q, const void* orbsdata) { return ejdbqueryaddor(jb, q, orbsdata); }

EJQ* queryhints(EJDB* jb, EJQ* q, const void* hintsbsdata) { return ejdbqueryhints(jb, q, hintsbsdata); }

void querydel(EJQ* q) { ejdbquerydel(q); }

bool setindex(EJCOLL* coll, const char* ipath, int flags) { return ejdbsetindex(coll, ipath, flags); }

EJQRESULT qryexecute(EJCOLL* jcoll, const EJQ* q, uint32_t* count, int qflags) {
    return ejdbqryexecute(jcoll, q, count, qflags, nullptr);
}

int qresultnum(EJQRESULT qr) { return ejdbqresultnum(qr); }

const void* qresultbsondata(EJQRESULT qr, int pos, int* size) { return ejdbqresultbsondata(qr, pos, size); }

void qresultdispose(EJQRESULT qr) { return ejdbqresultdispose(qr); }

bool syncoll(EJCOLL* jcoll) { return ejdbsyncoll(jcoll); }

bool syncdb(EJDB* jb) { return ejdbsyncdb(jb); }

bool tranbegin(EJCOLL* coll) { return ejdbtranbegin(coll); }

bool trancommit(EJCOLL* coll) { return ejdbtrancommit(coll); }

bool tranabort(EJCOLL* coll) { return ejdbtranabort(coll); }

bool transtatus(EJCOLL* jcoll, bool* txactive) { return ejdbtranstatus(jcoll, txactive); }

std::vector<char> metadb(EJDB* jb) {
    auto bs = ejdbmeta(jb);
    if(bs == nullptr)
        return {};
    std::vector<char> ret{bs->data, bs->data + bs->dataSize};
    bson_del(bs);
    return std::move(ret);
}

std::string collection_name(EJCOLL* coll) {
    assert(coll->cnamesz >= 0);
    return {coll->cname, static_cast<size_t>(coll->cnamesz)};
}

} // namespace c_ejdb
