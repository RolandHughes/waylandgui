cmake_minimum_required (VERSION 3.13..3.18)
set(CMAKE_OSX_DEPLOYMENT_TARGET 10.13)

#
#
message(STATUS "*******************")
message(STATUS "****** expects GLFW to be built from source from:")
message(STATUS "****** git clone https://github.com/glfw/glfw.git")
message(STATUS "******")
message(STATUS "****** Packages fromt he repository will pull in libglvnd")
message(STATUS "****** which breaks xWayland/Wayland")
message(STATUS "*******************")

# Extract project version from source
file(STRINGS "${CMAKE_CURRENT_SOURCE_DIR}/include/waylandgui/common.h"
  waylandgui_version_defines REGEX "#define WAYLANDGUI_VERSION_(MAJOR|MINOR|PATCH) ")

foreach(ver ${waylandgui_version_defines})
  if(ver MATCHES [[#define WAYLANDGUI_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$]])
      set(WAYLANDGUI_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}")
  endif()
endforeach()

if(WAYLANDGUI_VERSION_PATCH MATCHES [[\.([a-zA-Z0-9]+)$]])
  set(WAYLANDGUI_VERSION_TYPE "${CMAKE_MATCH_1}")
endif()

string(REGEX MATCH "^[0-9]+" WAYLANDGUI_VERSION_PATCH "${WAYLANDGUI_VERSION_PATCH}")

set(WAYLANDGUI_VERSION "${WAYLANDGUI_VERSION_MAJOR}.${WAYLANDGUI_VERSION_MINOR}.${WAYLANDGUI_VERSION_PATCH}")

message(STATUS "WaylandGUI v${WAYLANDGUI_VERSION} ${WAYLANDGUI_VERSION_TYPE}")

project(waylandgui
  DESCRIPTION
    "WaylandGUI"
  LANGUAGES
    CXX C
  VERSION
    ${WAYLANDGUI_VERSION}
)

if (NOT IS_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/ext/nanovg/src")
  message(FATAL_ERROR "The WaylandGUI dependency repositories (nanovg, etc.) are missing! "
    "You probably did not clone the project with --recursive. It is possible to recover "
    "by calling \"git submodule update --init --recursive\"")
endif()

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  message(STATUS "Setting build type to 'Release' as none was specified.")
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release"
    "MinSizeRel" "RelWithDebInfo")
endif()
string(TOUPPER "${CMAKE_BUILD_TYPE}" U_CMAKE_BUILD_TYPE)

set(WAYLANDGUI_MASTER_PROJECT OFF)
if (${CMAKE_CURRENT_SOURCE_DIR} STREQUAL ${CMAKE_SOURCE_DIR})
  set(WAYLANDGUI_MASTER_PROJECT ON)
endif()

set(WAYLANDGUI_BUILD_SHARED_DEFAULT ON)

if (CMAKE_CXX_COMPILER MATCHES "/em\\+\\+(-[a-zA-Z0-9.])?$")
  set(CMAKE_CXX_COMPILER_ID "Emscripten")
  set(WAYLANDGUI_BUILD_SHARED_DEFAULT OFF)

  set(CMAKE_STATIC_LIBRARY_SUFFIX ".bc")
  set(CMAKE_EXECUTABLE_SUFFIX ".bc")
  set(CMAKE_CXX_CREATE_STATIC_LIBRARY "<CMAKE_CXX_COMPILER> -o <TARGET> <LINK_FLAGS> <OBJECTS>")
  if (U_CMAKE_BUILD_TYPE MATCHES REL)
    add_compile_options(-O3 -DNDEBUG)
  endif()
endif()

option(WAYLANDGUI_BUILD_EXAMPLES            "Build WaylandGUI example application?" ON)
option(WAYLANDGUI_BUILD_SHARED              "Build WaylandGUI as a shared library?" ${WAYLANDGUI_BUILD_SHARED_DEFAULT})
option(WAYLANDGUI_INSTALL                   "Install WaylandGUI on `make install`?" ON)

include(GNUInstallDirs)
include(CMakeDependentOption)
include(FindPkgConfig)

# We always use Wayland
#
pkg_check_modules(Wayland REQUIRED
    wayland-client>=0.2.7
    wayland-cursor>=0.2.7
    wayland-egl>=0.2.7
    xkbcommon)
  list(APPEND WAYLANDGUI_LIBS "${Wayland_LINK_LIBRARIES}")

  message(STATUS "Wayland_LINK_LIBRARIES: ${Wayland_LINK_LIBRARIES}")

# We always use glfw3 for Wayland Weston environments
#
find_package(glfw3 REQUIRED)
list(APPEND WAYLANDGUI_LIBS glfw)

#  There is no GLESv3 library. Everything is in the header for
#  3 and the methods/functions/etc. are still in the v2 library
#
list(APPEND WAYLANDGUI_LIBS GLESv2) 


# Required libraries, flags, and include files for compiling and linking against waylandgui (all targets)
set(WAYLANDGUI_EXTRA_INCS "")

include(CheckCXXCompilerFlag)
include(CheckCXXSourceRuns)

macro(CHECK_CXX_COMPILER_AND_LINKER_FLAGS _RESULT _CXX_FLAGS _LINKER_FLAGS)
  set(CMAKE_REQUIRED_FLAGS ${_CXX_FLAGS})
  set(CMAKE_REQUIRED_LIBRARIES ${_LINKER_FLAGS})
  set(CMAKE_REQUIRED_QUIET TRUE)
  check_cxx_source_runs("#include <iostream>\nint main(int argc, char **argv) { std::cout << \"test\"; return 0; }" ${_RESULT})
  set(CMAKE_REQUIRED_FLAGS "")
  set(CMAKE_REQUIRED_LIBRARIES "")
endmacro()

# TODO:: Need to come up with Wayland versions of these
#        For now Wayland will always use GLES3
#
  list(APPEND WAYLANDGUI_EXTRA
    src/texture_gl.cpp src/shader_gl.cpp
    src/renderpass_gl.cpp src/opengl.cpp
    src/opengl_check.h
  )

  # Keep NanoVG in synch with what we are using
  #  
  set(NANOVG_GLES3_IMPLEMENTATION ON)
  
  set(WAYLANDGUI_GLOB "resources/*.gles")
  message(STATUS "WAYLANDGUI_LIBS: ${WAYLANDGUI_LIBS}")
# end TODO::  


# Shared library mode: add dllimport/dllexport flags to all symbols
if (WAYLANDGUI_BUILD_SHARED)
  message(STATUS "WaylandGUI: building shared library.")
else()
  message(STATUS "WaylandGUI: building static library.")
endif()

# Use C++17, visibility=hidden by default, interprocedural optimization
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

# Prefer libc++ in conjunction with Clang
if (CMAKE_CXX_COMPILER_ID MATCHES "Clang" AND NOT CMAKE_CXX_FLAGS MATCHES "-stdlib=libc\\+\\+")
  CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPP "-stdlib=libc++" "-stdlib=libc++")
  if (OR HAS_LIBCPP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -D_LIBCPP_VERSION")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++")
    message(STATUS "WaylandGUI: using libc++.")
  else()
    CHECK_CXX_COMPILER_AND_LINKER_FLAGS(HAS_LIBCPP_AND_CPPABI "-stdlib=libc++" "-stdlib=libc++ -lc++abi")
    if (HAS_LIBCPP_AND_CPPABI)
      set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -stdlib=libc++ -D_LIBCPP_VERSION")
      set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
      set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -stdlib=libc++ -lc++abi")
      message(STATUS "WaylandGUI: using libc++ and libc++abi.")
    else()
      message(FATAL_ERROR "When Clang is used to compile WaylandGUI, libc++ must be available -- GCC's libstdc++ is not supported! (please insteall the libc++ development headers, provided e.g. by the packages 'libc++-dev' and 'libc++abi-dev' on Debian/Ubuntu).")
    endif()
  endif()
endif()


if (${CMAKE_SYSTEM_NAME} MATCHES "BSD")
  include_directories(/usr/local/include)
  link_directories(/usr/local/lib)
  if (${CMAKE_SYSTEM_NAME} MATCHES "OpenBSD")
    include_directories(/usr/X11R6/include/)
    link_directories(/usr/X11R6/lib)
  endif()
endif()

set(WAYLANDGUI_ORIGIN "$ORIGIN")

if (CMAKE_SYSTEM MATCHES "Linux" OR CMAKE_SYSTEM_NAME MATCHES "BSD")
  list(APPEND WAYLANDGUI_LIBS pthread)
  if (NOT CMAKE_SYSTEM_NAME MATCHES "OpenBSD")
    list(APPEND WAYLANDGUI_LIBS rt)
  endif()
  if (CMAKE_SYSTEM MATCHES "Linux")
    list(APPEND WAYLANDGUI_LIBS dl)
  endif()
endif()

  
# Run simple cmake converter to put font files into the data segment

# Glob up resource files
file(GLOB resources
  "${CMAKE_CURRENT_SOURCE_DIR}/resources/*.ttf"
  "${WAYLANDGUI_GLOB}")

file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/resources")

# Precompile binary resources and shaders
foreach(fname_in IN LISTS resources)
    set(fname_out "${fname_in}")
  list(APPEND resources_processed ${fname_out})
endforeach()

# Concatenate resource files into a comma separated string
string(REGEX REPLACE "([^\\]|^);" "\\1," resources_string "${resources_processed}")
string(REGEX REPLACE "[\\](.)" "\\1" resources_string "${resources_string}")

# Create command line for running bin2c cmake script
set(bin2c_cmdline
  -DOUTPUT_C=waylandgui_resources.cpp
  -DOUTPUT_H=waylandgui_resources.h
  "-DINPUT_FILES=${resources_string}"
  -P "${CMAKE_CURRENT_SOURCE_DIR}/resources/bin2c.cmake")


# Run bin2c on resource files
add_custom_command(
  OUTPUT waylandgui_resources.cpp waylandgui_resources.h
  COMMAND ${CMAKE_COMMAND} ARGS ${bin2c_cmdline}
  DEPENDS ${resources} ${resources_processed}
  COMMENT "Running bin2c"
  PRE_BUILD VERBATIM)

# Needed to generated files
include_directories(${CMAKE_CURRENT_BINARY_DIR})

# Set library type
if (WAYLANDGUI_BUILD_SHARED)
  set(WAYLANDGUI_LIBRARY_TYPE "SHARED")
else()
  set(WAYLANDGUI_LIBRARY_TYPE "STATIC")
endif()

# Compile main WaylandGUI library
add_library(waylandgui ${WAYLANDGUI_LIBRARY_TYPE}
  # Merge NanoVG into the NanoGUI library
  ext/nanovg/src/nanovg.c
  ext/nanovg/src/stb_image.h

  # Merge into the NanoGUI library (only if needed)
  ${WAYLANDGUI_EXTRA}

  # Fonts etc.
  waylandgui_resources.cpp
  include/waylandgui/common.h src/common.cpp
  include/waylandgui/widget.h src/widget.cpp
  include/waylandgui/theme.h src/theme.cpp
  include/waylandgui/layout.h src/layout.cpp
  include/waylandgui/screen.h src/screen.cpp
  include/waylandgui/label.h src/label.cpp
  include/waylandgui/window.h src/window.cpp
  include/waylandgui/popup.h src/popup.cpp
  include/waylandgui/checkbox.h src/checkbox.cpp
  include/waylandgui/button.h src/button.cpp
  include/waylandgui/popupbutton.h src/popupbutton.cpp
  include/waylandgui/combobox.h src/combobox.cpp
  include/waylandgui/progressbar.h src/progressbar.cpp
  include/waylandgui/slider.h src/slider.cpp
  include/waylandgui/messagedialog.h src/messagedialog.cpp
  include/waylandgui/textbox.h src/textbox.cpp
  include/waylandgui/textarea.h src/textarea.cpp
  include/waylandgui/imagepanel.h src/imagepanel.cpp
  include/waylandgui/vscrollpanel.h src/vscrollpanel.cpp
  include/waylandgui/colorwheel.h src/colorwheel.cpp
  include/waylandgui/colorpicker.h src/colorpicker.cpp
  include/waylandgui/graph.h src/graph.cpp
  include/waylandgui/tabwidget.h src/tabwidget.cpp
  include/waylandgui/canvas.h src/canvas.cpp
  include/waylandgui/texture.h src/texture.cpp
  include/waylandgui/shader.h src/shader.cpp
  include/waylandgui/imageview.h src/imageview.cpp
  include/waylandgui/traits.h src/traits.cpp
  include/waylandgui/renderpass.h
  include/waylandgui/formhelper.h
  include/waylandgui/icons.h
  include/waylandgui/toolbutton.h
  include/waylandgui/opengl.h
  include/waylandgui/waylandgui.h
)

target_compile_definitions(waylandgui
  PUBLIC
    ${WAYLANDGUI_BACKEND_DEFS}

  PRIVATE
    -DWAYLANDGUI_BUILD
    -DNVG_STB_IMAGE_IMPLEMENTATION
)

target_include_directories(waylandgui
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>

  PRIVATE
    ext/nanovg/src
    ${CMAKE_CURRENT_BINARY_DIR}
)

if (WAYLANDGUI_BUILD_SHARED)
  target_compile_definitions(waylandgui
    PUBLIC
      -DWAYLANDGUI_SHARED -DNVG_SHARED
    PRIVATE
      -DNVG_BUILD
  )

endif()

target_include_directories(waylandgui PUBLIC
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/ext/nanovg/src>
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}/nanovg>)

if (WAYLANDGUI_BUILD_SHARED)
  target_link_libraries(waylandgui PRIVATE ${WAYLANDGUI_LIBS})
  set_property(TARGET waylandgui PROPERTY INTERPROCEDURAL_OPTIMIZATION ON)
  set_property(TARGET waylandgui PROPERTY DEFINE_SYMBOL "")
else()
  target_link_libraries(waylandgui PUBLIC ${WAYLANDGUI_LIBS})
endif()

# Auto-detect GLES include directory on Raspberry PI
if (EXISTS /opt/vc/include)
  target_include_directories(waylandgui PUBLIC /opt/vc/include)
endif()

if (WAYLANDGUI_INSTALL)
  install(TARGETS waylandgui
          LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
          ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR})

  install(DIRECTORY include/waylandgui DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
          FILES_MATCHING PATTERN "*.h")

  install(DIRECTORY ext/nanovg/src/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/nanovg
          FILES_MATCHING PATTERN "*.h")

  install(TARGETS waylandgui EXPORT waylandguiTargets)

  set(WAYLANDGUI_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_DATAROOTDIR}/cmake/waylandgui")

  include(CMakePackageConfigHelpers)
  configure_package_config_file(
    resources/waylandguiConfig.cmake.in waylandguiConfig.cmake
    INSTALL_DESTINATION ${WAYLANDGUI_CMAKECONFIG_INSTALL_DIR})

  write_basic_package_version_file(
    waylandguiConfigVersion.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY AnyNewerVersion ARCH_INDEPENDENT)

  install(
    EXPORT waylandguiTargets
    DESTINATION ${WAYLANDGUI_CMAKECONFIG_INSTALL_DIR})

  install(
    FILES
    ${CMAKE_CURRENT_BINARY_DIR}/waylandguiConfigVersion.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/waylandguiConfig.cmake
    DESTINATION ${WAYLANDGUI_CMAKECONFIG_INSTALL_DIR})
endif()

# Quench warnings while compiling NanoVG
if (CMAKE_COMPILER_IS_GNUCC)
  set_source_files_properties(ext/nanovg/src/nanovg.c PROPERTIES COMPILE_FLAGS -Wno-unused-result)
endif()

# Build example application if desired
if (WAYLANDGUI_BUILD_EXAMPLES)
  add_executable(example1      src/example1.cpp)
  add_executable(example2      src/example2.cpp)
  add_executable(example3      src/example3.cpp)
  add_executable(example4      src/example4.cpp)
  add_executable(example_icons src/example_icons.cpp)

  # One must include the WAYLANDGUI_LIBS as well
  target_link_libraries(example1      waylandgui ${WAYLANDGUI_LIBS})
  target_link_libraries(example2      waylandgui ${WAYLANDGUI_LIBS})
  target_link_libraries(example3      waylandgui ${WAYLANDGUI_LIBS})
  target_link_libraries(example4      waylandgui ${WAYLANDGUI_LIBS})
  target_link_libraries(example_icons waylandgui ${WAYLANDGUI_LIBS})

  # Copy icons for example application
  file(COPY resources/icons DESTINATION ${CMAKE_CURRENT_BINARY_DIR})
endif()



# vim: set et ts=2 sw=2 ft=cmake nospell:
