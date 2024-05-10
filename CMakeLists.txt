cmake_minimum_required(VERSION 3.14)
project(Hashtable)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED True)

find_program(JEMALLOC_FOUND jemalloc-config)
if(JEMALLOC_FOUND)
	execute_process(
		COMMAND jemalloc-config --libs
		OUTPUT_VARIABLE JEMALLOC_LIBS
		OUTPUT_STRIP_TRAILING_WHITESPACE)
	set(JEMALLOC_LIBS "${JEMALLOC_LIBS} -ljemalloc")
endif()

include(FetchContent)
FetchContent_Declare(
	parlaylib
	GIT_REPOSITORY https://github.com/cmuparlay/parlaylib.git
	GIT_TAG master
)
FetchContent_MakeAvailable(parlaylib)

add_executable(hashtable hashtable.cpp)
target_include_directories(hashtable PRIVATE ${parlaylib_SOURCE_DIR} ${parlaylib_SOURCE_DIR}/include/)
target_link_libraries(hashtable PRIVATE ${JEMALLOC_LIBS} "-pthread")
