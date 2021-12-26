# CMakeList.txt : Top-level CMake project file, do global configuration
# and include sub-projects here.

cmake_minimum_required (VERSION 3.8)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_LIST_DIR}/bin)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_LIST_DIR}/resources.rc")

project ("PlumbersDontWearTies" VERSION "0.1.0")

configure_file("${CMAKE_CURRENT_LIST_DIR}/PlumbersDontWearTies/config.h.in" "${CMAKE_CURRENT_LIST_DIR}/PlumbersDontWearTies/config.h")

if(MSVC)
    set(SDL2_INCLUDE_DIRS ${CMAKE_CURRENT_LIST_DIR}/SDL2/include)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8) # 64 bits
        set(SDL2_LIB_DIRS ${CMAKE_CURRENT_LIST_DIR}/SDL2/lib/x64)
    else() # 32 bits
        set(SDL2_LIB_DIRS ${CMAKE_CURRENT_LIST_DIR}/SDL2/lib/x86)
    endif()

    set(SDL2_LIBS SDL2main SDL2 SDL2_ttf)

    link_directories(${SDL2_LIB_DIRS})
else()
    find_package(SDL2 REQUIRED)
    set(SDL2_LIBS SDL2 SDL2_ttf)
endif()

include_directories(${SDL2_INCLUDE_DIRS})

# Include sub-projects.
add_subdirectory ("PlumbersDontWearTies")