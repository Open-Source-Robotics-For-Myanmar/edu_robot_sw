cmake_minimum_required(VERSION 3.16)

project(rom_socket VERSION 1.0.0 LANGUAGES CXX)

# C++ Standard ကို သတ်မှတ်ခြင်း
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Qt ရဲ့ MOC, UIC, RCC တွေကို အလိုအလျောက် run ပေးဖို့
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

# Qt 6 ကို ရှာဖွေခြင်း
find_package(Qt6 6.2 REQUIRED COMPONENTS
    Core
    Network
)

# libmpv ကို ရှာဖွေခြင်း
find_package(PkgConfig REQUIRED)
pkg_check_modules(MPV REQUIRED IMPORTED_TARGET mpv)

# Source ဖိုင်တွေကို သတ်မှတ်ခြင်း
set(PROJECT_SOURCES
    src/main.cpp
    src/ssl_server.cpp
    src/ssl_server.h
    src/wifi_tcp_server.cpp
    src/wifi_tcp_server.h
    src/wifi_configurator.cpp
    src/wifi_configurator.h
    src/settings_manager.cpp
    src/settings_manager.h
    src/behavior_tree_updater.cpp
    src/behavior_tree_updater.h
)

# Executable ဖန်တီးခြင်း
add_executable(${PROJECT_NAME}
    ${PROJECT_SOURCES}
)

# Qt libraries နဲ့ libmpv ကို link လုပ်ခြင်း
target_link_libraries(${PROJECT_NAME} PRIVATE
    Qt6::Core
    Qt6::Network
    PkgConfig::MPV
)

# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Debug mode မှာ compiler warnings တွေကို ပိုပြခြင်း
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_options(${PROJECT_NAME} PRIVATE
        -Wall
        -Wextra
        -Wpedantic
    )
endif()

# Install ဖို့ သတ်မှတ်ခြင်း
install(TARGETS ${PROJECT_NAME}
    RUNTIME DESTINATION bin
)
