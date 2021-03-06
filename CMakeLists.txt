cmake_minimum_required(VERSION 3.13)

# set up project
project(IslandHopper DESCRIPTION "fun fun fun")

# set path to cmake modules
set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/vendor/igor/src/cmake
                      ${CMAKE_MODULE_PATH})

set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Set a default build type if none was specified
set(default_build_type "Release")
if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/.git")
  set(default_build_type "RelWithDebInfo")
endif()

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(
    STATUS
      "Setting build type to '${default_build_type}' as none was specified.")
  set(CMAKE_BUILD_TYPE
      "${default_build_type}"
      CACHE STRING "Choose the type of build." FORCE)
  # Set the possible values of build type for cmake-gui
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
                                               "RelWithDebInfo")
endif()

if(${CMAKE_BUILD_TYPE} STREQUAL "RelWithDebInfo")
  add_definitions(-DRELWITHDEBINFO)
endif()

# global compiler/linker settings linux
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  message("using GCC")
  if(DEFINED CMAKE_BUILD_TYPE)
    message("build type ${CMAKE_BUILD_TYPE}")
  endif()
  add_compile_options(-fpermissive -std=c++17)
  add_definitions(-D_NEWTON_USE_DOUBLE -D_cplusplus=201703L)
  if(${CMAKE_BUILD_TYPE} STREQUAL "Debug")
    add_definitions(-DDEBUG)
  elseif()
    add_definitions(-DNDEBUG)
  endif()
  set(COMPILER "GCC")

  # windows
elseif(MSVC_VERSION GREATER_EQUAL "1920")
  message("using Visual Studio C++")
  add_compile_options("/std:c++17" "/MP" "/W2")
  add_definitions(-D_UNICODE -DUNICODE -DNOMINMAX -D_NEWTON_USE_DOUBLE)
  set(COMPILER "MSVCXX")
elseif()
  message(FATAL_ERROR "unsupported environment")
endif()

# igor
add_subdirectory(vendor/igor)

# Island Hopper
set(IH_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src/")
set(IH_HEADERS_DIR ${IH_SRC_DIR})
file(
  GLOB_RECURSE IH_SOURCES
  LIST_DIRECTORIES false
  "${IH_SRC_DIR}*.c*")
file(
  GLOB_RECURSE IH_HEADERS
  LIST_DIRECTORIES false
  "${IH_HEADERS_DIR}*.h*" "${IH_HEADERS_DIR}*.inl")
add_executable(IslandHopper ${IH_SOURCES} ${IH_HEADERS})
target_include_directories(
  IslandHopper PRIVATE ${IGOR_HEADERS_DIR} ${IAUX_HEADERS_DIR}
                       ${OMPF_HEADERS_DIR})
target_link_libraries(IslandHopper igor)
