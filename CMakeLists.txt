cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project(usb_meter_osd LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(wxBUILD_SHARED OFF)

message(STATUS "Fetching wxWidgets...")

FetchContent_Declare(
        wxWidgets
        GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(wxWidgets)
set(EXECUTABLE_NAME "USB-C Power")

set(SRCS main.cpp
        GraphPanel.cpp
        GraphPanel.h)
if(APPLE)
    # create bundle on apple compiles
    add_executable(usbc-power-osd MACOSX_BUNDLE ${SRCS})

    # Set a custom plist file for the app bundle - needed for Mac OS Retina display
    set_target_properties(usbc-power-osd PROPERTIES MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist)
else()
    # the WIN32 is needed for Windows in order for it to look for WinMain
    # instead of the main function. This is ignored on other systems,
    # so it works on all platforms
    add_executable(usbc-power-osd WIN32 ${SRCS} usbc-power-osd.exe.manifest)
endif()

target_link_libraries(usbc-power-osd PRIVATE wxcore wxnet)