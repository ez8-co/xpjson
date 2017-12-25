CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
CMAKE_POLICY(VERSION 2.8)

PROJECT(xpjson_ut)
AUX_SOURCE_DIRECTORY(. DIR_SRCS)

include_directories(${GTEST_INCLUDE_DIRS})

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    FIND_PACKAGE(GTest REQUIRED)
    FIND_PACKAGE(Threads REQUIRED)
    LINK_LIBRARIES(gtest pthread)

    SET(CMAKE_BUILD_TYPE "Debug")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
    SET(CMAKE_BUILD_TYPE "Debug")
    SET(CompilerFlags
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_C_FLAGS_DEBUG
        )
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -D_DEBUG")
    FOREACH(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MDd" "/MTd" ${CompilerFlag} "${${CompilerFlag}}")
    ENDFOREACH()
    SET(CompilerFlags
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS_RELEASE
        )
    FOREACH(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    ENDFOREACH()
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

# ADD_DEFINITIONS(-D__XPJSON_SUPPORT_DANGLING_COMMA__=1)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -std=c++0x")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -std=c++0x")
    ADD_DEFINITIONS(-D_GXX_EXPERIMENTAL_CXX0X__)
ENDIF (CMAKE_SYSTEM_NAME MATCHES "Linux")

ADD_EXECUTABLE(xpjson_ut ${DIR_SRCS})

# Options. Turn on with 'cmake -Dmyvarname=ON'.
OPTION(BUILD_TESTS "Build all tests." OFF) # Makes boolean 'test' available.

IF(BUILD_TESTS)
    ENABLE_TESTING()
    ADD_TEST(ut xpjson_ut)

    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -pthread -g -O0 -fprofile-arcs -ftest-coverage")
    ADD_EXECUTABLE(xpjson_coverage ${DIR_SRCS})
    SET(CMAKE_MODULE_PATH .)
    INCLUDE(CodeCoverage)
    SETUP_TARGET_FOR_COVERAGE(coverage xpjson_coverage coverage)
ENDIF()