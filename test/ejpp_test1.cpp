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

#include <boost/range/adaptor/filtered.hpp>

#include <ejpp/ejdb.hpp>
#include <jbson/document.hpp>
#include <jbson/builder.hpp>
#include <jbson/json_reader.hpp>
using namespace jbson;

#include <gtest/gtest.h>

struct EjdbTest1 : ::testing::Test {
    void SetUp() override {
        std::error_code ec;
        auto r = jb.open("dbt1", ejdb::db_mode::write|ejdb::db_mode::create|ejdb::db_mode::truncate, ec);
        ASSERT_TRUE(r);
        ASSERT_FALSE(ec);
        ASSERT_TRUE(static_cast<bool>(jb));
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

    ejdb::db jb;
};

TEST_F(EjdbTest1, TestSaveLoad) {
    ASSERT_TRUE(static_cast<bool>(jb));

    std::error_code ec;
    std::array<char, 12> oid;

    auto ccoll = jb.create_collection("contacts", ec);
    ASSERT_TRUE(static_cast<bool>(ccoll));
    ASSERT_FALSE(static_cast<bool>(ec));

    //Save record
    auto a1 = u8R"({
        "name": "Петров Петр",
        "phone": "333-222-333",
        "age": 33,
        "longage": 2.8147497671e+14,
        "doubleage": 0.333333
    })"_json_doc;

    auto o_oid = ccoll.save_document(a1.data(), ec);
    ASSERT_FALSE(static_cast<bool>(ec));
    ASSERT_TRUE(static_cast<bool>(o_oid));
    oid = *o_oid;
    EXPECT_NE(boost::as_literal("\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"), oid);

    auto o_lbson = ccoll.load_document(oid, ec);
    ASSERT_FALSE(static_cast<bool>(ec));
    ASSERT_TRUE(!o_lbson.empty());

    auto lbson = jbson::document(std::move(o_lbson));
    ASSERT_EQ(a1, boost::adaptors::filter(lbson, [](auto&& v) {
        return v.name() != "_id";
    }));
}

TEST_F(EjdbTest1, TestBuildQuery1) {
    ASSERT_TRUE(static_cast<bool>(jb));

    std::error_code ec;

    jbson::document q1 = u8R"({
        "name" : "Петров Петр",
        "age"  : 33,
        "family" : {
            "wife" : {
                "name"  : "Jeniffer",
                "age"   : {"$gt" : 25},
                "phone" : "444-111"
            },
            "children" : [
                {
                    "name" : "Dasha",
                    "age" : {"$in" : [1, 4, 10]}
                }
            ]
         }
    })"_json_doc;

    auto ejq = jb.create_query(q1.data(), ec);
    ASSERT_FALSE(static_cast<bool>(ec));
    ASSERT_TRUE(static_cast<bool>(ejq));
}
