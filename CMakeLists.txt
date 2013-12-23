project(ejpp CXX C)
cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

set(EJDB_STATIC_LIB true)
find_package(EJDB REQUIRED)
find_package(Boost REQUIRED)
include_directories(${Boost_INCLUDE_DIRS} ${EJDB_INCLUDE_DIRS})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option -Wno-unused-private-field -Wno-mismatched-tags")
 set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fno-omit-frame-pointer -pthread")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-local-typedefs -Wno-missing-field-initializers -Wno-attributes -Wno-unused-parameter")
if(NOT WIN32)
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fPIC")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${CMAKE_C_FLAGS} -std=c++1y")

set(CMAKE_C_FLAGS_DEBUG "${CMAKE_C_FLAGS} -g")

set(SRC_LIST ${SRC_LIST} src/ejpp/ejdb.cpp include/ejpp/ejdb.hpp)
set(SRC_LIST ${SRC_LIST} src/ejpp/c_ejdb.cpp include/ejpp/c_ejdb.hpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

message(STATUS "EJDB libs: " ${EJDB_LIBRARIES})

add_library(ejpp-static STATIC ${SRC_LIST})
SET_TARGET_PROPERTIES(ejpp-static PROPERTIES OUTPUT_NAME ejpp)
target_link_libraries(ejpp-static ${EJDB_LIBRARIES})

set(JBSON_ENABLE_TESTING ON)
add_subdirectory(jbson)
include_directories(jbson/include)

add_library(ejpp SHARED ${SRC_LIST})
target_link_libraries(ejpp ${EJDB_LIBRARIES})

install(TARGETS ejpp ejpp-static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(DIRECTORY include/ejpp DESTINATION include/ejpp)
install(DIRECTORY include/jbson DESTINATION include/jbson)
