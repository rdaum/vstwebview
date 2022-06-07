cmake_minimum_required(VERSION 3.21.0)

set(CMAKE_CXX_STANDARD 20)
set(ABSL_PROPAGATE_CXX_STD ON)

project(vstwebview)

# Third party packages are pulled in via FetchContent
include(FetchContent)

# VST3SDK
FetchContent_Declare(
        vst3sdk
        GIT_REPOSITORY https://github.com/steinbergmedia/vst3sdk.git
        GIT_SHALLOW 1
)
FetchContent_MakeAvailable(vst3sdk)

# json
FetchContent_Declare(json URL https://github.com/nlohmann/json/releases/download/v3.10.5/json.tar.xz)
FetchContent_MakeAvailable(json)

smtg_enable_vst3_sdk()
IF (WIN32)
    set(WEBVIEW_PLATFORM_SOURCES
            src/vstwebview/win32/webview_win32.h
           src/vstwebview/win32/webview_win32.cc
           src/vstwebview/win32/webview_edge_chromium.h
           src/vstwebview/win32/webview_edge_chromium.cc)
ENDIF()

if(UNIX AND NOT APPLE)
    set(WEBVIEW_PLATFORM_SOURCES
           src/vstwebview/gtk/webview_gtk.cc
            )
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(GTK3 REQUIRED gtk+-3.0)
    pkg_check_modules(GTKMM3 REQUIRED gtkmm-3.0)
    pkg_check_modules(WEBKIT2GTK REQUIRED webkit2gtk-4.1)
endif()


add_library(vstwebview
        ${WEBVIEW_PLATFORM_SOURCES}
        src/vstwebview/webview_controller_bindings.cc
        src/vstwebview/webview_pluginview.cc
        src/vstwebview/webview.cc)

target_compile_options(vstwebview PUBLIC "$<$<CONFIG:DEBUG>:-DDEVELOPMENT>")
target_compile_options(vstwebview PUBLIC "$<$<CONFIG:RELEASE>:-DRELEASE>")
target_include_directories(vstwebview PRIVATE ./src ${vstsdk_SOURCE_DIR} ${json_SOURCE_DIR}/include)
target_include_directories(vstwebview PUBLIC ./include)
add_dependencies(vstwebview nlohmann_json::nlohmann_json)

# Needed for webview2.h & friends on Win32.
if (WIN32)
    option(WIL_BUILD_PACKAGING "" OFF)
    option(WIL_BUILD_TESTS  "" OFF)
    FetchContent_Declare(wil GIT_REPOSITORY "https://github.com/microsoft/wil")
    FetchContent_MakeAvailable(wil)

    nuget_add(WebView2 "Microsoft.Web.WebView2" 1.0.1150.38)
    nuget_add(CppWinRT "Microsoft.Windows.CppWinRT" 2.0.220315.1)

    target_compile_definitions(vstwebview PRIVATE UNICODE=1 _UNICODE=1)
    target_include_directories(vstwebview PRIVATE ${WebView2_PATH}/build/native/include)

    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
        target_link_libraries(vstwebview PRIVATE ${WebView2_PATH}/build/native/x64/WebView2LoaderStatic.lib)
    else()
        target_link_libraries(vstwebview PRIVATE ${WebView2_PATH}/build/native/x86/WebView2LoaderStatic.lib)
    endif()

    target_link_libraries(vstwebview PRIVATE WIL::WIL Dwmapi Shlwapi)
endif()

if(UNIX AND NOT APPLE)
    target_include_directories(vstwebview PRIVATE ${GTK3_INCLUDE_DIRS} ${WEBKIT2GTK_INCLUDE_DIRS} ${GTKMM3_INCLUDE_DIRS})
    target_link_libraries(vstwebview PRIVATE ${GTK3_LIBRARIES} ${WEBKIT2GTK_LIBRARIES} ${GTKMM3_LIBRARIES})
endif()
