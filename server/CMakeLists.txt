cmake_minimum_required(VERSION 3.10)
project(server)

set(CMAKE_CXX_STANDARD 17)
set(Boost_USE_STATIC_LIBS        ON) 
set(Boost_USE_DEBUG_LIBS        OFF)
set(Boost_USE_RELEASE_LIBS       ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)

file(GLOB_RECURSE SERVER_HEADERS ${PROJECT_SOURCE_DIR}/include/*.hxx)
file(GLOB_RECURSE SERVER_SOURCES ${PROJECT_SOURCE_DIR}/*.cxx)

find_package(Boost 1.74.0 COMPONENTS date_time system)

if (Boost_FOUND)
    include_directories(${Boost_LIBRARIES})
    add_executable(${PROJECT_NAME} ${SERVER_HEADERS} ${SERVER_SOURCES} server.cc)
    target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES})
else()
    message(FATAL_ERROR "boost not found.")
endif()

# include(FetchContent)

# FetchContent_Declare(
#   googletest
#   URL https://github.com/google/googletest/archive/03597a01ee50ed33e9dfd640b249b4be3799d395.zip
# )

# FetchContent_MakeAvailable(googletest)

# # Настройте тесты на основе Google Test
# if(GTEST_FOUND)
#   enable_testing()
#   include(GoogleTest)
# endif()


# add_executable(run user.cc)

# add_executable(runTests test.cc)


# add_custom_target(coverage
#   COMMAND gcovr -r ${CMAKE_CURRENT_SOURCE_DIR} --exclude '.*gtest.*' --html --html-details -o coverage.html
#   DEPENDS run runTests
# )


# target_link_libraries(runTests gtest_main pthread)


# set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")
# set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -g -O0 -fprofile-arcs -ftest-coverage")



