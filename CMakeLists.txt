project(onireader)
cmake_minimum_required(VERSION 3.4)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} ${CMAKE_SOURCE_DIR}/cmake)

find_package(OpenNI2 REQUIRED)
include_directories(${OPENNI2_INCLUDE_DIRS})

add_subdirectory(pybind11)

if (SKBUILD)
  find_package(PythonExtensions REQUIRED)
endif(SKBUILD)

set(CMAKE_CXX_FLAGS -fPIC)
add_subdirectory(onireader)


#add_subdirectory(sample)
