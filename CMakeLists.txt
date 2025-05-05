cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project(usb_meter_osd LANGUAGES CXX)

include(ProcessorCount)
ProcessorCount(N)
if(NOT N EQUAL 0)
    set(CMAKE_BUILD_PARALLEL_LEVEL ${N})
endif()

if(APPLE)
    # Setzen Sie dies direkt nach dem project() Befehl
    set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "Minimum OS X deployment version" FORCE)
    # Needed for gettimeofday() on some macOS versions
    add_definitions(-D_DARWIN_C_SOURCE)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
endif()

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


set(wxBUILD_SHARED OFF)
set(wxUSE_WEBVIEW OFF)
set(wxUSE_REGEX OFF)
set(wxUSE_EXPAT OFF)
set(wxUSE_LIBTIFF OFF)
set(wxUSE_STC OFF)
set(wxUSE_RIBBON OFF)

message(STATUS "Fetching wxWidgets...")

FetchContent_Declare(
        wxWidgets
        GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
        GIT_SHALLOW ON
)
FetchContent_MakeAvailable(wxWidgets)
set(EXECUTABLE_NAME "USB-C Power")

set(SRCS
        Application.cpp
        GraphPanel.cpp
        GraphPanel.h
        PowerDelivery.cpp
        PowerDelivery.h
        OsdSettings.cpp
        OsdSettings.h
        SerialThread.cpp
        SerialThread.h
        SerialDriver.cpp
        SerialDriver.h
        SerialPortEnumerator.cpp
        SerialPortEnumerator.h
        Events.h
        Events.cpp
        MeasurementEvent.cpp
        MeasurementEvent.h
)
if(APPLE)
    # Create the app bundle
    add_executable(usbc-power-osd MACOSX_BUNDLE ${SRCS})

    # Set the icon name (just the filename)
    set(APP_ICON_NAME "usbpower.icns")

    # Set bundle properties
    set_target_properties(usbc-power-osd PROPERTIES
            MACOSX_BUNDLE TRUE
            MACOSX_BUNDLE_BUNDLE_NAME "USB-C Power"
            MACOSX_BUNDLE_EXECUTABLE_NAME "usbc-power-osd"
            MACOSX_BUNDLE_ICON_FILE ${APP_ICON_NAME}
            MACOSX_BUNDLE_GUI_IDENTIFIER "de.macwake.usbosd"
            MACOSX_BUNDLE_INFO_STRING "USB-C Power Monitor"
            MACOSX_BUNDLE_COPYRIGHT "Copyright MacWake.de"
            MACOSX_BUNDLE_SHORT_VERSION_STRING "1.0"
            MACOSX_BUNDLE_BUNDLE_VERSION "1.0"
            # Specify our custom Info.plist template
            MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in"
    )

    # Set the full path to the icon for the copy operation
    set(APP_ICON_PATH "${CMAKE_CURRENT_SOURCE_DIR}/${APP_ICON_NAME}")

    # Ensure the icon file is copied to Resources directory
    target_sources(usbc-power-osd PRIVATE ${APP_ICON_PATH})
    set_source_files_properties(${APP_ICON_PATH} PROPERTIES
            MACOSX_PACKAGE_LOCATION "Resources")

    # Add debugging message
    message(STATUS "Configuring macOS bundle with icon: ${APP_ICON_NAME}")
    message(STATUS "Using Info.plist template: ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist.in")
else()
    # the WIN32 is needed for Windows in order for it to look for WinMain
    # instead of the main function. This is ignored on other systems,
    # so it works on all platforms
    add_executable(usbc-power-osd WIN32 ${SRCS} usbc-power-osd.exe.manifest)
endif()

target_link_libraries(usbc-power-osd PRIVATE wxcore wxnet)