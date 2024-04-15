cmake_minimum_required(VERSION 3.19)

project(codana VERSION 0.0 HOMEPAGE_URL "https://github.com/drepper/termdetect" LANGUAGES C CXX)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug)
endif()
if(CMAKE_INSTALL_PREFIX_INITIALIZED_TO_DEFAULT)
    set(CMAKE_INSTALL_PREFIX "/usr")
endif()

set(debug_opts "-O0 -g3 -fvar-tracking -fvar-tracking-assignments -D_GLIBCXX_DEBUG -DDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "${debug_opts}")
set(CMAKE_C_FLAGS_DEBUG "${debug_opts}")

set(release_opts "-O3 -g -flto -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "${release_opts}")
set(CMAKE_C_FLAGS_RELEASE "${release_opts}")

set(relwithdebinfo_opts "-O2 -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${relwithdebinfo_opts}")
set(CMAKE_C_FLAGS_RELWITHDEBINFO "${relwithdebinfo_opts}")

set(minsizerel_opts "-Os -g -DNDEBUG")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${minsizerel_opts}")
set(CMAKE_C_FLAGS_MINSIZEREL "${minsizerel_opts}")

set(gcc_opts "-fsave-optimization-record -frecord-gcc-switches -fdiagnostics-color=always -fno-operator-names")
set(common_warnings "-Wall -Wextra -Wnull-dereference -Wdouble-promotion -Wshadow -Wformat=2 -Wcast-qual -Wcast-align -Wstrict-aliasing -Wpointer-arith -Winit-self -Wredundant-decls -Wundef -Wempty-body -Wduplicated-cond -Wduplicated-branches -Wlogical-op -Wrestrict")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)
string(APPEND CMAKE_CXX_FLAGS " ${gcc_opts} ${common_warnings} -Wuseless-cast -Wsuggest-override -Weffc++")

include(CheckPIESupported)
check_pie_supported()

enable_testing()
cmake_policy(SET CMP0110 NEW)

add_library(termdetect STATIC termdetect.cc termdetect.hh)

add_test(NAME "initialization" COMMAND inittest)
add_executable(inittest inittest.cc)
target_link_libraries(inittest termdetect)
