cmake_minimum_required(VERSION 2.8)

project(fcmalloc)

set(default_build_type "Release")
if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to '${default_build_type}' as none was specified.")
    set(CMAKE_BUILD_TYPE "${default_build_type}"
        CACHE STRING "Choose the type of build." FORCE)
    set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS
        "Debug" "Release"
    )
endif()

# global setting
add_compile_options("-std=c++11")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DDEBUG")
set(CMAKE_CXX_FLAGS_RELEASE "-O3")

# fcmalloc library
file(GLOB srcs
    src/*.cpp
)
add_library(fcmalloc SHARED
    ${srcs}
)
target_link_libraries(fcmalloc
    pthread
    dl
)

install(TARGETS fcmalloc
    LIBRARY DESTINATION lib
)
install(FILES fcmalloc.h
    DESTINATION include
)
