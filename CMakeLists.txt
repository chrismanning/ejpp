project(ejpp CXX C)
cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(EJPP_ENABLE_TESTING "Enable ejpp testing" ON)
option(EJPP_SANITIZE_ADDRESS "Use -fsanitize=address where available" OFF)
option(EJPP_SANITIZE_THREAD "Use -fsanitize=thread where available" OFF)
option(EJPP_LEAK_CHECKER "Enable memory leak checking where available" OFF)

if(${EJPP_ENABLE_TESTING})
enable_testing()
endif()

set(EJDB_STATIC_LIB true)
find_package(EJDB REQUIRED)
find_package(Boost 1.53.0 REQUIRED)
find_package(JBSON REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${EJDB_INCLUDE_DIRS} ${JBSON_INCLUDE_DIRS})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option -Wno-unused-private-field -Wno-mismatched-tags")
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

add_library(ejpp-static STATIC ${SRC_LIST})
SET_TARGET_PROPERTIES(ejpp-static PROPERTIES OUTPUT_NAME ejpp)
target_link_libraries(ejpp-static ${EJDB_LIBRARIES})

add_library(ejpp SHARED ${SRC_LIST})
target_link_libraries(ejpp ${EJDB_LIBRARIES})

install(TARGETS ejpp ejpp-static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(DIRECTORY include/ejpp DESTINATION include/ejpp)
