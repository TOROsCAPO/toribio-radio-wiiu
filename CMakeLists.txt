cmake_minimum_required(VERSION 3.13)
project(toribio_wiiu_suite C)
set(CMAKE_C_STANDARD 11)
find_package(PkgConfig REQUIRED)
pkg_check_modules(RADIO_DEPS REQUIRED IMPORTED_TARGET sdl2 libcurl libmpg123)
pkg_check_modules(JPEG_DEPS REQUIRED IMPORTED_TARGET libturbojpeg)
set_property(TARGET PkgConfig::RADIO_DEPS APPEND PROPERTY
  INTERFACE_LINK_LIBRARIES brotlidec brotlicommon)

add_library(toribio_common common/src/app.c)
target_include_directories(toribio_common PUBLIC common/include)
target_link_libraries(toribio_common PUBLIC PkgConfig::JPEG_DEPS)

function(add_toribio_app target source)
  add_executable(${target} ${source})
  target_link_libraries(${target} PRIVATE toribio_common)
  wut_create_rpx(${target})
endfunction()

add_toribio_app(wiiu-radio "apps/radio/src/main.c;apps/radio/src/radio_backend.c;apps/radio/src/catalog_backend.c;apps/radio/src/tls_config.c")
target_include_directories(wiiu-radio PRIVATE apps/radio/src)
target_link_libraries(wiiu-radio PRIVATE PkgConfig::RADIO_DEPS)
dkp_add_embedded_binary_library(radio_backgrounds
  apps/radio/assets/radio_bg_tv.jpg
  apps/radio/assets/radio_bg_drc.jpg
  apps/radio/assets/toribio_watermark_tv.rgba
  apps/radio/assets/toribio_watermark_drc.rgba)
target_link_libraries(wiiu-radio PRIVATE radio_backgrounds)
add_toribio_app(tapo-wiiu-viewer apps/tapo-viewer/src/main.c)
add_toribio_app(toribio-browser apps/toribio-browser/src/main.c)
