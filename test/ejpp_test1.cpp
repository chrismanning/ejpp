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

#include <string>
using namespace std::literals;
#include <list>
#include <fstream>

#include <ejpp/ejdb.hpp>
#include <jbson/document.hpp>
#include <jbson/builder.hpp>
#include <jbson/json_reader.hpp>
using namespace jbson;

#include <gtest/gtest.h>

struct EjdbTest : ::testing::Test {
    void SetUp() override {
        ASSERT_TRUE(static_cast<bool>(jb));
        std::error_code ec;
        auto r = jb.open("dbt1", ejdb::db_mode::write|ejdb::db_mode::create|ejdb::db_mode::truncate, ec);
        ASSERT_TRUE(r);
        ASSERT_FALSE(ec);
    }
    void TearDown() override {
        std::error_code ec;
        auto r = jb.remove_collection("contacts", true, ec);
        ASSERT_TRUE(r);
        ASSERT_FALSE(ec);
        r = jb.close(ec);
        ASSERT_TRUE(r);
        ASSERT_FALSE(ec);
    }

    ejdb::ejdb jb;
};

template <typename Rng>
bool operator==(const std::array<char, 12>& a, const Rng& b) {
    return boost::equal(a, b);
}

template <typename Rng>
bool operator==(const Rng& b, const std::array<char, 12>& a) {
    return boost::equal(a, b);
}

TEST_F(EjdbTest, TestSaveLoad) {
    ASSERT_TRUE(static_cast<bool>(jb));

    std::error_code ec;
    std::array<char, 12> oid;

    auto ccoll = jb.create_collection("contacts", ec);
    ASSERT_TRUE(static_cast<bool>(ccoll));
    ASSERT_FALSE(static_cast<bool>(ec));

    //Save record
    auto a1 = R"({
        "name": "Петров Петр",
        "phone": "333-222-333",
        "age": 33,
        "longage": 2.8147497671e+14,
        "doubleage": 0.333333
    })"_json_doc;

    auto o_oid = ccoll.save_document(a1, ec);
    ASSERT_FALSE(static_cast<bool>(ec));
    ASSERT_TRUE(static_cast<bool>(o_oid));
    oid = *o_oid;
    EXPECT_NE(boost::as_literal("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"), oid);

    auto lbson = ccoll.load_document(oid, ec);
    ASSERT_FALSE(static_cast<bool>(ec));
    ASSERT_TRUE(static_cast<bool>(lbson));

//    ASSERT_TRUE(boost::equal(a1, *lbson));
}

//TEST_F(EjdbTest, TestBuildQuery1) {
//    CU_ASSERT_PTR_NOT_NULL_FATAL(jb);
//    /*
//     Query = {
//        "name" : Петров Петр,
//        "age"  : 33,
//        "family" : {
//            "wife" : {
//                "name"  : "Jeniffer",
//                "age"   : {"$gt" : 25},
//                "phone" : "444-111"
//            },
//            "children" : [
//                {
//                    "name" : "Dasha",
//                    "age" : {"$in" : [1, 4, 10]}
//                }
//            ]
//         }
//     */
//    bson q1;
//    bson_init_as_query(&q1);
//    bson_append_string(&q1, "name", "Петров Петр");
//    bson_append_int(&q1, "age", 33);

//    bson q1family_wife;
//    bson_init_as_query(&q1family_wife);
//    bson_append_string(&q1family_wife, "name", "Jeniffer");
//    bson_append_start_object(&q1family_wife, "age");
//    bson_append_int(&q1family_wife, "$gt", 25);
//    bson_append_finish_object(&q1family_wife);

//    bson_append_string(&q1family_wife, "phone", "444-111");
//    bson_finish(&q1family_wife);

//    bson q1family_child;
//    bson_init_as_query(&q1family_child);
//    bson_append_string(&q1family_child, "name", "Dasha");

//    //"age" : {"$in" : [1, 4, 10]}
//    bson q1family_child_age_IN;
//    bson_init_as_query(&q1family_child_age_IN);
//    bson_append_start_array(&q1family_child_age_IN, "$in");
//    bson_append_int(&q1family_child_age_IN, "0", 1);
//    bson_append_int(&q1family_child_age_IN, "1", 4);
//    bson_append_int(&q1family_child_age_IN, "2", 10);
//    bson_append_finish_array(&q1family_child_age_IN);
//    bson_finish(&q1family_child_age_IN);
//    bson_append_bson(&q1family_child, "age", &q1family_child_age_IN);
//    bson_finish(&q1family_child);

//    bson q1family;
//    bson_init_as_query(&q1family);
//    bson_append_bson(&q1family, "wife", &q1family_wife);
//    bson_append_start_array(&q1family, "children");
//    bson_append_bson(&q1family, "0", &q1family_child);
//    bson_append_finish_array(&q1family);
//    bson_finish(&q1family);

//    bson_append_bson(&q1, "family", &q1family);
//    bson_finish(&q1);

//    CU_ASSERT_FALSE_FATAL(q1.err);
//    CU_ASSERT_FALSE_FATAL(q1family.err);
//    CU_ASSERT_FALSE_FATAL(q1family_wife.err);
//    CU_ASSERT_FALSE_FATAL(q1family_child.err);
//    CU_ASSERT_FALSE_FATAL(q1family_child_age_IN.err);

//    EJQ *ejq = ejdbcreatequery(jb, &q1, NULL, 0, NULL);
//    CU_ASSERT_PTR_NOT_NULL_FATAL(ejq);

//    bson_destroy(&q1);
//    bson_destroy(&q1family);
//    bson_destroy(&q1family_wife);
//    bson_destroy(&q1family_child);
//    bson_destroy(&q1family_child_age_IN);

//    CU_ASSERT_PTR_NOT_NULL_FATAL(ejq->qflist);
//    TCLIST *qmap = ejq->qflist;
//    CU_ASSERT_EQUAL(qmap->num, 7);

//    for (int i = 0; i < TCLISTNUM(qmap); ++i) {

//        const EJQF *qf = TCLISTVALPTR(qmap, i);
//        CU_ASSERT_PTR_NOT_NULL_FATAL(qf);
//        const char* key = qf->fpath;

//        switch (i) {
//            case 0:
//            {
//                CU_ASSERT_STRING_EQUAL(key, "name");
//                CU_ASSERT_PTR_NOT_NULL(qf);
//                CU_ASSERT_STRING_EQUAL(qf->expr, "Петров Петр");
//                CU_ASSERT_EQUAL(qf->tcop, TDBQCSTREQ);
//                break;
//            }
//            case 1:
//            {
//                CU_ASSERT_STRING_EQUAL(key, "age");
//                CU_ASSERT_PTR_NOT_NULL(qf);
//                CU_ASSERT_STRING_EQUAL(qf->expr, "33");
//                CU_ASSERT_EQUAL(qf->tcop, TDBQCNUMEQ);
//                break;
//            }
//            case 2:
//            {
//                CU_ASSERT_STRING_EQUAL(key, "family.wife.name");
//                CU_ASSERT_PTR_NOT_NULL(qf);
//                CU_ASSERT_STRING_EQUAL(qf->expr, "Jeniffer");
//                CU_ASSERT_EQUAL(qf->tcop, TDBQCSTREQ);
//                break;
//            }
//            case 3:
//            {
//                CU_ASSERT_STRING_EQUAL(key, "family.wife.age");
//                CU_ASSERT_PTR_NOT_NULL(qf);
//                CU_ASSERT_STRING_EQUAL(qf->expr, "25");
//                CU_ASSERT_EQUAL(qf->tcop, TDBQCNUMGT);
//                break;
//            }
//            case 4:
//            {
//                CU_ASSERT_STRING_EQUAL(key, "family.wife.phone");
//                CU_ASSERT_PTR_NOT_NULL(qf);
//                CU_ASSERT_STRING_EQUAL(qf->expr, "444-111");
//                CU_ASSERT_EQUAL(qf->tcop, TDBQCSTREQ);
//                break;
//            }
//            case 5:
//            {
//                CU_ASSERT_STRING_EQUAL(key, "family.children.0.name");
//                CU_ASSERT_PTR_NOT_NULL(qf);
//                CU_ASSERT_STRING_EQUAL(qf->expr, "Dasha");
//                CU_ASSERT_EQUAL(qf->tcop, TDBQCSTREQ);
//                break;
//            }
//            case 6:
//            {
//                CU_ASSERT_STRING_EQUAL(key, "family.children.0.age");
//                CU_ASSERT_PTR_NOT_NULL(qf);
//                CU_ASSERT_EQUAL(qf->ftype, BSON_ARRAY);
//                TCLIST *al = tclistload(qf->expr, qf->exprsz);
//                char* als = tcstrjoin(al, ',');
//                CU_ASSERT_STRING_EQUAL(als, "1,4,10");
//                TCFREE(als);
//                tclistdel(al);
//                CU_ASSERT_EQUAL(qf->tcop, TDBQCNUMOREQ);
//                break;
//            }
//        }
//    }

//    ejdbquerydel(ejq);
//}

//TEST_F(EjdbTest, TestDBOptions) {
//    EJCOLLOPTS opts;
//    opts.cachedrecords = 10000;
//    opts.compressed = true;
//    opts.large = true;
//    opts.records = 110000;
//    EJCOLL *coll = ejdbcreatecoll(jb, "optscoll", &opts);
//    CU_ASSERT_PTR_NOT_NULL_FATAL(coll);
//    TCHDB *hdb = coll->tdb->hdb;
//    CU_ASSERT_TRUE(hdb->bnum >= (opts.records * 2 + 1));
//    CU_ASSERT_EQUAL(hdb->rcnum, opts.cachedrecords);
//    CU_ASSERT_TRUE(hdb->opts & HDBTDEFLATE);
//    CU_ASSERT_TRUE(hdb->opts & HDBTLARGE);
//    CU_ASSERT_TRUE(ejdbrmcoll(jb, "optscoll", true));
//}
