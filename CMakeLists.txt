cmake_minimum_required(VERSION 3.21)
project(mslug3 VERSION 0.1.0 LANGUAGES C CXX)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# The neogeorecomp toolkit: the in-tree copy (a submodule in a clean
# checkout), or a checkout next to this project.
if(EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp/CMakeLists.txt)
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/neogeorecomp)
else()
    set(ng_default ${CMAKE_CURRENT_SOURCE_DIR}/../neogeorecomp)
endif()
set(NEOGEORECOMP_DIR ${ng_default} CACHE PATH "neogeorecomp checkout")
add_subdirectory(${NEOGEORECOMP_DIR} neogeorecomp_build)

# Your ROM files: the MAME "mslug3" set and the "neogeo" system ROMs,
# unzipped into one directory. Without it the player builds interpreter-only.
set(MSLUG3_ROM_DIR "" CACHE PATH "Directory with your mslug3 and Neo Geo BIOS ROM files")
# Optional: entry points a previous run found at runtime (--dump-misses).
set(MSLUG3_ENTRIES "${CMAKE_BINARY_DIR}/entries.txt" CACHE FILEPATH "Extra entry points for the recompiler")

# Stage 0: ROM decryption (NEO-SMA program + NEO-CMC42 graphics + S ROM).
add_executable(mslug3_decrypt tools/decrypt.c)

# Stage 1: the generator.
add_executable(mslug3_recomp src/recomp_main.c src/game.c)
target_link_libraries(mslug3_recomp PRIVATE m68krecomp_core)
target_include_directories(mslug3_recomp PRIVATE src)

# Stage 2: the player, with the generated C when ROMs are available.
set(GAME_SOURCES src/main.c)
set(GEN_DIR ${CMAKE_BINARY_DIR}/generated)
set(GEN_FILES 16)
if(MSLUG3_ROM_DIR)
    set(DEC_DIR ${CMAKE_BINARY_DIR}/roms)
    set(DEC_STAMP ${DEC_DIR}/.decrypted)
    add_custom_command(
        OUTPUT ${DEC_STAMP}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${DEC_DIR}
        COMMAND mslug3_decrypt --rom-path ${MSLUG3_ROM_DIR} --out ${DEC_DIR}
        COMMAND ${CMAKE_COMMAND} -E touch ${DEC_STAMP}
        DEPENDS mslug3_decrypt
        COMMENT "Decrypting Metal Slug 3 ROMs (NEO-SMA + NEO-CMC42)")

    set(GEN_SOURCES ${GEN_DIR}/recomp_table.c)
    math(EXPR last "${GEN_FILES} - 1")
    foreach(i RANGE ${last})
        if(i LESS 10)
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_0${i}.c)
        else()
            list(APPEND GEN_SOURCES ${GEN_DIR}/recomp_${i}.c)
        endif()
    endforeach()
    # The entry list starts empty; the `profile` target fills it from a
    # scripted play session and the next build regenerates with it.
    if(NOT EXISTS ${MSLUG3_ENTRIES})
        file(WRITE ${MSLUG3_ENTRIES} "")
    endif()
    file(MAKE_DIRECTORY ${GEN_DIR})
    add_custom_command(
        OUTPUT ${GEN_SOURCES} ${GEN_DIR}/recomp_common.h
        COMMAND mslug3_recomp --rom-path ${DEC_DIR} --out ${GEN_DIR} --files ${GEN_FILES}
                --entries ${MSLUG3_ENTRIES}
        DEPENDS mslug3_recomp ${MSLUG3_ENTRIES} ${DEC_STAMP}
        COMMENT "Recompiling Metal Slug 3 from ${DEC_DIR}")
    list(APPEND GAME_SOURCES ${GEN_SOURCES})
endif()

add_executable(mslug3 ${GAME_SOURCES} src/game.c)
set_target_properties(mslug3 PROPERTIES OUTPUT_NAME "Metal_Slug_3")
target_link_libraries(mslug3 PRIVATE neogeorecomp)
target_include_directories(mslug3 PRIVATE src ${GEN_DIR})
if(MSLUG3_ROM_DIR)
    add_custom_target(profile
        COMMAND mslug3 --headless --rom-path ${DEC_DIR} --frames 6000 --dump-misses ${MSLUG3_ENTRIES}
        COMMAND mslug3 --headless --rom-path ${DEC_DIR} --frames 19000
                --input ${CMAKE_CURRENT_SOURCE_DIR}/tests/play.txt --dump-misses ${MSLUG3_ENTRIES}
        DEPENDS mslug3
        COMMENT "Profiling a scripted session for runtime-only entry points")
    target_compile_definitions(mslug3 PRIVATE MSLUG3_HAVE_RECOMP)
    if(MSVC)
        # Generated routines are large; /bigobj lifts the section limit.
        # /O1 avoids MSVC's superlinear optimizer blow-up on these giant
        # dispatch functions and still runs the 68000 code at full speed.
        set_source_files_properties(${GEN_SOURCES} PROPERTIES COMPILE_OPTIONS "/bigobj;/O1")
    endif()
endif()
if(MSVC)
    target_compile_definitions(mslug3 PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(mslug3_recomp PRIVATE _CRT_SECURE_NO_WARNINGS)
    target_compile_definitions(mslug3_decrypt PRIVATE _CRT_SECURE_NO_WARNINGS)
endif()

# Ship the SDL2 DLL (when the window is built) next to the executable.
if(WIN32 AND TARGET SDL2::SDL2)
    add_custom_command(TARGET mslug3 POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different $<TARGET_RUNTIME_DLLS:mslug3> $<TARGET_FILE_DIR:mslug3>
        COMMAND_EXPAND_LISTS)
endif()
