cmake_minimum_required(VERSION 3.10)
project(ble_v1)

set(CMAKE_CXX_STANDARD 17)

# Method 1: Using pkg-config (recommended)
find_package(PkgConfig REQUIRED)
pkg_check_modules(SYSTEMD REQUIRED libsystemd)

# Add headers path manually
include_directories("/usr/include/sdbus-c++")


find_package(sdbus-c++ REQUIRED)
# Add executable
add_executable(ble_v1 main.cpp)
target_link_libraries(ble_v1 ${SYSTEMD_LIBRARIES})
target_include_directories(ble_v1 PRIVATE ${SYSTEMD_INCLUDE_DIRS})
target_compile_options(ble_v1 PRIVATE ${SYSTEMD_CFLAGS_OTHER})

# Link to the library directly
target_link_libraries(ble_v1  sdbus-c++)
