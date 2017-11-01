CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
CMAKE_POLICY(VERSION 2.8)

PROJECT(xpjson_bench)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")
    LINK_LIBRARIES(gtest pthread)

    SET(CMAKE_BUILD_TYPE "Release")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb -std=c++0x")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall -std=c++0x")
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

ADD_EXECUTABLE(xpjson_bench "benchmark.cpp")