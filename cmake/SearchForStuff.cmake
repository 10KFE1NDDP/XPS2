#-------------------------------------------------------------------------------
#						Search all libraries on the system
#-------------------------------------------------------------------------------
## Use cmake package to find module
find_package(ALSA)
find_package(BZip2)
find_package(Gettext) # translation tool
find_package(Git)
find_package(JPEG)
find_package(OpenGL)
# The requirement of wxWidgets is checked in SelectPcsx2Plugins module
# Does not require the module (allow to compile non-wx plugins)
# Force the unicode build (the variable is only supported on cmake 2.8.3 and above)
# Warning do not put any double-quote for the argument...
# set(wxWidgets_CONFIG_OPTIONS --unicode=yes --debug=yes) # In case someone want to debug inside wx
#
# Fedora uses an extra non-standard option ... Arch must be the first option.
# They do uname -m if missing so only fix for cross compilations.
# http://pkgs.fedoraproject.org/cgit/wxGTK.git/plain/wx-config
if(Fedora AND CMAKE_CROSSCOMPILING)
    set(wxWidgets_CONFIG_OPTIONS --arch ${PCSX2_TARGET_ARCHITECTURES} --unicode=yes)
else()
    set(wxWidgets_CONFIG_OPTIONS --unicode=yes)
endif()

# Temprorary help for Arch-based distros.
# They have wx2.8, lib32-wx2.8 and wx3.0 but no lib32-wx3.0.
# wx2.8 => /usr/bin/wx-config-2.8, /usr/bin/wxrc-2.8
# lib32-wx2.8 => /usr/bin/wx-config32-2.8, /usr/bin/wxrc32-2.8
# wx3.0 => /usr/bin/wx-config, /usr/bin/wxrc -> /usr/bin/wxrc-3.0
# I'm going to take a wild guess and predict this:
# lib32-wx3.0 => /usr/bin/wx-config32-3.0, /usr/bin/wxrc32-3.0
# FindwxWidgets only searches for wxrc and wx-config. Therefore only native
# wx3.0 works since everything else has non-standard naming.
if(NOT DEFINED wxWidgets_CONFIG_EXECUTABLE)
    if(CMAKE_CROSSCOMPILING)
        # May need to fix the filenames for lib32-wx3.0.
        if(NOT WX28_API AND ${PCSX2_TARGET_ARCHITECTURES} MATCHES "i386" AND EXISTS "/usr/bin/wx-config32-3.0" AND EXISTS "/usr/bin/wxrc32-3.0")
            set(wxWidgets_CONFIG_EXECUTABLE "/usr/bin/wx-config32-3.0")
            set(wxWidgets_wxrc_EXECUTABLE "/usr/bin/wxrc32-3.0")
        elseif(WX28_API AND ${PCSX2_TARGET_ARCHITECTURES} MATCHES "i386" AND EXISTS "/usr/bin/wx-config32-2.8" AND EXISTS "/usr/bin/wxrc32-2.8")
            set(wxWidgets_CONFIG_EXECUTABLE "/usr/bin/wx-config32-2.8")
            set(wxWidgets_wxrc_EXECUTABLE "/usr/bin/wxrc32-2.8")
        endif()
    else()
        if(WX28_API AND EXISTS "/usr/bin/wx-config-2.8" AND EXISTS "/usr/bin/wxrc-2.8")
            set(wxWidgets_CONFIG_EXECUTABLE "/usr/bin/wx-config-2.8")
            set(wxWidgets_wxrc_EXECUTABLE "/usr/bin/wxrc-2.8")
        endif()
    endif()
endif()

find_package(wxWidgets COMPONENTS base core adv)
find_package(ZLIB)

## Use pcsx2 package to find module
include(FindCg)
include(FindGlew)
include(FindLibc)

## Use CheckLib package to find module
include(CheckLib)
if(Linux)
    check_lib(AIO aio libaio.h)
endif()
check_lib(EGL EGL EGL/egl.h)
check_lib(GLESV2 GLESv2 GLES3/gl3ext.h) # NOTE: looking for GLESv3, not GLESv2
check_lib(PORTAUDIO portaudio portaudio.h pa_linux_alsa.h)
check_lib(SOUNDTOUCH SoundTouch soundtouch/SoundTouch.h)

if(SDL2_API)
    check_lib(SDL2 SDL2 SDL.h PATH_SUFFIXES SDL2)
else()
    # Tell cmake that we use SDL as a library and not as an application
    set(SDL_BUILDING_LIBRARY TRUE)
    find_package(SDL)
endif()

if(UNIX)
    find_package(X11)
    # Most plugins (if not all) and PCSX2 core need gtk2, so set the required flags
    if (GTK3_API)
        if(CMAKE_CROSSCOMPILING)
            find_package(GTK3 REQUIRED gtk)
        else()
            check_lib(GTK3 gtk+-3.0 gtk/gtk.h)
        endif()
    else()
        find_package(GTK2 REQUIRED gtk)
    endif()
endif()

#----------------------------------------
#		    Use system include
#----------------------------------------
if(UNIX)
	if(GTK2_FOUND)
		include_directories(${GTK2_INCLUDE_DIRS})
    elseif(GTK3_FOUND)
		include_directories(${GTK3_INCLUDE_DIRS})
        # A lazy solution
        set(GTK2_LIBRARIES ${GTK3_LIBRARIES})
	endif()

	if(X11_FOUND)
		include_directories(${X11_INCLUDE_DIR})
	endif()
endif()

if(ALSA_FOUND)
	include_directories(${ALSA_INCLUDE_DIRS})
endif()

if(BZIP2_FOUND)
	include_directories(${BZIP2_INCLUDE_DIR})
endif()

if(CG_FOUND)
	include_directories(${CG_INCLUDE_DIRS})
endif()

if(JPEG_FOUND)
	include_directories(${JPEG_INCLUDE_DIR})
endif()

if(GLEW_FOUND)
    include_directories(${GLEW_INCLUDE_DIR})
endif()

if(OPENGL_FOUND)
	include_directories(${OPENGL_INCLUDE_DIR})
endif()

if(SDL_FOUND AND NOT SDL2_API)
	include_directories(${SDL_INCLUDE_DIR})
endif()

if(wxWidgets_FOUND)
	include(${wxWidgets_USE_FILE})
endif()

if(ZLIB_FOUND)
	include_directories(${ZLIB_INCLUDE_DIRS})
endif()

#----------------------------------------
#  Use  project-wide include directories
#----------------------------------------
include_directories(${CMAKE_SOURCE_DIR}/common/include
					${CMAKE_SOURCE_DIR}/common/include/Utilities
					${CMAKE_SOURCE_DIR}/common/include/x86emitter
                    # File generated by Cmake
                    ${CMAKE_BINARY_DIR}/common/include
                    )

#----------------------------------------
# Check correctness of the parameter
# Note: wxWidgets_INCLUDE_DIRS must be defined
#----------------------------------------
include(ApiValidation)
WX_vs_SDL()
WX_vs_GTK3()
WX_version()
