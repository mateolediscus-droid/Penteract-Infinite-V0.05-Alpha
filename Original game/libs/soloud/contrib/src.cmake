set (TARGET_NAME SoLoud)

set (HEADER_PATH ../include)
set (SOURCE_PATH ../src)

set (LINK_LIBRARIES)

# Headers
set (TARGET_HEADERS
	${HEADER_PATH}/soloud.h
	${HEADER_PATH}/soloud_audiosource.h
	${HEADER_PATH}/soloud_ay.h
	${HEADER_PATH}/soloud_bassboostfilter.h
	${HEADER_PATH}/soloud_biquadresonantfilter.h
	${HEADER_PATH}/soloud_bus.h
	${HEADER_PATH}/soloud_c.h
	${HEADER_PATH}/soloud_dcremovalfilter.h
	${HEADER_PATH}/soloud_echofilter.h
	${HEADER_PATH}/soloud_error.h
	${HEADER_PATH}/soloud_fader.h
	${HEADER_PATH}/soloud_fft.h
	${HEADER_PATH}/soloud_fftfilter.h
	${HEADER_PATH}/soloud_file.h
	${HEADER_PATH}/soloud_file_hack_off.h
	${HEADER_PATH}/soloud_file_hack_on.h
	${HEADER_PATH}/soloud_filter.h
	${HEADER_PATH}/soloud_flangerfilter.h
	${HEADER_PATH}/soloud_freeverbfilter.h
	${HEADER_PATH}/soloud_internal.h
	${HEADER_PATH}/soloud_lofifilter.h
	${HEADER_PATH}/soloud_misc.h
	${HEADER_PATH}/soloud_monotone.h
	${HEADER_PATH}/soloud_noise.h
	${HEADER_PATH}/soloud_openmpt.h
	${HEADER_PATH}/soloud_queue.h
	${HEADER_PATH}/soloud_robotizefilter.h
	${HEADER_PATH}/soloud_sfxr.h
	${HEADER_PATH}/soloud_speech.h
	${HEADER_PATH}/soloud_tedsid.h
	${HEADER_PATH}/soloud_thread.h
	${HEADER_PATH}/soloud_vic.h
	${HEADER_PATH}/soloud_vizsn.h
	${HEADER_PATH}/soloud_wav.h
	${HEADER_PATH}/soloud_waveshaperfilter.h
	${HEADER_PATH}/soloud_wavstream.h
)


# Core
set (CORE_PATH ${SOURCE_PATH}/core)
set (CORE_SOURCES
	${CORE_PATH}/soloud.cpp
	${CORE_PATH}/soloud_audiosource.cpp
	${CORE_PATH}/soloud_bus.cpp
	${CORE_PATH}/soloud_core_3d.cpp
	${CORE_PATH}/soloud_core_basicops.cpp
	${CORE_PATH}/soloud_core_faderops.cpp
	${CORE_PATH}/soloud_core_filterops.cpp
	${CORE_PATH}/soloud_core_getters.cpp
	${CORE_PATH}/soloud_core_setters.cpp
	${CORE_PATH}/soloud_core_voicegroup.cpp
	${CORE_PATH}/soloud_core_voiceops.cpp
	${CORE_PATH}/soloud_fader.cpp
	${CORE_PATH}/soloud_fft.cpp
	${CORE_PATH}/soloud_fft_lut.cpp
	${CORE_PATH}/soloud_file.cpp
	${CORE_PATH}/soloud_filter.cpp
	${CORE_PATH}/soloud_misc.cpp
	${CORE_PATH}/soloud_queue.cpp
	${CORE_PATH}/soloud_thread.cpp
)


# Audiosources
set (AUDIOSOURCES_PATH ${SOURCE_PATH}/audiosource)
set (AUDIOSOURCES_SOURCES
	# ay
	${AUDIOSOURCES_PATH}/ay/chipplayer.cpp
	${AUDIOSOURCES_PATH}/ay/chipplayer.h
	${AUDIOSOURCES_PATH}/ay/sndbuffer.cpp
	${AUDIOSOURCES_PATH}/ay/sndbuffer.h
	${AUDIOSOURCES_PATH}/ay/sndchip.cpp
	${AUDIOSOURCES_PATH}/ay/sndchip.h
	${AUDIOSOURCES_PATH}/ay/sndrender.cpp
	${AUDIOSOURCES_PATH}/ay/sndrender.h
	${AUDIOSOURCES_PATH}/ay/soloud_ay.cpp

	# monotone
	${AUDIOSOURCES_PATH}/monotone/soloud_monotone.cpp

	# noise
	${AUDIOSOURCES_PATH}/noise/soloud_noise.cpp

	# openmpt
	${AUDIOSOURCES_PATH}/openmpt/soloud_openmpt.cpp
	${AUDIOSOURCES_PATH}/openmpt/soloud_openmpt_dll.c

	# sfxr
	${AUDIOSOURCES_PATH}/sfxr/soloud_sfxr.cpp

	# speech
	${AUDIOSOURCES_PATH}/speech/Elements.def
	${AUDIOSOURCES_PATH}/speech/darray.cpp
	${AUDIOSOURCES_PATH}/speech/darray.h
	${AUDIOSOURCES_PATH}/speech/klatt.cpp
	${AUDIOSOURCES_PATH}/speech/klatt.h
	${AUDIOSOURCES_PATH}/speech/resonator.cpp
	${AUDIOSOURCES_PATH}/speech/resonator.h
	${AUDIOSOURCES_PATH}/speech/soloud_speech.cpp
	${AUDIOSOURCES_PATH}/speech/tts.cpp
	${AUDIOSOURCES_PATH}/speech/tts.h

	# tedsid
	${AUDIOSOURCES_PATH}/tedsid/sid.cpp
	${AUDIOSOURCES_PATH}/tedsid/sid.h
	${AUDIOSOURCES_PATH}/tedsid/soloud_tedsid.cpp
	${AUDIOSOURCES_PATH}/tedsid/ted.cpp
	${AUDIOSOURCES_PATH}/tedsid/ted.h

	# vic
	${AUDIOSOURCES_PATH}/vic/soloud_vic.cpp

	# vizsn
	${AUDIOSOURCES_PATH}/vizsn/soloud_vizsn.cpp

	# wav
	${AUDIOSOURCES_PATH}/wav/dr_flac.h
	${AUDIOSOURCES_PATH}/wav/dr_impl.cpp
	${AUDIOSOURCES_PATH}/wav/dr_mp3.h
	${AUDIOSOURCES_PATH}/wav/dr_wav.h
	${AUDIOSOURCES_PATH}/wav/soloud_wav.cpp
	${AUDIOSOURCES_PATH}/wav/soloud_wavstream.cpp
	${AUDIOSOURCES_PATH}/wav/stb_vorbis.c
	${AUDIOSOURCES_PATH}/wav/stb_vorbis.h
)


# Backend (miniaudio)
set (BACKENDS_PATH ${SOURCE_PATH}/backend)
set (BACKENDS_SOURCES)

add_definitions (-DWITH_MINIAUDIO)

set (BACKENDS_SOURCES
	${BACKENDS_SOURCES}
	${BACKENDS_PATH}/miniaudio/miniaudio.h
	${BACKENDS_PATH}/miniaudio/soloud_miniaudio.cpp
)

# Filters
set (FILTERS_PATH ${SOURCE_PATH}/filter)
set (FILTERS_SOURCES
	${FILTERS_PATH}/soloud_bassboostfilter.cpp
	${FILTERS_PATH}/soloud_biquadresonantfilter.cpp
	${FILTERS_PATH}/soloud_dcremovalfilter.cpp
	${FILTERS_PATH}/soloud_duckfilter.cpp
	${FILTERS_PATH}/soloud_echofilter.cpp
	${FILTERS_PATH}/soloud_eqfilter.cpp
	${FILTERS_PATH}/soloud_fftfilter.cpp
	${FILTERS_PATH}/soloud_flangerfilter.cpp
	${FILTERS_PATH}/soloud_freeverbfilter.cpp
	${FILTERS_PATH}/soloud_lofifilter.cpp
	${FILTERS_PATH}/soloud_robotizefilter.cpp
	${FILTERS_PATH}/soloud_waveshaperfilter.cpp
)


# All together
source_group ("Includes"		FILES ${TARGET_HEADERS})
source_group ("Core"			FILES ${CORE_SOURCES})
source_group ("Audiosources"	FILES ${AUDIOSOURCES_SOURCES})
source_group ("Backends"		FILES ${BACKENDS_SOURCES})
source_group ("Filters"			FILES ${FILTERS_SOURCES})

set (TARGET_SOURCES
	${CORE_SOURCES}
	${AUDIOSOURCES_SOURCES}
	${BACKENDS_SOURCES}
	${FILTERS_SOURCES}
)

if (SOLOUD_C_API)
	set (TARGET_SOURCES
		${TARGET_SOURCES}
		${SOURCE_PATH}/c_api/soloud.def
		${SOURCE_PATH}/c_api/soloud_c.cpp
	)
	set (TARGET_HEADERS
		${TARGET_HEADERS}
		${HEADER_PATH}/soloud_c.h
	)
endif()

if (SOLOUD_DYNAMIC)
	add_library(${TARGET_NAME} ${TARGET_SOURCES} ${TARGET_HEADERS})
	include(GenerateExportHeader)
	generate_export_header(${TARGET_NAME})
endif ()

if (SOLOUD_STATIC)
	add_library(${TARGET_NAME} STATIC ${TARGET_SOURCES} ${TARGET_HEADERS})
endif()

target_link_libraries (${TARGET_NAME} ${LINK_LIBRARIES})

target_include_directories (${TARGET_NAME}
	PUBLIC
		$<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/../include>
		$<INSTALL_INTERFACE:include>
)

include (Install)
INSTALL(FILES ${TARGET_HEADERS} DESTINATION include/soloud)
