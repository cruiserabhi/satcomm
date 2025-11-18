cmake_minimum_required(VERSION 3.10.2)

project(HarmanSatCommService LANGUAGES C CXX)

set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# TelSDK headers are under /usr/include/telux like Qualcomm's test app. :contentReference[oaicite:13]{index=13}
include_directories(BEFORE
  ${CMAKE_CURRENT_SOURCE_DIR}
  ${CMAKE_CURRENT_SOURCE_DIR}/include
  ${CMAKE_CURRENT_SOURCE_DIR}/ConfigLoader
  /usr/include/telux
)

# cJSON sources that your NtnConfigLoader.hpp depends on
set(CJSON_SRC
  ${CMAKE_CURRENT_SOURCE_DIR}/ConfigLoader/cJSON.c
  ${CMAKE_CURRENT_SOURCE_DIR}/ConfigLoader/cJSON_Utils.c
)

# ---------- Shared library ----------
set(SVC_SRC
  src/C_SatCommService.cpp
)
set(SVC_HDR
  ${CMAKE_CURRENT_SOURCE_DIR}/include/C_SatCommService.hpp
)

add_library(harman_satcom_service SHARED ${SVC_SRC} ${SVC_HDR} ${CJSON_SRC})
target_compile_options(harman_satcom_service PRIVATE -Wall -Wextra -Werror)
target_link_libraries(harman_satcom_service PRIVATE telux_satcom pthread)

install(TARGETS harman_satcom_service
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin)
install(FILES ${SVC_HDR} DESTINATION include/HarmanSatCommService)

# ---------- AudioTest app ----------
add_executable(AudioTest
  tests/AudioTest.cpp
  ${CJSON_SRC}
)
target_compile_options(AudioTest PRIVATE -Wall -Wextra -Werror)
target_link_libraries(AudioTest PRIVATE harman_satcom_service telux_satcom pthread)

# (Optional) mimic Qualcomm sample style for install
install(TARGETS AudioTest RUNTIME DESTINATION bin)
