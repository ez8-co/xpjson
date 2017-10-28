CMAKE_MINIMUM_REQUIRED(VERSION 2.8)
CMAKE_POLICY(VERSION 2.8)

PROJECT(example)

IF (CMAKE_SYSTEM_NAME MATCHES "Linux")

    SET(CMAKE_BUILD_TYPE "Debug")
    SET(CMAKE_CXX_FLAGS_DEBUG "$ENV{CXXFLAGS} -O0 -Wall -g2 -ggdb")
    SET(CMAKE_CXX_FLAGS_RELEASE "$ENV{CXXFLAGS} -O3 -Wall")
ELSEIF (CMAKE_SYSTEM_NAME MATCHES "Windows")
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

ADD_EXECUTABLE(xpjson_example1 "example1.cpp")

ADD_EXECUTABLE(xpjson_example2 "example2.cpp")

ADD_EXECUTABLE(xpjson_example3 "example3.cpp")