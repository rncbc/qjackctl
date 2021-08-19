# Distributed under the OSI-approved BSD 3-Clause License.  See accompanying
# file Copyright.txt or https://cmake.org/licensing for details.

#[=======================================================================[.rst:
FindPortAudio
-----------

Find the PortAudio libraries

Set PortAudio_ROOT cmake or environment variable to the PortAudio install root directory
to use a specific PortAudio installation.

IMPORTED targets
^^^^^^^^^^^^^^^^

This module defines the following :prop_tgt:`IMPORTED` target:

``PortAudio::PortAudio``

Result variables
^^^^^^^^^^^^^^^^

This module will set the following variables if found:

``PortAudio_INCLUDE_DIRS``
  where to find portaudio.h, etc.
``PortAudio_LIBRARIES``
  the libraries to link against to use PortAudio.
``PortAudio_FOUND``
  TRUE if found

#]=======================================================================]

include(FindPackageHandleStandardArgs)

# Use hints from pkg-config if available
find_package (PkgConfig QUIET)
if(PKG_CONFIG_FOUND)
	pkg_check_modules(PC_PortAudio portaudio-2.0)
endif()

find_package(Threads QUIET)

# Look for the necessary header
find_path(PortAudio_INCLUDE_DIR
	NAMES portaudio.h
	HINTS
		${PC_PortAudio_INCLUDE_DIRS}
)
mark_as_advanced(PortAudio_INCLUDE_DIR)

# Look for the necessary library
set(CMAKE_FIND_LIBRARY_PREFIXES "lib" "")
find_library(PortAudio_LIBRARY
	NAMES ${PC_PortAudio_LIBRARIES} portaudio
	NAMES_PER_DIR
	HINTS
		${PC_PortAudio_LIBRARY_DIRS}
)
mark_as_advanced(PortAudio_LIBRARY)
	
set(PortAudio_INCLUDE_DIRS ${PortAudio_INCLUDE_DIR} CACHE PATH "PortAudio include directories")

if(PC_PortAudio_FOUND)
	# Use pkg-config data as it might have additional transitive dependencies of portaudio
	# itself in case of using the static lib.
	set(PortAudio_LIBRARIES ${PC_PortAudio_LIBRARIES} CACHE FILEPATH "PortAudio libraries" )

	find_package_handle_standard_args(PortAudio
		REQUIRED_VARS PortAudio_LIBRARIES PortAudio_INCLUDE_DIRS
		VERSION_VAR PortAudio_VERSION
	)
else()
	set(PortAudio_LIBRARIES ${PortAudio_LIBRARY} CACHE FILEPATH "PortAudio libraries" )
	find_package_handle_standard_args(PortAudio
		REQUIRED_VARS PortAudio_LIBRARIES PortAudio_INCLUDE_DIRS)
endif()

if(PortAudio_FOUND)
	# Create the imported target
	if(NOT TARGET PortAudio::PortAudio)
		add_library(PortAudio::PortAudio UNKNOWN IMPORTED)
		set_target_properties(PortAudio::PortAudio PROPERTIES IMPORTED_LOCATION "${PortAudio_LIBRARY}")
		target_include_directories(PortAudio::PortAudio INTERFACE ${PortAudio_INCLUDE_DIRS})
		target_link_libraries(PortAudio::PortAudio INTERFACE ${PortAudio_LIBRARIES})
		if(WIN32 AND NOT PC_PortAudio_FOUND)
			# Needed when building against the static library
			target_link_libraries(PortAudio::PortAudio INTERFACE dsound setupapi winmm ole32 uuid)
		endif()
		if(Threads_FOUND)
			# Needed when building against the static library
			target_link_libraries(PortAudio::PortAudio INTERFACE Threads::Threads)
		endif()
	endif()
else()
	message(STATUS "Set PortAudio_ROOT cmake or environment variable to the PortAudio install root directory to use a specific PortAudio installation.")
endif()
