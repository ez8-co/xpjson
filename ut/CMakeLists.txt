CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
CMAKE_POLICY(VERSION 2.8)

PROJECT(xpjson_ut)
AUX_SOURCE_DIRECTORY(. DIR_SRCS)
FIND_PACKAGE(GTest REQUIRED)

include_directories(${GTEST_INCLUDE_DIRS})

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    FIND_PACKAGE(Threads REQUIRED)
    LINK_LIBRARIES(gtest pthread)

    SET(CMAKE_BUILD_TYPE "Debug")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    LINK_LIBRARIES(gtest)
    SET(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE
        )
    FOREACH(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
        string(REPLACE "/MTd" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    ENDFOREACH()
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

ADD_EXECUTABLE(xpjson_ut ${DIR_SRCS})

ADD_DEFINITIONS(-D__XPJSON_SUPPORT_DANGLING_COMMA__=1)
ADD_EXECUTABLE(xpjson_dangling_comma_ut ${DIR_SRCS})

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -std=c++0x")  
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -std=c++0x")
    ADD_DEFINITIONS(-D_GXX_EXPERIMENTAL_CXX0X__)
    ADD_EXECUTABLE(xpjson_cxx0x_ut ${DIR_SRCS})
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

# Options. Turn on with 'cmake -Dmyvarname=ON'.
option(BUILD_TESTS "Build all tests." OFF) # Makes boolean 'test' available.

IF(BUILD_TESTS)
    ENABLE_TESTING()
    ADD_TEST(UT xpjson_ut)
    ADD_TEST(DANGLING_COMMA xpjson_dangling_comma_ut)
    ADD_TEST(C++11 xpjson_cxx0x_ut)

    IF (CMAKE_BUILD_TYPE STREQUAL "Coverage")
        INCLUDE(CodeCoverage)
        SETUP_TARGET_FOR_COVERAGE(xpjson_coverage xpjson_ut coverage)

        SET(CMAKE_CXX_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
        SET(CMAKE_C_FLAGS "-g -O0 -fprofile-arcs -ftest-coverage")
    ENDIF() #CMAKE_BUILD_TYPE STREQUAL "Coverage"
ENDIF()