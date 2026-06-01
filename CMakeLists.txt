cmake_minimum_required(VERSION 3.20)
project(kinet-accel VERSION 0.1.0 LANGUAGES CXX)

# kinet-accel is THE shared library for Go/FFI consumers
# It statically links kinet-gpu core and exports only C ABI

find_package(kinet-gpu CONFIG REQUIRED)

# Build as SHARED library
add_library(kinetaccel SHARED
  src/session.cpp
  src/tensor.cpp
  src/ml.cpp
  src/crypto.cpp
  src/zk.cpp
  src/lattice.cpp
  src/fhe.cpp
  src/dex.cpp
  src/c_api.cpp
)

add_library(kinet::accel ALIAS kinetaccel)

target_compile_features(kinetaccel PUBLIC cxx_std_23)

# Building the library - export symbols
target_compile_definitions(kinetaccel PRIVATE KINET_ACCEL_BUILDING)

target_include_directories(kinetaccel
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)

# Link gpu core PRIVATELY (statically embedded)
# The static library symbols are absorbed into kinetaccel.so
target_link_libraries(kinetaccel
  PRIVATE kinet::kinet_gpu_core
)

# Platform libraries for dlopen (backend plugin loading)
if(UNIX AND NOT APPLE)
  target_link_libraries(kinetaccel PRIVATE dl)
endif()

# Hide all symbols except those explicitly exported
set_target_properties(kinetaccel PROPERTIES
  CXX_VISIBILITY_PRESET hidden
  VISIBILITY_INLINES_HIDDEN ON
  # Version info for soname
  VERSION ${PROJECT_VERSION}
  SOVERSION 0
)

# macOS: ensure symbols are resolved at load time
if(APPLE)
  set_target_properties(kinetaccel PROPERTIES
    MACOSX_RPATH ON
    INSTALL_RPATH "@loader_path"
  )
endif()

include(GNUInstallDirs)
include(CMakePackageConfigHelpers)

# Install shared library
install(TARGETS kinetaccel
  EXPORT kinet-accel-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

# Install only the C API header (not internal C++ headers)
install(FILES
  include/kinet/accel/c_api.h
  include/kinet/accel/backend_api.h
  DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/kinet/accel
)

install(EXPORT kinet-accel-targets
  NAMESPACE kinet::
  FILE kinet-accelTargets.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kinet-accel
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/cmake/kinet-accelConfig.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/kinet-accelConfig.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kinet-accel
)

write_basic_package_version_file(
  ${CMAKE_CURRENT_BINARY_DIR}/kinet-accelConfigVersion.cmake
  VERSION ${PROJECT_VERSION}
  COMPATIBILITY ExactVersion
)

install(FILES
  ${CMAKE_CURRENT_BINARY_DIR}/kinet-accelConfig.cmake
  ${CMAKE_CURRENT_BINARY_DIR}/kinet-accelConfigVersion.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/kinet-accel
)
