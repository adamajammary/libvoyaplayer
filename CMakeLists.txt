cmake_minimum_required(VERSION 3.12)

project (voyaplayer)

# libvoyaplayer depends on the following third-party libraries:
# - sdl2, sdl2_ttf, ffmpeg, libaom, zlib

set(EXT_LIB_DIR "${EXT_LIB_DIR}")
set(INC_DIR     "${EXT_LIB_DIR}/include")
set(LIB_DIR     "${EXT_LIB_DIR}/lib")
set(SRC_DIR     "${CMAKE_SOURCE_DIR}")
set(DIST_DIR    "${CMAKE_BINARY_DIR}/dist")

set(CMAKE_CXX_STANDARD          "20")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(INCS      "include" ${INC_DIR} ${DIRENT_DIR})
set(TEST_INCS "include" ${INC_DIR})

if (WIN32)
    set(LIBS      SDL2.lib SDL2_ttf.lib avcodec.lib avdevice.lib avfilter.lib avformat.lib avutil.lib swresample.lib swscale.lib zlib.lib)
    set(TEST_LIBS SDL2.lib SDL2_ttf.lib)
else()
    set(LIB_PREP "lib")

    set(LIBS      -lSDL2 -lSDL2_ttf -lavcodec -lavdevice -lavfilter -lavformat -lavutil -lswresample -lswscale -lz)
    set(TEST_LIBS -lSDL2 -lSDL2_ttf)
endif()

if (ANDROID)
    set(OS "_android")

    set(DLL_EXT "so")
    set(OUT_DIR "android/testvoyaplayer")

    set(OS_LIBS "-landroid")
elseif (IOS)
    set(OS "_ios")

    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_FLAGS      "${CMAKE_CXX_FLAGS} -x objective-c++")

    set(CFG_DIR "$<CONFIG>-iphoneos/")
    set(DLL_EXT "a")
    set(OUT_DIR "${CFG_DIR}testvoyaplayer.app")

    set(INFO_PLIST "${SRC_DIR}/ios/MacOSXBundleInfo.plist.in")
    set(RC         "${SRC_DIR}/ios/LaunchScreen.storyboard")

    set(OS_TEST_LIBS "-liconv -framework AudioToolbox -framework AVFAudio -framework CoreBluetooth -framework CoreFoundation -framework CoreGraphics -framework CoreHaptics -framework CoreMotion -framework Foundation -framework GameController -framework ImageIO -framework Metal -framework MobileCoreServices -framework OpenGLES -framework QuartzCore -framework UIKit -laom -framework CoreMedia -framework CoreVideo -framework Security -framework VideoToolbox")
elseif (APPLE)
    set(OS "_macosx")

    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_FLAGS      "${CMAKE_CXX_FLAGS} -x objective-c++")

    set(CFG_DIR "$<CONFIG>/")
    set(DLL_EXT "dylib")
    set(OUT_DIR "$<CONFIG>")

    set(OS_LIBS "-framework AppKit")
elseif (LINUX)
    set(OS "_linux")

    set(DLL_EXT "so")
    set(OUT_DIR ".")

    set(OS_INCS "/usr/include/gtk-3.0" "/usr/include/pango-1.0" "/usr/include/glib-2.0" "/usr/lib/x86_64-linux-gnu/glib-2.0/include" "/usr/include/harfbuzz" "/usr/include/freetype2" "/usr/include/libpng16" "/usr/include/libmount" "/usr/include/blkid" "/usr/include/fribidi" "/usr/include/cairo" "/usr/include/pixman-1" "/usr/include/gdk-pixbuf-2.0" "/usr/include/x86_64-linux-gnu" "/usr/include/gio-unix-2.0" "/usr/include/atk-1.0" "/usr/include/at-spi2-atk/2.0" "/usr/include/at-spi-2.0" "/usr/include/dbus-1.0" "/usr/lib/x86_64-linux-gnu/dbus-1.0/include")
    set(OS_LIBS "-lgtk-3 -lgdk-3 -lz -lpangocairo-1.0 -lpango-1.0 -lharfbuzz -latk-1.0 -lcairo-gobject -lcairo -lgdk_pixbuf-2.0 -lgio-2.0 -lgobject-2.0 -lglib-2.0")
elseif (WIN32)
    set(OS "_windows")
    
    set(CFG_DIR "$(Configuration)/")
    set(DLL_EXT "dll")
    set(OUT_DIR "$(Configuration)")
    set(RC      "${SRC_DIR}/windows/libvoyaplayer.rc")

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT testvoyaplayer)
endif()

file(GLOB INC_H "${SRC_DIR}/include/*.h")

source_group("Resources" FILES ${RC})

# libvoyaplayer

file(GLOB SOURCES_H   "${SRC_DIR}/src/*.h")
file(GLOB SOURCES_CPP "${SRC_DIR}/src/*.cpp")

if (IOS)
    add_library(voyaplayer STATIC ${SOURCES_CPP} ${SOURCES_H} ${INC_H})
else()
    add_library(voyaplayer SHARED ${SOURCES_CPP} ${SOURCES_H} ${INC_H} ${RC})
endif()

target_compile_definitions(voyaplayer PRIVATE ${OS} _CRT_SECURE_NO_WARNINGS NOMINMAX MAKEDLL)
target_include_directories(voyaplayer PRIVATE ${INCS} ${OS_INCS})
target_link_directories(voyaplayer PRIVATE ${LIB_DIR})
target_link_libraries(voyaplayer PRIVATE ${LIBS} ${OS_LIBS})

if (OS STREQUAL "_macosx")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change @rpath/SDL2_ttf.dylib @rpath/libSDL2_ttf.dylib ${CFG_DIR}libvoyaplayer.dylib)
endif()

if (WIN32)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND if not exist \"${DIST_DIR}/bin\" mkdir     \"${DIST_DIR}/bin\")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND if not exist \"${DIST_DIR}/include\" mkdir \"${DIST_DIR}/include\")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND if not exist \"${DIST_DIR}/lib\" mkdir     \"${DIST_DIR}/lib\")

    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}voyaplayer.lib ${DIST_DIR}/lib/)
else()
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND mkdir -p \"${DIST_DIR}/bin\")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND mkdir -p \"${DIST_DIR}/include\")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND mkdir -p \"${DIST_DIR}/lib\")

    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}${LIB_PREP}voyaplayer.${DLL_EXT} ${DIST_DIR}/lib/)
endif()

add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/include/*.h ${DIST_DIR}/include/)

add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/LICENSE                  ${DIST_DIR}/LICENSE.txt)
add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-ffmpeg.txt   ${DIST_DIR}/)
add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-libaom.txt   ${DIST_DIR}/)
add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-sdl2.txt     ${DIST_DIR}/)
add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-sdl2_ttf.txt ${DIST_DIR}/)
add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-zlib.txt     ${DIST_DIR}/)

# testvoyaplayer

file(GLOB TEST_H   "${SRC_DIR}/test/*.h")
file(GLOB TEST_CPP "${SRC_DIR}/test/*.cpp")

if (ANDROID)
    add_library(testvoyaplayer SHARED ${TEST_CPP} ${TEST_H} ${INC_H})
elseif (IOS)
    add_executable(testvoyaplayer MACOSX_BUNDLE ${TEST_CPP} ${TEST_H} ${INC_H} ${RC})
else()
    add_executable(testvoyaplayer ${TEST_CPP} ${TEST_H} ${INC_H})
endif()

target_compile_definitions(testvoyaplayer PRIVATE ${OS} NOMINMAX)
target_include_directories(testvoyaplayer PRIVATE ${TEST_INCS} ${OS_TEST_INCS})
target_link_directories(testvoyaplayer PRIVATE ${LIB_DIR})
target_link_libraries(testvoyaplayer PRIVATE voyaplayer ${TEST_LIBS} ${OS_TEST_LIBS})

if (ANDROID)
    set_property(TARGET testvoyaplayer PROPERTY ADDITIONAL_CLEAN_FILES "android;${DIST_DIR}")
elseif (IOS)
    set(CMAKE_XCODE_ATTRIBUTE_PRODUCT_BUNDLE_IDENTIFIER "com.libvoyaplayer.test")
    set(CMAKE_XCODE_ATTRIBUTE_TARGETED_DEVICE_FAMILY    "1,2")

    set_target_properties(testvoyaplayer PROPERTIES
        MACOSX_BUNDLE_INFO_PLIST           "${INFO_PLIST}"
        MACOSX_BUNDLE_BUNDLE_NAME          "Voya Player Library"
        MACOSX_BUNDLE_COPYRIGHT            "(c) 2021 Adam A. Jammary (Jammary Studio)"
        MACOSX_BUNDLE_GUI_IDENTIFIER       "com.libvoyaplayer.test"
        MACOSX_BUNDLE_ICON_FILE            "icon"
        MACOSX_BUNDLE_BUNDLE_VERSION       "1.0.0"
        MACOSX_BUNDLE_LONG_VERSION_STRING  "1.0.0"
        MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0.0"
    )

    set_property(TARGET testvoyaplayer PROPERTY ADDITIONAL_CLEAN_FILES "${DIST_DIR}")
elseif (WIN32)
    set_property(TARGET testvoyaplayer PROPERTY LINK_FLAGS_DEBUG   "-SUBSYSTEM:CONSOLE,5.02")
    set_property(TARGET testvoyaplayer PROPERTY LINK_FLAGS_RELEASE "-SUBSYSTEM:WINDOWS,5.02")

    set_property(TARGET testvoyaplayer PROPERTY ADDITIONAL_CLEAN_FILES "${DIST_DIR}")
endif()

if (ANDROID)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -rf ${SRC_DIR}/android .)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND mkdir -p \"${OUT_DIR}/libs/arm64-v8a\")

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f libtestvoyaplayer.so ${OUT_DIR}/libs/arm64-v8a/libmain.so)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f libvoyaplayer.so     ${OUT_DIR}/libs/arm64-v8a/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libaom.so        ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavcodec.so    ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavdevice.so   ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavfilter.so   ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavformat.so   ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavutil.so     ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libswresample.so ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libswscale.so    ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libSDL2.so       ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libSDL2_ttf.so   ${OUT_DIR}/libs/arm64-v8a/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libz.so          ${OUT_DIR}/libs/arm64-v8a/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND ${CMAKE_BINARY_DIR}/android/gradlew --stop)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND ${CMAKE_BINARY_DIR}/android/gradlew assembleDebug WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/android)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${OUT_DIR}/build/outputs/apk/debug/testvoyaplayer-debug.apk ${DIST_DIR}/bin/)
elseif (IOS)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/img/icon* ${OUT_DIR}/)

    add_custom_target(testvoyaplayer_app ALL cp -rf ${OUT_DIR} ${DIST_DIR}/bin/ DEPENDS testvoyaplayer)
elseif (APPLE)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/img/icon* ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}testvoyaplayer      ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}libvoyaplayer.dylib ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libaom.dylib          ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavcodec.60.dylib   ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavdevice.60.dylib  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavfilter.9.dylib   ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavformat.60.dylib  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavutil.58.dylib    ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libswresample.4.dylib ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libswscale.7.dylib    ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libSDL2.dylib         ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libSDL2_ttf.dylib     ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libz.dylib            ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${DIST_DIR}/bin/icon*   ${OUT_DIR}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${DIST_DIR}/bin/*.dylib ${OUT_DIR}/)
elseif (WIN32)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/img/icon* ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}testvoyaplayer.exe ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}voyaplayer.dll     ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/aom.dll          ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avcodec-60.dll   ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avdevice-60.dll  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avfilter-9.dll   ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avformat-60.dll  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/avutil-58.dll    ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/swresample-4.dll ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/swscale-7.dll    ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/SDL2.dll         ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/SDL2_ttf.dll     ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/zlib.dll         ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${DIST_DIR}/bin/icon* ${OUT_DIR}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${DIST_DIR}/bin/*.dll ${OUT_DIR}/)
else()
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/img/icon* ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}testvoyaplayer           ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}libvoyaplayer.${DLL_EXT} ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libaom.${DLL_EXT}        ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavcodec.${DLL_EXT}    ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavdevice.${DLL_EXT}   ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavfilter.${DLL_EXT}   ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavformat.${DLL_EXT}   ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libavutil.${DLL_EXT}     ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libswresample.${DLL_EXT} ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libswscale.${DLL_EXT}    ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libSDL2.${DLL_EXT}       ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libSDL2_ttf.${DLL_EXT}   ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_DIR}/libz.${DLL_EXT}          ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${DIST_DIR}/bin/icon*        ${OUT_DIR}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${DIST_DIR}/bin/*.${DLL_EXT} ${OUT_DIR}/)
endif()

#install

install(DIRECTORY ${DIST_DIR}/bin/     DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY ${DIST_DIR}/lib/     DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(DIRECTORY ${DIST_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/libvoyaplayer)
install(DIRECTORY ${DIST_DIR}/         DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.txt")

if (LINUX OR IOS OR APPLE)
    install(DIRECTORY ${DIST_DIR}/bin/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin PATTERN testvoyaplayer PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()
