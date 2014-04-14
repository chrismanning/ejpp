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

namespace c_ejdb {

const char* version();

bool isvalidoidstr(const char* oid);

const char* errmsg(int ecode);

int ecode(EJDB* jb);

EJDB* newdb(void);

void del(EJDB* jb);

bool closedb(EJDB* jb);

bool open(EJDB* jb, const char* path, int mode);

bool isopen(EJDB* jb);

EJCOLL* getcoll(EJDB* jb, const char* colname);

std::vector<EJCOLL*> getcolls(EJDB* jb);

EJCOLL* createcoll(EJDB* jb, const char* colname, void* opts);

bool rmcoll(EJDB* jb, const char* colname, bool unlinkfile);

bool savebson(EJCOLL* coll, const void* bsdata, char oid[12]);

bool savebson2(EJCOLL* jcoll, const void* bsdata, char oid[12], bool merge);

bool rmbson(EJCOLL* coll, char oid[12]);

std::vector<char> loadbson(EJCOLL* coll, const char oid[12]);

EJQ* createquery(EJDB* jb, const void* qbsdata);

EJQ* queryaddor(EJDB* jb, EJQ* q, const void* orbsdata);

EJQ* queryhints(EJDB* jb, EJQ* q, const void* hintsbsdata);

void querydel(EJQ* q);

bool setindex(EJCOLL* coll, const char* ipath, int flags);

EJQRESULT qryexecute(EJCOLL* jcoll, const EJQ* q, uint32_t* count, int qflags);

int qresultnum(EJQRESULT qr);

const void* qresultbsondata(EJQRESULT qr, int pos, int* size);

void qresultdispose(EJQRESULT qr);

// uint32_t update(EJCOLL *jcoll, bson *qobj, bson *orqobjs,
//                                int orqobjsnum, bson *hints, TCXSTR *log);

bool syncoll(EJCOLL* jcoll);

bool syncdb(EJDB* jb);

bool tranbegin(EJCOLL* coll);

bool trancommit(EJCOLL* coll);

bool tranabort(EJCOLL* coll);

bool transtatus(EJCOLL* jcoll, bool* txactive);

std::vector<char> metadb(EJDB*);

boost::string_ref collection_name(EJCOLL* coll);

} // namespace c_ejdb

#endif // EJDB_C_EJDB_HPP
