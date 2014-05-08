project(ejpp CXX C)
cmake_minimum_required(VERSION 2.8.11)

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

option(EJPP_ENABLE_TESTING "Enable ejpp testing" ON)
option(EJPP_SANITIZE_ADDRESS "Use -fsanitize=address where available" OFF)
option(EJPP_LEAK_CHECKER "Enable memory leak checking where available" OFF)

set(EJPP_DOC_OUTPUT_DIR "${CMAKE_CURRENT_SOURCE_DIR}/doc/out" CACHE FILEPATH
"Directory to output generated documentation")

if(${EJPP_ENABLE_TESTING})
enable_testing()
endif()

find_package(EJDB REQUIRED)
find_package(Boost 1.53.0 REQUIRED)
find_package(JBSON REQUIRED)

include_directories(${Boost_INCLUDE_DIRS} ${EJDB_INCLUDE_DIRS} ${JBSON_INCLUDE_DIRS})

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang")
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unknown-warning-option -Wno-unused-private-field -Wno-mismatched-tags")
endif()

if(${EJPP_SANITIZE_ADDRESS})
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=address")
 if(${EJPP_LEAK_CHECKER})
  SET(ENV{ASAN_OPTIONS} "detect_leaks=1")
 endif(${EJPP_LEAK_CHECKER})
elseif(${EJPP_LEAK_CHECKER})
 set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -fsanitize=leak")
endif()

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -fno-omit-frame-pointer -pthread -fvisibility=hidden")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-unused-local-typedefs -Wno-missing-field-initializers")
set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wno-attributes -Wno-unused-parameter")
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
target_link_libraries(ejpp ${EJDB_LIBRARIES} -Wl,--exclude-libs=tcejdb)
set_target_properties(ejpp PROPERTIES COMPILE_DEFINITIONS EJPP_EXPORTS=ejpp_EXPORTS)

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

include(ExternalProject)
ExternalProject_Add(
    gtest_${PROJECT_NAME}
    SVN_REPOSITORY http://googletest.googlecode.com/svn/trunk/
    SVN_REVISION -r 660
    UPDATE_COMMAND ""
    TIMEOUT 10
    CMAKE_ARGS --no-warn-unused-cli ${PARENT_CMAKE_ARGS} "-DCMAKE_CXX_FLAGS=${CMAKE_CXX_FLAGS} -std=c++11" -DBUILD_SHARED_LIBS=OFF
    # Disable install step
    INSTALL_COMMAND ""
    LOG_DOWNLOAD ON
    LOG_CONFIGURE ON
    LOG_BUILD ON)
ExternalProject_Get_Property(gtest_${PROJECT_NAME} source_dir)
set(GTEST_INCLUDE_DIRS ${source_dir}/include)
message(STATUS "Gtest include dirs: ${GTEST_INCLUDE_DIRS}")

include_directories(${GTEST_INCLUDE_DIRS})

ExternalProject_Get_Property(gtest_${PROJECT_NAME} binary_dir)
set(GTEST_LIBRARY_DIR ${binary_dir})
set(GTEST_LIBRARIES "-L${GTEST_LIBRARY_DIR} -lgtest")

add_library(${PROJECT_NAME}_test_main OBJECT test/test_main.cpp)
add_dependencies(${PROJECT_NAME}_test_main gtest_${PROJECT_NAME})

include_directories(${CMAKE_CURRENT_SOURCE_DIR}/include)

function(cxx_test test_name)
  add_executable(${test_name} test/${test_name}.cpp $<TARGET_OBJECTS:${PROJECT_NAME}_test_main>)
  add_dependencies(${test_name} gtest_${PROJECT_NAME})
  target_link_libraries(${test_name} ${LINK_LIBS} ejpp z ${GTEST_LIBRARIES})
  add_test(${test_name} ${EXECUTABLE_OUTPUT_PATH}/${test_name})
endfunction()

cxx_test(ejpp_test1)
cxx_test(ejpp_test2)
cxx_test(ejpp_test3)
#cxx_test(ejpp_test4)

install(TARGETS ejpp ejpp-static
    LIBRARY DESTINATION lib
    ARCHIVE DESTINATION lib)
install(DIRECTORY include/ejpp DESTINATION include/ejpp)

find_package(Doxygen)

if(DOXYGEN_FOUND)
configure_file(${CMAKE_CURRENT_SOURCE_DIR}/Doxyfile.in ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile @ONLY)
add_custom_target(doc
    ${DOXYGEN_EXECUTABLE} ${CMAKE_CURRENT_BINARY_DIR}/Doxyfile
    COMMENT "Generating API documentation with Doxygen" VERBATIM
)
endif(DOXYGEN_FOUND)
