cmake_minimum_required(VERSION 3.23)
project(gdpacman LANGUAGES C CXX)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON) 

find_package(libgit2 REQUIRED)
find_package(Boost REQUIRED)
find_package(termcolor REQUIRED)

add_executable(gdpacman
    src/main.cpp
)

target_link_libraries(gdpacman 
    libgit2::libgit2
    Boost::filesystem
    Boost::program_options
    Boost::boost
    Boost::json
    termcolor::termcolor
)

target_include_directories(gdpacman PUBLIC 
    "${PROJECT_BINARY_DIR}"
)

install(TARGETS gdpacman DESTINATION bin)