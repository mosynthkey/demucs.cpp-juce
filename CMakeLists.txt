cmake_minimum_required(VERSION 3.22)

project(DemucsJUCE VERSION 0.0.1)
include(FetchContent)
# Set OpenBLAS options before including demucs.cpp
set(USE_OPENBLAS ON CACHE BOOL "Use OpenBLAS")
set(BLA_VENDOR OpenBLAS)
if(APPLE)
    set(CMAKE_PREFIX_PATH ${CMAKE_PREFIX_PATH} "/opt/homebrew/opt/openblas")
    find_package(BLAS REQUIRED)
elseif(WIN32)
    # Version and download URL for OpenBLAS
    set(OPENBLAS_VERSION "0.3.29")
    set(OPENBLAS_URL "https://github.com/OpenMathLib/OpenBLAS/releases/download/v${OPENBLAS_VERSION}/OpenBLAS-${OPENBLAS_VERSION}_x64.zip")
    
    # Download and extract OpenBLAS
    FetchContent_Declare(
        openblas
        URL ${OPENBLAS_URL}
        DOWNLOAD_EXTRACT_TIMESTAMP TRUE
    )
    FetchContent_MakeAvailable(openblas)
    
    # Set OpenBLAS path
    set(OPENBLAS_ROOT "${openblas_SOURCE_DIR}")
    set(OPENBLAS_INCLUDE_DIR "${OPENBLAS_ROOT}/include")
    set(OPENBLAS_LIBRARIES "${OPENBLAS_ROOT}/lib/libopenblas.dll.a")
    # for demucs.cpp
    set(BLAS_LIBRARIES "${OPENBLAS_LIBRARIES}" CACHE PATH "Path to BLAS library")
    set(BLAS_FOUND TRUE CACHE BOOL "BLAS found")
endif()
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
        ${OPENBLAS_INCLUDE_DIR}
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
        "${OPENBLAS_LIBRARIES}"
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
)

if(USE_OPENBLAS)
    set(BLAS_LIBRARIES "${OPENBLAS_LIBRARIES}")
    set(BLAS_FOUND TRUE)
endif()

# copy dll for windows
if(WIN32)
    add_custom_command(TARGET DemucsJUCE POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${OPENBLAS_ROOT}/bin/libopenblas.dll"
            $<TARGET_FILE_DIR:DemucsJUCE>
    )
endif() 