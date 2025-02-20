cmake_minimum_required(VERSION 3.12)
project(KV-Evaluator VERSION 1.0 LANGUAGES CXX)

# specify c/c++ standard
if(NOT CMAKE_CXX_STANDARD)
  set(CMAKE_CXX_STANDARD 17)
  set(CMAKE_CXX_STANDARD_REQUIRED ON)
  set(CMAKE_CXX_EXTENSIONS OFF)
endif(NOT CMAKE_CXX_STANDARD)

list(APPEND CMAKE_PREFIX_PATH "${PROJECT_SOURCE_DIR}/lib/libmkl")

set(SRC_DIR "${PROJECT_SOURCE_DIR}/src")

if (MSVC)
  set(CMAKE_CXX_FLAGS "/O2 /arch:AVX2 /W1 /EHsc")
elseif (CMAKE_CXX_COMPILER_ID STREQUAL "Intel")
  set(CMAKE_CXX_FLAGS "-O3 -xHost -ggdb3 -pthread")
else ()
  set(CMAKE_CXX_FLAGS "-O3 -march=native -ggdb3 -pthread")
endif ()

include_directories(
  ${SRC_DIR}
)

add_executable(encode "${SRC_DIR}/encode.cc")
# add_executable(encode_huf "${SRC_DIR}/encode_huf.cc")
# add_executable(encode_huf_ext "${SRC_DIR}/encode_huf_ext.cc")
add_executable(analyze "${SRC_DIR}/analyze.cc")
add_executable(analyze_freq "${SRC_DIR}/analyze_freq.cc")
add_executable(format "${SRC_DIR}/format.cc")
add_executable(truncate "${SRC_DIR}/truncate.cc")
add_executable(binarize "${SRC_DIR}/binarize.cc")

find_package(Boost COMPONENTS program_options REQUIRED)
if (Boost_FOUND)
  include_directories(${Boost_INCLUDE_DIRS})
  target_link_libraries(encode ${Boost_LIBRARIES})
  # target_link_libraries(encode_huf ${Boost_LIBRARIES})
  # target_link_libraries(encode_huf_ext ${Boost_LIBRARIES})
  target_link_libraries(analyze ${Boost_LIBRARIES})
  target_link_libraries(analyze_freq ${Boost_LIBRARIES})
  target_link_libraries(format ${Boost_LIBRARIES})
  target_link_libraries(truncate ${Boost_LIBRARIES})
  target_link_libraries(binarize ${Boost_LIBRARIES})
else ()
  message(WARNING "Boost libs are not found")
endif ()