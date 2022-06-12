cmake_minimum_required(VERSION 3.15.0)

project(panner-webview
        VERSION 0.1
        DESCRIPTION "Steinberg VST 3 Panner example using vstwebview"
        )

# The samples were useful for looking at, but now they're bloating the build dir.
set(SMTG_ADD_VST3_PLUGINS_SAMPLES OFF)
set(SMTG_ADD_VST3_HOSTING_SAMPLES OFF)

set(SMTG_CREATE_PLUGIN_LINK ON)

# If validation is on then the execution of the host in debug seems to fail
set(SMTG_RUN_VST_VALIDATOR OFF)

# We don't want VSTGUI to bloat the binary.
set(SMTG_ADD_VSTGUI OFF)

set(BUILD_SHARED_LIBS FALSE)

# Abseil link on Linux will fail without this.
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

if (UNIX)
    add_compile_options(-Wno-suggest-override)
    if (NOT APPLE)
        set(PLATFORM_LIBRARIES tbb)
    endif()
endif()

smtg_enable_vst3_sdk()

smtg_add_vst3plugin(panner
        include/plugcontroller.h
        include/plugids.h
        include/plugprocessor.h
        include/version.h
        source/plugfactory.cpp
        source/plugcontroller.cpp
        source/plugprocessor.cpp
        )

smtg_target_add_plugin_resources(panner
        RESOURCES
        resource/background.png
        resource/index.html
        resource/style.css
        resource/main.js
        )

smtg_target_add_plugin_snapshots(panner
        RESOURCES
        resource/A2EAF7DB320640F48EDE380DDF89562C_snapshot.png
        resource/A2EAF7DB320640F48EDE380DDF89562C_snapshot_2.0x.png
        )


target_link_libraries(panner
        PRIVATE
        vstwebview
        sdk
        nlohmann_json::nlohmann_json
        ${PLATFORM_LIBRARIES}
        )