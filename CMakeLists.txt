cmake_minimum_required(VERSION 3.0.1)
project(Agos)

macro(USE_C99)
    if (CMAKE_VERSION VERSION_LESS "3.1")
        if (CMAKE_C_COMPILER_ID STREQUAL "GNU")
            SET(CMAKE_C_FLAGS "-std=gnu99 ${CMAKE_C_FLAGS}")
        endif(CMAKE_C_COMPILER_ID)
    else()
        SET(CMAKE_C_STANDARD 99)
    endif()
endmacro(USE_C99)

USE_C99()

SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall")

SET( CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/bin )

FIND_PACKAGE(LibXml2)
if (LIBXML2_FOUND)
    SET (CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -I ${LIBXML2_INCLUDE_DIR}")
    SET (CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -I ${LIBXML2_INCLUDE_DIR}")
endif()

ADD_SUBDIRECTORY(src/string)

SET(SOURCE_FILES src/main.c src/opts.c src/opts.h src/compile.h  src/fsm.h  src/fsm_util.h  src/lex.h)

ADD_EXECUTABLE(Agos ${SOURCE_FILES})

TARGET_LINK_LIBRARIES(Agos ${LIBXML2_LIBRARIES} string)