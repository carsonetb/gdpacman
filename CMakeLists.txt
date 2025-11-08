cmake_minimum_required(VERSION 3.23)
project(gdpacman LANGUAGES C CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

set(BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(BUILD_CLAR  OFF CACHE BOOL "" FORCE)
set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

add_subdirectory(deps/boost)
add_subdirectory(deps/libgit2)

add_executable(gdpacman
    src/main.cpp
)

target_link_libraries(gdpacman PRIVATE
    Boost::filesystem
    Boost::program_options
    Boost::range
    libgit2package
)

target_include_directories(gdpacman PRIVATE deps/libgit2/include)

install(TARGETS gdpacman DESTINATION bin)