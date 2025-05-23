cmake_policy(SET CMP0077 NEW)

set(MSDF_ATLAS_BUILD_STANDALONE OFF)
set(MSDF_ATLAS_USE_SKIA OFF)
set(MSDF_ATLAS_NO_ARTERY_FONT ON)
set(MSDFGEN_DISABLE_PNG ON)

set(SDL2MIXER_DEPS_SHARED OFF)
set(SDL2MIXER_VENDORED ON)
set(SDL2MIXER_FLAC OFF)
set(SDL2MIXER_MOD OFF)
set(SDL2MIXER_MP3 OFF)
set(SDL2MIXER_MIDI OFF)
set(SDL2MIXER_OPUS OFF)
set(SDL2MIXER_VORBIS "VORBISFILE")
set(SDL2MIXER_WAVPACK OFF)

add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/msdf-atlas-gen")
add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/nativefiledialog-extended")
add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/o1heap")
add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/SDL")
add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/SDL_mixer")

if (APPLE)
    add_subdirectory("${UNLEASHED_RECOMP_THIRDPARTY_ROOT}/MoltenVK")
endif()

set(FFMPEG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FFmpeg) 

add_custom_command(
    OUTPUT ${FFMPEG_PATH}/.patched
    COMMAND patch -p1 --forward --silent < ../ffmpeg_mathops.patch
    COMMAND touch .patched
    COMMENT "Applying FFmpeg patch..."
    WORKING_DIRECTORY ${FFMPEG_PATH}
)

add_custom_command(
    OUTPUT ${FFMPEG_PATH}/config.h
    COMMAND ./configure --arch=x86_64 --disable-everything --disable-programs --disable-all --disable-x86asm --disable-autodetect --disable-network --enable-avcodec --enable-avformat --enable-avutil --enable-decoder='mp3,mp3float,wmav2,xmaframes' --enable-parser=mpegaudio --enable-demuxer='asf,mp3' --enable-protocol=file
    COMMENT "Configuring FFmpeg..."
    WORKING_DIRECTORY ${FFMPEG_PATH}
)
add_custom_target(ffmpeg-configure DEPENDS ${FFMPEG_PATH}/config.h ${FFMPEG_PATH}/.patched)

function(import_ffmpeg_library name)
    if (${CMAKE_GENERATOR} STREQUAL "Unix Makefiles")
        set(MAKE_COMMAND $(MAKE) -j$(nproc))
    elseif (${CMAKE_GENERATOR} STREQUAL "Ninja")
        set(MAKE_COMMAND make -j$$(nproc))
    else()
        set(MAKE_COMMAND make)
    endif()

    add_custom_command(
        OUTPUT "${FFMPEG_PATH}/lib${name}/lib${name}.a"
        COMMAND ${MAKE_COMMAND} -C ${FFMPEG_PATH} "lib${name}/lib${name}.a"
        COMMENT "Building lib${name}/lib${name}.a"
        DEPENDS ffmpeg-configure
        WORKING_DIRECTORY ${FFMPEG_PATH}
    )

    add_custom_target(ffmpeg-build-${name} DEPENDS "${FFMPEG_PATH}/lib${name}/lib${name}.a")

    add_library(ffmpeg::${name} STATIC IMPORTED GLOBAL)
    set_property(TARGET ffmpeg::${name} PROPERTY IMPORTED_LOCATION "${FFMPEG_PATH}/lib${name}/lib${name}.a")
    set_property(TARGET ffmpeg::${name} PROPERTY INTERFACE_INCLUDE_DIRECTORIES "${FFMPEG_PATH}")
    add_dependencies(ffmpeg::${name} ffmpeg-build-${name})
endfunction()

import_ffmpeg_library(avcodec)
import_ffmpeg_library(avformat)
import_ffmpeg_library(avfilter)
import_ffmpeg_library(avdevice)
import_ffmpeg_library(avutil)
import_ffmpeg_library(swscale)
import_ffmpeg_library(swresample)
import_ffmpeg_library(postproc)
