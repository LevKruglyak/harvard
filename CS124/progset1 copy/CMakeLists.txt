cmake_minimum_required (VERSION 2.8.11)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
#add_compile_options(-fsanitize=address)
#add_link_options(-fsanitize=address)

#set(CMAKE_BUILD_TYPE Debug)
set(CMAKE_CXX_FLAGS "-Wall -Wextra -O3")

add_executable(randmst helpers.cpp graphs.cpp main.cpp)
target_compile_features(randmst PUBLIC cxx_std_17)

