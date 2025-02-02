cmake_minimum_required(VERSION 3.22)

project(DemucsJUCE VERSION 0.0.1)

# Set OpenBLAS options before including demucs.cpp
set(USE_OPENBLAS ON CACHE BOOL "Use OpenBLAS")
set(BLA_VENDOR OpenBLAS)
set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/opt/homebrew/opt/openblas")

# Add JUCE
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Submodule/JUCE)

# Configure demucs.cpp build
set(JUCE_BUILD_HELPER_TOOLS OFF CACHE BOOL "")
set(JUCE_BUILD_EXAMPLES OFF CACHE BOOL "")
set(JUCE_BUILD_TESTS OFF CACHE BOOL "")

# Add demucs.cpp but disable tests and examples
set(BUILD_TESTING OFF CACHE BOOL "")
set(BUILD_EXAMPLES OFF CACHE BOOL "")
add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Submodule/demucs.cpp EXCLUDE_FROM_ALL)

juce_add_gui_app(DemucsJUCE
    PRODUCT_NAME "Demucs JUCE"
    VERSION "0.0.1"
    COMPANY_NAME "Your Company"
    COMPANY_WEBSITE "https://yourcompany.com"
    COMPANY_EMAIL "info@yourcompany.com"
    BUNDLE_ID "com.yourcompany.demucsjuce"
)

juce_generate_juce_header(DemucsJUCE)

target_sources(DemucsJUCE
    PRIVATE
        Source/Main.cpp
        Source/MainComponent.cpp
)

target_include_directories(DemucsJUCE
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_BINARY_DIR}/JuceLibraryCode
        ${CMAKE_CURRENT_SOURCE_DIR}/Submodule/demucs.cpp/src
)

target_compile_definitions(DemucsJUCE
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DemucsJUCE,JUCE_PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DemucsJUCE,JUCE_VERSION>"
)

target_link_libraries(DemucsJUCE
    PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
        juce::juce_gui_extra
        juce::juce_audio_basics
        juce::juce_audio_devices
        juce::juce_audio_formats
        juce::juce_audio_processors
        juce::juce_audio_utils
        demucs.cpp.lib
        BLAS::BLAS
        curl
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(USE_OPENBLAS)
    find_package(BLAS REQUIRED)
    target_link_libraries(DemucsJUCE PRIVATE BLAS::BLAS)
endif() 