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

add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/msdf-atlas-gen")
add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/nativefiledialog-extended")
add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/o1heap")
add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/plume")
add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/SDL")
add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/SDL_mixer")

if (APPLE)
    add_subdirectory("${MARATHON_RECOMP_THIRDPARTY_ROOT}/MoltenVK")
endif()

if (NOT WIN32)
    set(FFMPEG_PATH ${CMAKE_CURRENT_SOURCE_DIR}/FFmpeg)

    add_custom_command(
            OUTPUT ${FFMPEG_PATH}/config.h
            COMMAND ./configure --disable-all --disable-x86asm --disable-autodetect --disable-network --enable-avcodec --enable-decoder='xmaframes'
            COMMENT "Configuring FFmpeg..."
            WORKING_DIRECTORY ${FFMPEG_PATH}
    )

    add_custom_target(ffmpeg-configure DEPENDS ${FFMPEG_PATH}/config.h)

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
    import_ffmpeg_library(avutil)
endif()
