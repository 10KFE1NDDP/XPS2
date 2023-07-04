# Try to find Wayland on a Unix system
#
# This will define:
#
#   Wayland_FOUND       - True if Wayland is found
#
#   The following imported targets: 
#   Wayland::Client     - Imported Client
#   Wayland::Server     - Imported Server
#   Wayland::Egl        - Imported Egl
#   Wayland::Cursor     - Imported Cursor
#
# Copyright (c) 2013 Martin Gräßlin <mgraesslin@kde.org>
#
# Redistribution and use is allowed according to the terms of the BSD license.
# For details see the accompanying COPYING-CMAKE-SCRIPTS file.

IF (NOT WIN32)
	IF (WAYLAND_INCLUDE_DIR AND WAYLAND_LIBRARIES)
		# In the cache already
		SET(WAYLAND_FIND_QUIETLY TRUE)
	ENDIF ()

	FIND_PATH(WAYLAND_CLIENT_INCLUDE_DIR  NAMES wayland-client.h PATHS /usr/include/wayland)
	FIND_PATH(WAYLAND_SERVER_INCLUDE_DIR  NAMES wayland-server.h PATHS /usr/include/wayland)
	FIND_PATH(WAYLAND_EGL_INCLUDE_DIR     NAMES wayland-egl.h PATHS /usr/include/wayland)
	FIND_PATH(WAYLAND_CURSOR_INCLUDE_DIR  NAMES wayland-cursor.h PATHS /usr/include/wayland)

	FIND_LIBRARY(WAYLAND_CLIENT_LIBRARIES NAMES wayland-client PATHS /usr/include/wayland)
	FIND_LIBRARY(WAYLAND_SERVER_LIBRARIES NAMES wayland-server PATHS /usr/include/wayland)
	FIND_LIBRARY(WAYLAND_EGL_LIBRARIES    NAMES wayland-egl PATHS /usr/include/wayland)
	FIND_LIBRARY(WAYLAND_CURSOR_LIBRARIES NAMES wayland-cursor PATHS /usr/include/wayland)

	include(FindPackageHandleStandardArgs)

	# FIND_PACKAGE_HANDLE_STANDARD_ARGS is just meant to find the main package and set package found. Not set variables or find individual libs
	FIND_PACKAGE_HANDLE_STANDARD_ARGS(Wayland REQUIRED_VARS
		WAYLAND_CLIENT_LIBRARIES WAYLAND_CLIENT_INCLUDE_DIR
		WAYLAND_SERVER_LIBRARIES WAYLAND_SERVER_INCLUDE_DIR
		WAYLAND_EGL_LIBRARIES    WAYLAND_EGL_INCLUDE_DIR
		WAYLAND_CURSOR_LIBRARIES WAYLAND_CURSOR_INCLUDE_DIR
	)

	if (WAYLAND_CLIENT_INCLUDE_DIR AND WAYLAND_CLIENT_LIBRARIES AND NOT TARGET Wayland::Client)
		add_library(Wayland::Client UNKNOWN IMPORTED)
		set_target_properties(Wayland::Client PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_CLIENT_INCLUDE_DIR}" IMPORTED_LOCATION "${WAYLAND_CLIENT_LIBRARIES}")
	endif()
	if (WAYLAND_SERVER_INCLUDE_DIR AND WAYLAND_SERVER_LIBRARIES AND NOT TARGET Wayland::Server)
		add_library(Wayland::Server UNKNOWN IMPORTED)
		set_target_properties(Wayland::Server PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_SERVER_INCLUDE_DIR}" IMPORTED_LOCATION "${WAYLAND_SERVER_LIBRARIES}")
	endif()
	if (WAYLAND_EGL_INCLUDE_DIR AND WAYLAND_EGL_LIBRARIES AND NOT TARGET Wayland::Egl)
		add_library(Wayland::Egl UNKNOWN IMPORTED)
		set_target_properties(Wayland::Egl PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_EGL_INCLUDE_DIR}" IMPORTED_LOCATION "${WAYLAND_EGL_LIBRARIES}")
	endif()
	if (WAYLAND_CURSOR_INCLUDE_DIR AND WAYLAND_CURSOR_LIBRARIES AND NOT TARGET Wayland::Cursor)
		add_library(Wayland::Cursor UNKNOWN IMPORTED)
		set_target_properties(Wayland::Cursor PROPERTIES INTERFACE_INCLUDE_DIRECTORIES "${WAYLAND_CURSOR_INCLUDE_DIR}" IMPORTED_LOCATION "${WAYLAND_CURSOR_LIBRARIES}")
	endif()

	MARK_AS_ADVANCED(
		WAYLAND_CLIENT_INCLUDE_DIR  WAYLAND_CLIENT_LIBRARIES
		WAYLAND_SERVER_INCLUDE_DIR  WAYLAND_SERVER_LIBRARIES
		WAYLAND_EGL_INCLUDE_DIR     WAYLAND_EGL_LIBRARIES
		WAYLAND_CURSOR_INCLUDE_DIR  WAYLAND_CURSOR_LIBRARIES
	)

ENDIF ()
