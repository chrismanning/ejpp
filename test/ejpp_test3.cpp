/**************************************************************************
**  Copyright (C) 2013 Christian Manning
**
**  This program is free software: you can redistribute it and/or modify
**  it under the terms of the GNU General Public License as published by
**  the Free Software Foundation, either version 3 of the License, or
**  (at your option) any later version.
**
**  This program is distributed in the hope that it will be useful,
**  but WITHOUT ANY WARRANTY; without even the implied warranty of
**  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
**  GNU General Public License for more details.
**
**  You should have received a copy of the GNU General Public License
**  along with this program.  If not, see <http://www.gnu.org/licenses/>.
**************************************************************************/

#include <ejpp/ejdb.hpp>
#include <jbson/builder.hpp>

#include <gtest/gtest.h>

//static EJDB *jb;
//const int RS = 100000;
//const int QRS = 100;
//static bson* recs;

//static void eprint(EJDB *jb, int line, const char *func);

struct EjdbTest3 : ::testing::Test {
    void SetUp() override {
        std::error_code ec;
        auto r = jb.open("dbt3", ejdb::db_mode::read | ejdb::db_mode::write | ejdb::db_mode::create, ec);
        ASSERT_TRUE(r);
        ASSERT_FALSE(ec);
        ASSERT_TRUE(static_cast<bool>(jb));
    }
    void TearDown() override {
        jb.sync(ec);
        ec.clear();
        auto r = jb.close(ec);
        ASSERT_TRUE(r);
        ASSERT_FALSE(ec);
    }

    ejdb::db jb;
    std::error_code ec;
};

//int init_suite(void) {
//    assert(QRS < RS);
//    jb = ejdbnew();
//    if (!ejdbopen(jb, "dbt3", JBOWRITER | JBOCREAT | JBOTRUNC)) {
//        return 1;
//    }
//    srand(tcmstime());
//    recs = malloc(RS * sizeof (bson));
//    if (!recs) {
//        return 1;
//    }
//    for (int i = 0; i < RS; ++i) {
//        bson bs;
//        bson_init(&bs);
//        bson_append_long(&bs, "ts", tcmstime());
//        char str[128];
//        int len = 0;
//        do {
//            len = rand() % 128;
//        } while (len <= 0);
//        str[0] = 'A' + (i % 26);
//        for (int j = 1; j < len; ++j) {
//            str[j] = 'a' + rand() % 26;
//        }
//        str[len] = '\0';
//        bson_append_string(&bs, "rstring", str);
//        bson_finish(&bs);
//        recs[i] = bs;
//    }

//    return 0;
//}

//int clean_suite(void) {
//    ejdbclose(jb);
//    ejdbdel(jb);
//    for (int i = 0; i < RS; ++i) {
//        bson_destroy(&recs[i]);
//    }
//    free(recs);
//    return 0;
//}

//void testPerf1() {
//    CU_ASSERT_PTR_NOT_NULL_FATAL(jb);
//    CU_ASSERT_PTR_NOT_NULL_FATAL(recs);
//    EJCOLL *coll = ejdbcreatecoll(jb, "pcoll1", NULL);
//    CU_ASSERT_PTR_NOT_NULL_FATAL(coll);
//    unsigned long st = tcmstime();
//    for (int i = 0; i < RS; ++i) {
//        bson_oid_t oid;
//        ejdbsavebson(coll, recs + i, &oid);
//    }
//    ejdbsyncoll(coll);
//    fprintf(stderr, "\ntestPerf1(): SAVED %d BSON OBJECTS, TIME %lu ms\n", RS, tcmstime() - st);

//    st = tcmstime();
//    uint32_t acount = 0;
//    int i;
//    for (i = 0; i < QRS; ++i) {
//        int idx = rand() % QRS;
//        bson *bs = recs + idx;
//        assert(bs);
//        EJQ *q = ejdbcreatequery(jb, bs, NULL, 0, NULL);
//        assert(q);
//        uint32_t count;
//        ejdbqryexecute(coll, q, &count, JBQRYCOUNT, NULL);
//        assert(count);
//        if (count != 1) {
//            fprintf(stderr, "CNT=%u\n", count);
//        }
//        acount += count;
//        ejdbquerydel(q);
//    }
//    CU_ASSERT_TRUE(i <= acount);
//    fprintf(stderr, "testPerf1(): %u QUERIES, TIME: %lu ms, PER QUERY TIME: %lu ms\n",
//            i, tcmstime() - st, (unsigned long) ((tcmstime() - st) / QRS));

//    st = tcmstime();
//    CU_ASSERT_TRUE(ejdbsetindex(coll, "rstring", JBIDXSTR));
//    fprintf(stderr, "testPerf1(): SET INDEX 'rstring' TIME: %lu ms\n", tcmstime() - st);

//    st = tcmstime();
//    acount = 0;
//    for (i = 0; i < QRS; ++i) {
//        int idx = rand() % QRS;
//        bson *bs = recs + idx;
//        assert(bs);
//        EJQ *q = ejdbcreatequery(jb, bs, NULL, 0, NULL);
//        assert(q);
//        uint32_t count;
//        ejdbqryexecute(coll, q, &count, JBQRYCOUNT, NULL);
//        assert(count);
//        acount += count;
//        ejdbquerydel(q);
//    }
//    CU_ASSERT_TRUE(i <= acount);
//    fprintf(stderr, "testPerf1(): %u QUERIES WITH 'rstring' INDEX, TIME: %lu ms, PER QUERY TIME: %lu ms\n",
//            i, tcmstime() - st, (unsigned long) ((tcmstime() - st) / QRS));

//    bson bsq1;
//    bson_init_as_query(&bsq1);
//    bson_append_start_object(&bsq1, "$set");
//    bson_append_int(&bsq1, "intv", 1);
//    bson_append_finish_object(&bsq1);
//    bson_finish(&bsq1);

//    EJQ *q = ejdbcreatequery(jb, &bsq1, NULL, JBQRYCOUNT, NULL);
//    CU_ASSERT_PTR_NOT_NULL_FATAL(q);
//    uint32_t count;
//    st = tcmstime(); //$set op
//    ejdbqryexecute(coll, q, &count, JBQRYCOUNT, NULL);
//    if (ejdbecode(jb) != 0) {
//        eprint(jb, __LINE__, "$set test");
//        CU_ASSERT_TRUE(false);
//    }
//    CU_ASSERT_EQUAL(count, RS);
//    fprintf(stderr, "testPerf1(): {'$set' : {'intv' : 1}} FOR %u OBJECTS, TIME %lu ms\n", count, tcmstime() - st);
//    ejdbquerydel(q);
//    bson_destroy(&bsq1);

//    bson_init_as_query(&bsq1);
//    bson_append_start_object(&bsq1, "$inc");
//    bson_append_int(&bsq1, "intv", 1);
//    bson_append_finish_object(&bsq1);
//    bson_finish(&bsq1);

//    q = ejdbcreatequery(jb, &bsq1, NULL, JBQRYCOUNT, NULL);
//    CU_ASSERT_PTR_NOT_NULL_FATAL(q);
//    st = tcmstime(); //$inc op
//    ejdbqryexecute(coll, q, &count, JBQRYCOUNT, NULL);
//    if (ejdbecode(jb) != 0) {
//        eprint(jb, __LINE__, "$inc test");
//        CU_ASSERT_TRUE(false);
//    }
//    CU_ASSERT_EQUAL(count, RS);
//    fprintf(stderr, "testPerf1(): {'$inc' : {'intv' : 1}} FOR %u OBJECTS, TIME %lu ms\n", count, tcmstime() - st);
//    ejdbquerydel(q);
//    bson_destroy(&bsq1);

//    bson_init_as_query(&bsq1);
//    bson_append_int(&bsq1, "intv", 2);
//    bson_finish(&bsq1);
//    q = ejdbcreatequery(jb, &bsq1, NULL, JBQRYCOUNT, NULL);
//    ejdbqryexecute(coll, q, &count, JBQRYCOUNT, NULL);
//    CU_ASSERT_EQUAL(count, RS);
//    ejdbquerydel(q);
//    bson_destroy(&bsq1);

//    ejdbrmcoll(jb, coll->cname, true);
//}

////Race conditions

//typedef struct {
//    int id;
//    EJDB *jb;

//} TARGRACE;

//static void eprint(EJDB *jb, int line, const char *func) {
//    int ecode = ejdbecode(jb);
//    fprintf(stderr, "%d: %s: error: %d: %s\n",
//            line, func, ecode, ejdberrmsg(ecode));
//}

//static void *threadrace1(void *_tr) {
//    const int iterations = 500;
//    TARGRACE *tr = (TARGRACE*) _tr;
//    bool err = false;

//    bson bq;
//    bson_init_as_query(&bq);
//    bson_append_int(&bq, "tid", tr->id);
//    bson_finish(&bq);

//    bson_type bt;
//    bson_iterator it;
//    void *bsdata;
//    bool saved = false;
//    int lastcnt = 0;

//    EJCOLL *coll = ejdbcreatecoll(jb, "threadrace1", NULL);
//    CU_ASSERT_PTR_NOT_NULL_FATAL(coll);

//    EJQ *q = ejdbcreatequery(jb, &bq, NULL, 0, NULL);
//    TCXSTR *log = tcxstrnew();

//    for (int i = 0; !err && i < iterations; ++i) {
//        CU_ASSERT_PTR_NOT_NULL_FATAL(q);
//        tcxstrclear(log);

//        bson_oid_t oid2;
//        bson_oid_t *oid = NULL;
//        int cnt = 0;
//        uint32_t count;
//        TCLIST *res = NULL;

//        if (ejdbecode(jb) != 0) {
//            eprint(jb, __LINE__, "threadrace1");
//            err = true;
//            goto ffinish;
//        }
//        res = ejdbqryexecute(coll, q, &count, 0, log);
//        if (ejdbecode(jb) != 0) {
//            eprint(jb, __LINE__, "threadrace1.ejdbqryexecute");
//            err = true;
//            goto ffinish;
//        }
//        if (count != 1 && saved) {
//            fprintf(stderr, "%d:COUNT=%d it=%d\n", tr->id, count, i);
//            CU_ASSERT_TRUE(false);
//            goto ffinish;
//        }
//        if (count > 0) {
//            bsdata = TCLISTVALPTR(res, 0);
//            CU_ASSERT_PTR_NOT_NULL_FATAL(bsdata);
//            bt = bson_find_from_buffer(&it, bsdata, "cnt");
//            CU_ASSERT_EQUAL_FATAL(bt, BSON_INT);
//            cnt = bson_iterator_int(&it);
//            bt = bson_find_from_buffer(&it, bsdata, "_id");
//            CU_ASSERT_EQUAL_FATAL(bt, BSON_OID);
//            oid = bson_iterator_oid(&it);
//            CU_ASSERT_PTR_NOT_NULL_FATAL(oid);
//        }

//        bson sbs;
//        bson_init(&sbs);
//        if (oid) {
//            bson_append_oid(&sbs, "_id", oid);
//        }
//        bson_append_int(&sbs, "tid", tr->id);
//        bson_append_int(&sbs, "cnt", ++cnt);
//        bson_finish(&sbs);

//        if (!ejdbsavebson(coll, &sbs, &oid2)) {
//            eprint(jb, __LINE__, "threadrace1.ejdbsavebson");
//            err = true;
//        }
//        saved = true;
//        bson_destroy(&sbs);
//        lastcnt = cnt;
//ffinish:
//        if (res) tclistdel(res);
//    }
//    if (q) ejdbquerydel(q);
//    if (log) tcxstrdel(log);
//    bson_destroy(&bq);
//    CU_ASSERT_EQUAL(lastcnt, iterations);
//    //fprintf(stderr, "\nThread %d finished", tr->id);
//    return err ? "error" : NULL;
//}

//void testRace1() {
//    CU_ASSERT_PTR_NOT_NULL_FATAL(jb);
//    const int tnum = 50;
//    bool err = false;
//    TARGRACE targs[tnum];
//    pthread_t threads[tnum];

//    EJCOLL *coll = ejdbcreatecoll(jb, "threadrace1", NULL);
//    CU_ASSERT_PTR_NOT_NULL_FATAL(coll);
//    if (!ejdbsetindex(coll, "tid", JBIDXNUM)) { //INT INDEX
//        eprint(jb, __LINE__, "testRace1");
//        err = true;
//    }
//    if (err) {
//        goto finish;
//    }

//    for (int i = 0; i < tnum; i++) {
//        targs[i].jb = jb;
//        targs[i].id = i;
//        if (pthread_create(threads + i, NULL, threadrace1, targs + i) != 0) {
//            eprint(jb, __LINE__, "pthread_create");
//            targs[i].id = -1;
//            err = true;
//        }
//    }

//    for (int i = 0; i < tnum; i++) {
//        if (targs[i].id == -1) continue;
//        void *rv;
//        if (pthread_join(threads[i], &rv) != 0) {
//            eprint(jb, __LINE__, "pthread_join");
//            err = true;
//        } else if (rv) {
//            err = true;
//        }
//    }

//finish:
//    CU_ASSERT_FALSE(err);
//}

//void testRace2() {
//    CU_ASSERT_PTR_NOT_NULL_FATAL(jb);
//    const int tnum = 50;
//    bool err = false;
//    TARGRACE targs[tnum];
//    pthread_t threads[tnum];

//    ejdbrmcoll(jb, "threadrace1", true);
//    EJCOLL *coll = ejdbcreatecoll(jb, "threadrace1", NULL);
//    CU_ASSERT_PTR_NOT_NULL_FATAL(coll);
//    if (!ejdbsetindex(coll, "tid", JBIDXDROPALL)) { //NO INDEX
//        eprint(jb, __LINE__, "testRace2");
//        err = true;
//    }
//    if (err) {
//        goto finish;
//    }

//    for (int i = 0; i < tnum; i++) {
//        targs[i].jb = jb;
//        targs[i].id = i;
//        if (pthread_create(threads + i, NULL, threadrace1, targs + i) != 0) {
//            eprint(jb, __LINE__, "pthread_create");
//            targs[i].id = -1;
//            err = true;
//        }
//    }
//    for (int i = 0; i < tnum; i++) {
//        if (targs[i].id == -1) continue;
//        void *rv;
//        if (pthread_join(threads[i], &rv) != 0) {
//            eprint(jb, __LINE__, "pthread_join");
//            err = true;
//        } else if (rv) {
//            err = true;
//        }
//    }

//finish:
//    CU_ASSERT_FALSE(err);
//}

TEST_F(EjdbTest3, testTransactions1) {
    ASSERT_TRUE(jb.remove_collection("trans1", true, ec));
    ASSERT_FALSE(ec);
    auto coll = jb.create_collection("trans1", ec);
    ASSERT_TRUE(static_cast<bool>(coll));
    ASSERT_FALSE(ec);

    auto bs = jbson::document(jbson::builder("foo", "bar"));
    std::array<char, 12> oid;
    {
        ejdb::unique_transaction t{coll.transaction()};

        ASSERT_TRUE(t.owns_transaction());

        auto o_oid = coll.save_document(bs, ec);
        EXPECT_FALSE(ec);
        ASSERT_TRUE(o_oid);

        ASSERT_FALSE(jb.error());
        ASSERT_NO_THROW(t.commit());
        ASSERT_FALSE(t.owns_transaction());

        oid = *o_oid;
    }

    ASSERT_TRUE(static_cast<bool>(coll));
    auto o_doc = coll.load_document(oid, ec);
    EXPECT_FALSE(ec);
    ASSERT_TRUE(o_doc);

    {
        ejdb::unique_transaction t{coll.transaction(), ejdb::defer_transaction};

        ASSERT_FALSE(t.owns_transaction());
        ASSERT_NO_THROW(t.start());
        ASSERT_TRUE(t.owns_transaction());

        auto o_oid = coll.save_document(bs, ec);
        EXPECT_TRUE(static_cast<bool>(coll));
        EXPECT_FALSE(ec);
        ASSERT_TRUE(o_oid);
        oid = *o_oid;

        ASSERT_NO_THROW(t.abort());
        ASSERT_FALSE(t.owns_transaction());
    }

    o_doc = coll.load_document(oid, ec);
    EXPECT_FALSE(ec);
    EXPECT_FALSE(o_doc);
}

TEST_F(EjdbTest3, testTransactions2) {
    ASSERT_TRUE(jb.remove_collection("trans1", true, ec));
    ASSERT_FALSE(ec);
    auto coll = jb.create_collection("trans1", ec);
    ASSERT_TRUE(static_cast<bool>(coll));
    ASSERT_FALSE(ec);

    auto bs = jbson::document(jbson::builder("foo", "bar"));
    std::array<char, 12> oid;
    {
        ejdb::transaction_guard guard{coll.transaction()};

        auto o_oid = coll.save_document(bs, ec);
        EXPECT_FALSE(ec);
        ASSERT_TRUE(o_oid);

        oid = *o_oid;
    }

    ASSERT_TRUE(static_cast<bool>(coll));
    auto o_doc = coll.load_document(oid, ec);
    EXPECT_FALSE(ec);
    ASSERT_TRUE(o_doc);

    {
        ASSERT_FALSE(jb.error());
        ejdb::unique_transaction u_trans{};
        ASSERT_FALSE(u_trans.owns_transaction());
        u_trans = ejdb::unique_transaction{coll.transaction()};
        ASSERT_TRUE(u_trans.owns_transaction());

        auto o_oid = coll.save_document(bs, ec);
        EXPECT_TRUE(static_cast<bool>(coll));
        EXPECT_FALSE(ec);
        ASSERT_TRUE(o_oid);
        oid = *o_oid;

        ASSERT_NO_THROW(u_trans.abort());
    }

    o_doc = coll.load_document(oid, ec);
    EXPECT_FALSE(ec);
    EXPECT_FALSE(o_doc);
}

TEST_F(EjdbTest3, testTransactions3) {
    ASSERT_TRUE(jb.remove_collection("trans1", true, ec));
    ASSERT_FALSE(ec);
    auto coll = jb.create_collection("trans1", ec);
    ASSERT_TRUE(static_cast<bool>(coll));
    ASSERT_FALSE(ec);

    auto bs = jbson::document(jbson::builder("foo", "bar"));
    std::array<char, 12> oid;
    {
        ejdb::transaction_guard guard{coll.transaction()};

        auto o_oid = coll.save_document(bs, ec);
        EXPECT_FALSE(ec);
        ASSERT_TRUE(o_oid);

        oid = *o_oid;
    }

    ASSERT_TRUE(static_cast<bool>(coll));
    auto o_doc = coll.load_document(oid, ec);
    EXPECT_FALSE(ec);
    ASSERT_TRUE(o_doc);

    {
        ejdb::unique_transaction tran{coll.transaction()};
        ASSERT_TRUE(tran.owns_transaction());

        tran.release();
        ASSERT_FALSE(tran.owns_transaction());

        tran = ejdb::unique_transaction{coll.transaction(), ejdb::adopt_transaction};
        ASSERT_TRUE(tran.owns_transaction());

        auto o_oid = coll.save_document(bs, ec);
        EXPECT_FALSE(ec);
        ASSERT_TRUE(o_oid);

        oid = *o_oid;
    }

    ASSERT_TRUE(static_cast<bool>(coll));
    o_doc = coll.load_document(oid, ec);
    EXPECT_FALSE(ec);
    ASSERT_TRUE(o_doc);

    try {
        ASSERT_FALSE(jb.error());
        ejdb::transaction_guard guard_trans{coll.transaction()};

        auto o_oid = coll.save_document(bs, ec);
        EXPECT_TRUE(static_cast<bool>(coll));
        EXPECT_FALSE(ec);
        ASSERT_TRUE(o_oid);
        oid = *o_oid;

        throw 0;
    }
    catch(...) {}

    o_doc = coll.load_document(oid, ec);
    EXPECT_FALSE(ec);
    EXPECT_FALSE(o_doc);

    try {
        ASSERT_FALSE(jb.error());
        ejdb::unique_transaction u_trans{coll.transaction()};

        auto o_oid = coll.save_document(bs, ec);
        EXPECT_TRUE(static_cast<bool>(coll));
        EXPECT_FALSE(ec);
        ASSERT_TRUE(o_oid);
        oid = *o_oid;

        throw 0;
    }
    catch(...) {}

    o_doc = coll.load_document(oid, ec);
    EXPECT_FALSE(ec);
    EXPECT_FALSE(o_doc);
}
