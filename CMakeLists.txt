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

get_cmake_property(CACHE_VARS CACHE_VARIABLES)
foreach(CACHE_VAR ${CACHE_VARS})
  get_property(CACHE_VAR_HELPSTRING CACHE ${CACHE_VAR} PROPERTY HELPSTRING)
  if(CACHE_VAR_HELPSTRING STREQUAL "No help, variable specified on the command line.")
    get_property(CACHE_VAR_TYPE CACHE ${CACHE_VAR} PROPERTY TYPE)
    if(CACHE_VAR_TYPE STREQUAL "UNINITIALIZED")
      set(CACHE_VAR_TYPE)
    else()
      set(CACHE_VAR_TYPE :${CACHE_VAR_TYPE})
    endif()
    set(PARENT_CMAKE_ARGS ${PARENT_CMAKE_ARGS} "-D${CACHE_VAR}${CACHE_VAR_TYPE}=${${CACHE_VAR}}")
  endif()
endforeach()
message("PARENT_CMAKE_ARGS: ${PARENT_CMAKE_ARGS}")

include(ExternalProject)
ExternalProject_Add(bsoncpp
PREFIX ${CMAKE_CURRENT_SOURCE_DIR}/bson-cpp
SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/bson-cpp
CMAKE_ARGS --no-warn-unused-cli ${PARENT_CMAKE_ARGS}
 -DCMAKE_INSTALL_PREFIX=${CMAKE_SOURCE_DIR} "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS}"
INSTALL_DIR ${CMAKE_SOURCE_DIR}
)

set(SRC_LIST ${SRC_LIST} src/ejpp/ejdb.cpp include/ejpp/ejdb.hpp)
set(SRC_LIST ${SRC_LIST} src/ejpp/c_ejdb.cpp include/ejpp/c_ejdb.hpp)

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

message(STATUS "EJDB libs: " ${EJDB_LIBRARIES})

add_library(ejpp-static STATIC ${SRC_LIST})
SET_TARGET_PROPERTIES(ejpp-static PROPERTIES OUTPUT_NAME ejpp)
target_link_libraries(ejpp-static ${EJDB_LIBRARIES} -L${CMAKE_SOURCE_DIR}/lib/ -lbsoncpp)
add_dependencies(ejpp-static bsoncpp)

add_library(ejpp SHARED ${SRC_LIST})
target_link_libraries(ejpp ${EJDB_LIBRARIES} -L${CMAKE_SOURCE_DIR}/lib/ -lbsoncpp)
add_dependencies(ejpp bsoncpp)

install(TARGETS ejpp ejpp-static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(DIRECTORY include/ejpp DESTINATION include/ejpp)
install(DIRECTORY inc/bson DESTINATION include/bson
    FILES_MATCHING PATTERN "*.h"
)
install(DIRECTORY lib/bson DESTINATION lib/bson)
