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

#ifndef EJDB_C_EJDB_HPP
#define EJDB_C_EJDB_HPP

#include <tcejdb/tcutil.h>
#include <boost/utility/string_ref_fwd.hpp>
#include <vector>

extern "C" {

struct EJDB;
typedef struct EJDB EJDB;

struct EJCOLL;
typedef struct EJCOLL EJCOLL;

struct EJQ;
typedef struct EJQ EJQ;

typedef TCLIST* EJQRESULT;

} // extern "C"

/*!
 * \brief Contains wrappers for EJDB's C functions. Unpollutes the global namespace.
 * \namespace c_ejdb
 */
namespace c_ejdb {

//! Returns ejdbversion()
const char* version();

//! Returns ejdbisvalidoidstr(oid)
bool isvalidoidstr(const char* oid);

//! Returns ejdberrmsg(ecode)
const char* errmsg(int ecode);

//! Returns ejdbecode(jb)
int ecode(EJDB* jb);

//! Returns ejdbnew()
EJDB* newdb(void);

//! Calls ejdbdel(jb)
void del(EJDB* jb);

//! Returns ejdbclose(jb)
bool closedb(EJDB* jb);

//! Returns ejdbopen(jb, path, mode)
bool open(EJDB* jb, const char* path, int mode);

//! Returns ejdbisopen(jb)
bool isopen(EJDB* jb);

//! Returns ejdbgetcoll(jb, colname)
EJCOLL* getcoll(EJDB* jb, const char* colname);

//! Returns a transformation of ejdbgetcolls(jb)
std::vector<EJCOLL*> getcolls(EJDB* jb);

//! Returns ejdbcreatecoll(jb, colname, opts)
EJCOLL* createcoll(EJDB* jb, const char* colname, void* opts);

//! Returns ejdbrmcoll(jb, collname, unlinkfile)
bool rmcoll(EJDB* jb, const char* colname, bool unlinkfile);

//! Returns ejdbsavebson3(coll, bsdata, oid, merge)
bool savebson(EJCOLL* jcoll, const std::vector<char>& bsdata, char oid[12], bool merge, int* err);

//! Returns ejdbrmbson(coll, oid)
bool rmbson(EJCOLL* coll, char oid[12]);

//! Returns transformation of ejdbloadbson(coll, oid)
std::vector<char> loadbson(EJCOLL* coll, const char oid[12]);

//! Returns ejdbcreatequery2(jb, qbsdata)
EJQ* createquery(EJDB* jb, const void* qbsdata);

//! Returns ejdbqueryaddor(jb, q, orbsdata)
EJQ* queryaddor(EJDB* jb, EJQ* q, const void* orbsdata);

//! Returns ejdbqueryhints(jb, q, hintsbsdata)
EJQ* queryhints(EJDB* jb, EJQ* q, const void* hintsbsdata);

//! Calls ejdbquerydel(q)
void querydel(EJQ* q);

//! Returns ejdbsetindex(coll, ipath, flags
bool setindex(EJCOLL* coll, const char* ipath, int flags);

//! Returns ejdbqryexecute(jcoll, q, count, qflags, nullptr)
EJQRESULT qryexecute(EJCOLL* jcoll, const EJQ* q, uint32_t* count, int qflags);

//! Returns ejdbqresultnum(qr)
int qresultnum(EJQRESULT qr);

//! Returns ejdbqresultbsondata(qr, pos, size)
const void* qresultbsondata(EJQRESULT qr, int pos, int* size);

//! Calls ejdbqresultdispose(qr)
void qresultdispose(EJQRESULT qr);

// uint32_t update(EJCOLL *jcoll, bson *qobj, bson *orqobjs,
//                                int orqobjsnum, bson *hints, TCXSTR *log);

//! Returns ejdbsyncoll(jcoll)
bool syncoll(EJCOLL* jcoll);

//! Returns ejdbsyncdb(jb)
bool syncdb(EJDB* jb);

//! Returns ejdbtranbegin(coll)
bool tranbegin(EJCOLL* coll);

//! Returns ejdbtrancommit(coll)
bool trancommit(EJCOLL* coll);

//! Returns ejdbtranabort(coll)
bool tranabort(EJCOLL* coll);

//! Returns ejdbtranstatus(jcoll, txactive)
bool transtatus(EJCOLL* jcoll, bool* txactive);

//! Returns transformation of ejdbmeta(jb)
std::vector<char> metadb(EJDB* jb);

//! Returns name of a collection.
boost::string_ref collection_name(EJCOLL* coll);

} // namespace c_ejdb

#endif // EJDB_C_EJDB_HPP
