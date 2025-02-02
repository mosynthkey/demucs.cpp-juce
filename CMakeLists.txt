cmake_minimum_required(VERSION 3.22)

project(DemucsJUCE VERSION 0.0.1)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/Submodule/JUCE)

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

# Add include directories for JUCE headers
target_include_directories(DemucsJUCE
    PRIVATE
        ${CMAKE_CURRENT_SOURCE_DIR}/Source
        ${CMAKE_CURRENT_BINARY_DIR}/JuceLibraryCode
)

target_compile_definitions(DemucsJUCE
    PRIVATE
        JUCE_WEB_BROWSER=0
        JUCE_USE_CURL=0
        JUCE_APPLICATION_NAME_STRING="$<TARGET_PROPERTY:DemucsJUCE,PRODUCT_NAME>"
        JUCE_APPLICATION_VERSION_STRING="$<TARGET_PROPERTY:DemucsJUCE,VERSION>"
)

target_link_libraries(DemucsJUCE
    PRIVATE
        juce::juce_core
        juce::juce_data_structures
        juce::juce_events
        juce::juce_graphics
        juce::juce_gui_basics
    PUBLIC
        juce::juce_recommended_config_flags
        juce::juce_recommended_lto_flags
        juce::juce_recommended_warning_flags
) 