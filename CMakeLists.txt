cmake_minimum_required(VERSION 3.0)
project(new_words_discoverer)

set(SRC_FILES 
    ${CMAKE_CURRENT_SOURCE_DIR}/newwords_discoverer/main.cpp 
    ${CMAKE_CURRENT_SOURCE_DIR}/newwords_discoverer/Discoverer.cpp)

# use static runtime in MSVC
if(MSVC)
    set(CompilerFlags
        CMAKE_CXX_FLAGS
        CMAKE_CXX_FLAGS_DEBUG
        CMAKE_CXX_FLAGS_RELEASE
        CMAKE_C_FLAGS
        CMAKE_C_FLAGS_DEBUG
        CMAKE_C_FLAGS_RELEASE)
    foreach(CompilerFlag ${CompilerFlags})
        string(REPLACE "/MD" "/MT" ${CompilerFlag} "${${CompilerFlag}}")
    endforeach()
endif()

# find boost
set(Boost_USE_STATIC_LIBS ON)
find_package(Boost REQUIRED COMPONENTS program_options)
include_directories(${Boost_INCLUDE_DIRS})
link_directories(${BOOST_LIBRARYDIR})

add_executable(${PROJECT_NAME} ${SRC_FILES})
set_property(TARGET ${PROJECT_NAME} 
    PROPERTY CXX_STANDARD 14)
target_link_libraries(${PROJECT_NAME} ${Boost_LIBRARIES} pthread)
install(TARGETS ${PROJECT_NAME} DESTINATION ${CMAKE_CURRENT_SOURCE_DIR}/install)