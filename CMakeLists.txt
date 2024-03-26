cmake_minimum_required(VERSION 3.21)

project (voyaplayer)

set(EXT_LIB_DIR "${EXT_LIB_DIR}")
set(INC_DIR     "${EXT_LIB_DIR}/include")
set(LIB_DIR     "${EXT_LIB_DIR}/lib")
set(SRC_DIR     "${CMAKE_SOURCE_DIR}")
set(DIST_DIR    "${CMAKE_BINARY_DIR}/dist")

set(CMAKE_CXX_STANDARD          "20")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# libvoyaplayer depends on the following third-party libraries:
# - sdl2, sdl2_ttf, ffmpeg, libaom, zlib

if (IOS)
    set(EXT_LIB_AOM  libaom.a)
    set(EXT_LIB_AVC  libavcodec.a)
    set(EXT_LIB_AVD  libavdevice.a)
    set(EXT_LIB_AVFI libavfilter.a)
    set(EXT_LIB_AVFO libavformat.a)
    set(EXT_LIB_AVU  libavutil.a)
    set(EXT_LIB_SWR  libswresample.a)
    set(EXT_LIB_SWS  libswscale.a)
    set(EXT_LIB_SDL  libSDL2.a)
    set(EXT_LIB_SDLT libSDL2_ttf.a)
    set(EXT_LIB_ZLIB libz.a)
elseif (APPLE)
    set(EXT_LIB_AOM  libaom.dylib)
    set(EXT_LIB_AVC  libavcodec.60.dylib)
    set(EXT_LIB_AVD  libavdevice.60.dylib)
    set(EXT_LIB_AVFI libavfilter.9.dylib)
    set(EXT_LIB_AVFO libavformat.60.dylib)
    set(EXT_LIB_AVU  libavutil.58.dylib)
    set(EXT_LIB_SWR  libswresample.4.dylib)
    set(EXT_LIB_SWS  libswscale.7.dylib)
    set(EXT_LIB_SDL  libSDL2.dylib)
    set(EXT_LIB_SDLT libSDL2_ttf.dylib)
    set(EXT_LIB_ZLIB libz.dylib)
elseif (WIN32)
    set(EXT_LIB_AOM  aom.dll)
    set(EXT_LIB_AVC  avcodec-60.dll)
    set(EXT_LIB_AVD  avdevice-60.dll)
    set(EXT_LIB_AVFI avfilter-9.dll)
    set(EXT_LIB_AVFO avformat-60.dll)
    set(EXT_LIB_AVU  avutil-58.dll)
    set(EXT_LIB_SWR  swresample-4.dll)
    set(EXT_LIB_SWS  swscale-7.dll)
    set(EXT_LIB_SDL  SDL2.dll)
    set(EXT_LIB_SDLT SDL2_ttf.dll)
    set(EXT_LIB_ZLIB zlib.dll)
else()
    set(EXT_LIB_AOM  libaom.so)
    set(EXT_LIB_AVC  libavcodec.so)
    set(EXT_LIB_AVD  libavdevice.so)
    set(EXT_LIB_AVFI libavfilter.so)
    set(EXT_LIB_AVFO libavformat.so)
    set(EXT_LIB_AVU  libavutil.so)
    set(EXT_LIB_SWR  libswresample.so)
    set(EXT_LIB_SWS  libswscale.so)
    set(EXT_LIB_SDL  libSDL2.so)
    set(EXT_LIB_SDLT libSDL2_ttf.so)
    set(EXT_LIB_ZLIB libz.so)
endif()

find_file(LIB_AOM  ${EXT_LIB_AOM}  PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_AVC  ${EXT_LIB_AVC}  PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_AVD  ${EXT_LIB_AVD}  PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_AVFI ${EXT_LIB_AVFI} PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_AVFO ${EXT_LIB_AVFO} PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_AVU  ${EXT_LIB_AVU}  PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_SWR  ${EXT_LIB_SWR}  PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_SWS  ${EXT_LIB_SWS}  PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_SDL  ${EXT_LIB_SDL}  PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_SDLT ${EXT_LIB_SDLT} PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)
find_file(LIB_ZLIB ${EXT_LIB_ZLIB} PATHS ${LIB_DIR} NO_CACHE REQUIRED CMAKE_FIND_ROOT_PATH_BOTH)

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

    set(OS_TEST_LIBS "-landroid")
elseif (IOS)
    set(OS "_ios")

    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_FLAGS      "${CMAKE_CXX_FLAGS} -x objective-c++")

    set(CFG_DIR    "$<CONFIG>-${IOS_SDK}/")
    set(DLL_EXT    "a")
    set(FONT_ARIAL "/System/Library/Fonts/Supplemental/Arial Unicode.ttf")
    set(OUT_DIR    "${CFG_DIR}testvoyaplayer.app")

    set(INFO_PLIST "${SRC_DIR}/ios/MacOSXBundleInfo.plist.in")
    set(RC         "${SRC_DIR}/ios/LaunchScreen.storyboard")

    set(OS_TEST_LIBS "-framework AudioToolbox -framework AVFAudio -framework CoreBluetooth -framework CoreFoundation -framework CoreGraphics -framework CoreHaptics -framework CoreMotion -framework Foundation -framework GameController -framework ImageIO -framework Metal -framework MobileCoreServices -framework OpenGLES -framework QuartzCore -framework UIKit -laom -framework CoreMedia -framework CoreVideo -framework Security -framework VideoToolbox")
elseif (APPLE)
    set(OS "_macosx")

    set(CMAKE_CXX_EXTENSIONS OFF)
    set(CMAKE_CXX_FLAGS      "${CMAKE_CXX_FLAGS} -x objective-c++")

    set(CFG_DIR "$<CONFIG>/")
    set(DLL_EXT "dylib")
    set(OUT_DIR "$<CONFIG>")
elseif (WIN32)
    set(OS "_windows")
    
    set(CFG_DIR "$(Configuration)/")
    set(DLL_EXT "dll")
    set(OUT_DIR "$(Configuration)")
    set(RC      "${SRC_DIR}/windows/libvoyaplayer.rc")

    set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT testvoyaplayer)
else()
    set(OS "_linux")

    set(DLL_EXT "so")
    set(OUT_DIR ".")
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
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change @rpath/SDL2_ttf.dylib @rpath/${EXT_LIB_SDLT} ${CFG_DIR}libvoyaplayer.dylib)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change ${LIB_AVC}            @rpath/${EXT_LIB_AVC}  ${CFG_DIR}libvoyaplayer.dylib)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change ${LIB_AVD}            @rpath/${EXT_LIB_AVD}  ${CFG_DIR}libvoyaplayer.dylib)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change ${LIB_AVFI}           @rpath/${EXT_LIB_AVFI} ${CFG_DIR}libvoyaplayer.dylib)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change ${LIB_AVFO}           @rpath/${EXT_LIB_AVFO} ${CFG_DIR}libvoyaplayer.dylib)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change ${LIB_AVU}            @rpath/${EXT_LIB_AVU}  ${CFG_DIR}libvoyaplayer.dylib)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change ${LIB_SWR}            @rpath/${EXT_LIB_SWR}  ${CFG_DIR}libvoyaplayer.dylib)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change ${LIB_SWS}            @rpath/${EXT_LIB_SWS}  ${CFG_DIR}libvoyaplayer.dylib)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND install_name_tool -change @rpath/libz.1.dylib   @rpath/${EXT_LIB_ZLIB} ${CFG_DIR}libvoyaplayer.dylib)
endif()

if (WIN32)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND if exist \"${DIST_DIR}\" rmdir /s /q \"${DIST_DIR}\")

    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND if not exist \"${DIST_DIR}/bin\" mkdir     \"${DIST_DIR}/bin\")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND if not exist \"${DIST_DIR}/include\" mkdir \"${DIST_DIR}/include\")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND if not exist \"${DIST_DIR}/lib\" mkdir     \"${DIST_DIR}/lib\")

    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}voyaplayer.lib ${DIST_DIR}/lib/)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}voyaplayer.dll ${DIST_DIR}/lib/)
else()
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND rm -rf \"${DIST_DIR}\")

    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND mkdir -p \"${DIST_DIR}/bin\")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND mkdir -p \"${DIST_DIR}/include\")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND mkdir -p \"${DIST_DIR}/lib\")

    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}${LIB_PREP}voyaplayer.${DLL_EXT} ${DIST_DIR}/lib/)
endif()

add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/include/*.h ${DIST_DIR}/include/)

add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/LICENSE ${DIST_DIR}/LICENSE.txt)

if (EXISTS "${EXT_LIB_DIR}/LICENSE-ffmpeg.txt")
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-ffmpeg.txt   ${DIST_DIR}/)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-libaom.txt   ${DIST_DIR}/)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-sdl2.txt     ${DIST_DIR}/)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-sdl2_ttf.txt ${DIST_DIR}/)
    add_custom_command(TARGET voyaplayer POST_BUILD COMMAND cp -f ${EXT_LIB_DIR}/LICENSE-zlib.txt     ${DIST_DIR}/)
endif()

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

if (OS STREQUAL "_macosx")
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND install_name_tool -change @rpath/SDL2_ttf.dylib @rpath/libSDL2_ttf.dylib ${CFG_DIR}testvoyaplayer)
endif()

if (IOS)
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
elseif (WIN32)
    set_property(TARGET testvoyaplayer PROPERTY LINK_FLAGS_DEBUG   "-SUBSYSTEM:CONSOLE,5.02")
    set_property(TARGET testvoyaplayer PROPERTY LINK_FLAGS_RELEASE "-SUBSYSTEM:WINDOWS,5.02")
endif()

if (ANDROID)
    set_property(TARGET testvoyaplayer PROPERTY ADDITIONAL_CLEAN_FILES "android;${DIST_DIR}")
else()
    set_property(TARGET testvoyaplayer PROPERTY ADDITIONAL_CLEAN_FILES "${DIST_DIR}")
endif()

if (ANDROID)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -rf ${SRC_DIR}/android .)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND mkdir -p \"${OUT_DIR}/src/main/assets\")

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/video/* ${OUT_DIR}/src/main/assets/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND mkdir -p \"${OUT_DIR}/libs/${ANDROID_ABI}\")

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f libtestvoyaplayer.so ${OUT_DIR}/libs/${ANDROID_ABI}/libmain.so)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f libvoyaplayer.so     ${OUT_DIR}/libs/${ANDROID_ABI}/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AOM}  ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVC}  ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVD}  ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVFI} ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVFO} ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVU}  ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SWR}  ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SWS}  ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SDL}  ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SDLT} ${OUT_DIR}/libs/${ANDROID_ABI}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_ZLIB} ${OUT_DIR}/libs/${ANDROID_ABI}/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND ${CMAKE_BINARY_DIR}/android/gradlew --stop)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND ${CMAKE_BINARY_DIR}/android/gradlew assembleDebug WORKING_DIRECTORY ${CMAKE_BINARY_DIR}/android)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND mv -f ${OUT_DIR}/build/outputs/apk/debug/testvoyaplayer-debug.apk ${DIST_DIR}/bin/testvoyaplayer-${ANDROID_ABI}-debug.apk)
elseif (IOS)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/img/icon.png ${OUT_DIR}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/video/*      ${OUT_DIR}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${FONT_ARIAL}           ${OUT_DIR}/)

    add_custom_target(testvoyaplayer_app ALL mv -f ${OUT_DIR} ${DIST_DIR}/bin/testvoyaplayer-${CMAKE_OSX_ARCHITECTURES}.app DEPENDS testvoyaplayer)
elseif (WIN32)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/img/icon.ppm ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/img/icon.ppm ${OUT_DIR}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/video/*      ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/video/*      ${OUT_DIR}/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}testvoyaplayer.exe ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}voyaplayer.dll     ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AOM}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVC}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVD}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVFI} ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVFO} ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVU}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SWR}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SWS}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SDL}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SDLT} ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_ZLIB} ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${DIST_DIR}/bin/*.dll ${OUT_DIR}/)
else()
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/img/icon.ppm ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/img/icon.ppm ${OUT_DIR}/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/video/*      ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${SRC_DIR}/video/*      ${OUT_DIR}/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}testvoyaplayer           ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${CFG_DIR}libvoyaplayer.${DLL_EXT} ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AOM}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVC}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVD}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVFI} ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVFO} ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_AVU}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SWR}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SWS}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SDL}  ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_SDLT} ${DIST_DIR}/bin/)
    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${LIB_ZLIB} ${DIST_DIR}/bin/)

    add_custom_command(TARGET testvoyaplayer POST_BUILD COMMAND cp -f ${DIST_DIR}/bin/*.${DLL_EXT} ${OUT_DIR}/)
endif()

#install

install(DIRECTORY ${DIST_DIR}/bin/     DESTINATION ${CMAKE_INSTALL_PREFIX}/bin)
install(DIRECTORY ${DIST_DIR}/lib/     DESTINATION ${CMAKE_INSTALL_PREFIX}/lib)
install(DIRECTORY ${DIST_DIR}/include/ DESTINATION ${CMAKE_INSTALL_PREFIX}/include/libvoyaplayer)
install(DIRECTORY ${DIST_DIR}/         DESTINATION ${CMAKE_INSTALL_PREFIX} FILES_MATCHING PATTERN "*.txt")

if (NOT ANDROID AND NOT WIN32)
    install(DIRECTORY ${DIST_DIR}/bin/ DESTINATION ${CMAKE_INSTALL_PREFIX}/bin PATTERN testvoyaplayer PERMISSIONS OWNER_READ OWNER_WRITE OWNER_EXECUTE GROUP_READ GROUP_EXECUTE WORLD_READ WORLD_EXECUTE)
endif()
