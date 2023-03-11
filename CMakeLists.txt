cmake_minimum_required(VERSION 3.12)

project (testvoyaplayer)
project (libvoyaplayer)

# Voya Player library depends on the following third-party libraries:
# - sdl2, sdl2_ttf, ffmpeg, libaom, zlib

set(EXT_LIB_DIR "")
set(INC_DIR     "${EXT_LIB_DIR}/include")
set(LIB_DIR     "${EXT_LIB_DIR}/lib")
set(CPP_VERSION "20")

if (APPLE)
    set(OS "_macosx")
elseif (UNIX)
    set(OS "_linux")
elseif (WIN32)
    set(OS         "_windows")
    set(DIRENT_DIR "")
    set(RC         "windows/libvoyaplayer.rc")

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT testvoyaplayer)
endif()

if (WIN32)
    set(LIBS SDL2.lib SDL2_ttf.lib avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib swresample.lib swscale.lib zlib.lib)
else()
    set(LIBS -lSDL2 -lSDL2_ttf -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lz)
endif()

file(GLOB INC_H       "inc/*.h")
file(GLOB SOURCES_H   "src/*.h")
file(GLOB SOURCES_CPP "src/*.cpp")
file(GLOB TEST_H      "test/*.h")
file(GLOB TEST_CPP    "test/*.cpp")

add_compile_definitions(${OS} "MAKEDLL")

include_directories("inc" ${INC_DIR} ${DIRENT_DIR})
link_directories(${LIB_DIR})

add_library(libvoyaplayer SHARED ${SOURCES_CPP} ${SOURCES_H} ${INC_H} ${RC})
target_link_libraries(libvoyaplayer ${LIBS})
set_property(TARGET libvoyaplayer PROPERTY CXX_STANDARD ${CPP_VERSION})

add_executable(testvoyaplayer ${TEST_CPP} ${TEST_H} ${INC_H})
target_link_libraries(testvoyaplayer libvoyaplayer SDL2.lib)
set_property(TARGET testvoyaplayer PROPERTY CXX_STANDARD ${CPP_VERSION})

set(BUILD_DIR "${CMAKE_BINARY_DIR}")
set(SRC_DIR   "${CMAKE_SOURCE_DIR}")
set(DIST_DIR  "${BUILD_DIR}/dist")

if (APPLE)
    set(DLL_EXT "dylib")
    set(LIB_EXT "a")
    set(OUT_DIR "${BUILD_DIR}/out")
elseif (UNIX)
    set(DLL_EXT "so")
    set(LIB_EXT "a")
    set(OUT_DIR "${BUILD_DIR}/out")
elseif (WIN32)
    set(DLL_EXT "dll")
    set(LIB_EXT "lib")
    set(OUT_DIR "${BUILD_DIR}/$(Configuration)")
endif()

add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND if not exist \"${DIST_DIR}/bin\" mkdir \"${DIST_DIR}/bin\")
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND if not exist \"${DIST_DIR}/inc\" mkdir \"${DIST_DIR}/inc\")
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND if not exist \"${DIST_DIR}/lib\" mkdir \"${DIST_DIR}/lib\")
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND if not exist \"${OUT_DIR}\"      mkdir \"${OUT_DIR}\")

add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avcodec*.${DLL_EXT}    ${OUT_DIR})
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avdevice*.${DLL_EXT}   ${OUT_DIR})
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avfilter*.${DLL_EXT}   ${OUT_DIR})
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avformat*.${DLL_EXT}   ${OUT_DIR})
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avutil*.${DLL_EXT}     ${OUT_DIR})
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/SDL2.${DLL_EXT}        ${OUT_DIR})
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/SDL2_ttf.${DLL_EXT}    ${OUT_DIR})
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/swresample*.${DLL_EXT} ${OUT_DIR})
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/swscale*.${DLL_EXT}    ${OUT_DIR})
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/zlib.${DLL_EXT}        ${OUT_DIR})

add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${OUT_DIR}/*.${DLL_EXT}              ${DIST_DIR}/bin/)
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/inc/*.h                   ${DIST_DIR}/inc/)
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${OUT_DIR}/libvoyaplayer*.${LIB_EXT} ${DIST_DIR}/lib/)
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/LICENSE                   ${DIST_DIR}/LICENSE.txt)
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-ffmpeg.txt    ${DIST_DIR}/)
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-libaom.txt    ${DIST_DIR}/)
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-sdl2.txt      ${DIST_DIR}/)
add_custom_command(TARGET libvoyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-sdl2_ttf.txt  ${DIST_DIR}/)

if (WIN32)
    set_target_properties(testvoyaplayer PROPERTIES LINK_FLAGS_DEBUG   "-SUBSYSTEM:CONSOLE,5.02")
    set_target_properties(testvoyaplayer PROPERTIES LINK_FLAGS_RELEASE "-SUBSYSTEM:WINDOWS,5.02")

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ../img/*.bmp ${OUT_DIR})
endif()
