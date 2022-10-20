# [CMake]
cmake_minimum_required(VERSION 3.24 FATAL_ERROR)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
include(cmake/test_utils.cmake)

# [Project]
project(redbase
  VERSION 0.0.1
  LANGUAGES CXX
  DESCRIPTION "a implementation of redbase, for stanford's CS346 Spring"
  )

# [Compiler]
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_FLAGS "-fPIC")

option(ENABLE_TEST "Whether or not to enable tests" OFF)
set(CMAKE_BUILD_TYPE "Debug")

# [Source]
set(SRC_LIST
  src/pf_error.cc
  )

# [Targets]

# [Targets/Library]
add_library(${PROJECT_NAME} SHARED ${SRC_LIST})

# [Targets/Executable]
add_executable("${PROJECT_NAME}-cli" src/redbase.cc)
target_link_libraries("${PROJECT_NAME}-cli" PRIVATE ${PROJECT_NAME})

# [Targets/Test]
if (${ENABLE_TEST})
  message(STATUS "Tests is enable")
  enable_testing()

  # 添加测试 add_unit_test(test_name test_libraries)
  # 注意需要在tests目录下创建于test_name同名的源文件 如 test_name.cc

  add_unit_test(project_setting ${PROJECT_NAME})

else ()
  message (STATUS "Tests is off")
endif ()
