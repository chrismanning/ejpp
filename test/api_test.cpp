/**************************************************************************
**  Copyright (C) 2014 Christian Manning
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

#define private public
#include <ejpp/ejdb.hpp>
#include <jbson/builder.hpp>

#include <gtest/gtest.h>

TEST(ApiTest, ErrorTest1) {
    std::error_code ec;

    ejdb::db jb{};
    ASSERT_FALSE(static_cast<bool>(jb));

    ejdb::collection coll;
    ASSERT_FALSE(static_cast<bool>(coll));
    ASSERT_EQ(&coll, coll.transaction().m_collection);

    ASSERT_NO_THROW(coll = jb.get_collection("coll", ec));
    EXPECT_FALSE(static_cast<bool>(coll));
    EXPECT_TRUE(static_cast<bool>(ec));
    EXPECT_EQ((int)std::errc::operation_not_permitted, ec.value());
    ec.clear();

    ASSERT_NO_THROW(jb.open("db_apisdf", ejdb::db_mode::read | ejdb::db_mode::write, ec));
    EXPECT_FALSE(jb.is_open());
    EXPECT_TRUE(static_cast<bool>(ec));
    EXPECT_EQ((int)ejdb::errc::file_not_found, ec.value());
    ec.clear();

    ASSERT_NO_THROW(jb.open(
        "db_api", ejdb::db_mode::read | ejdb::db_mode::write | ejdb::db_mode::create | ejdb::db_mode::truncate, ec));
    EXPECT_TRUE(jb.is_open());
    EXPECT_FALSE(static_cast<bool>(ec));
    EXPECT_EQ((int)ejdb::errc::success, ec.value());
    ec.clear();

    ASSERT_NO_THROW(jb.remove_collection("coll", true));

    // removing non-existant collection is not an error
    ASSERT_NO_THROW(EXPECT_TRUE(jb.remove_collection("coll", true, ec)));
    EXPECT_FALSE(static_cast<bool>(ec));
    EXPECT_EQ((int)ejdb::errc::success, ec.value());

    // collection not existing is not an error
    ASSERT_NO_THROW(coll = jb.get_collection("coll"));
    EXPECT_FALSE(static_cast<bool>(coll));

    ASSERT_NO_THROW(coll = jb.create_collection("coll"));
    EXPECT_TRUE(static_cast<bool>(coll));

    ASSERT_NO_THROW(coll = jb.get_collection("coll"));
    EXPECT_TRUE(static_cast<bool>(coll));

    {
        auto doc = coll.load_document({{0}}, ec);
        ASSERT_TRUE(doc.empty());
        EXPECT_FALSE(static_cast<bool>(ec));
    }

    {
        std::vector<char> doc;
        EXPECT_NO_THROW(doc = coll.load_document({{0}}));
        ASSERT_TRUE(doc.empty());
    }

    ASSERT_NO_THROW(jb.sync());

    {
        std::vector<char> doc;
        EXPECT_NO_THROW(doc = jb.metadata());
        ASSERT_FALSE(doc.empty());
    }

    {
        std::vector<char> doc{};
        ASSERT_TRUE(doc.empty());
        EXPECT_THROW(coll.save_document(doc), std::system_error);
    }

    {
        std::vector<char> doc{{0, 0, 0, 0, 0}};
        EXPECT_THROW(coll.save_document(doc), std::system_error);
    }

    std::array<char, 12> oid;
    {
        std::vector<char> doc{{5, 0, 0, 0, 0}};
        ASSERT_NO_THROW(oid = coll.save_document(doc));
    }

    EXPECT_THROW(ejdb::query() |= std::vector<char>{}, std::system_error);

    ASSERT_NO_THROW(EXPECT_EQ(1u, coll.get_all().size()));
    EXPECT_NO_THROW(coll.remove_document(oid));
}
