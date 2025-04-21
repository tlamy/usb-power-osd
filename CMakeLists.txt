cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project(usb_meter_osd LANGUAGES CXX)

include(FetchContent)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set architecture for macOS builds
if (APPLE)
    # For universal binary (works on both Intel and Apple Silicon)
    set(CMAKE_OSX_ARCHITECTURES "x86_64;arm64")
    # Or for Intel-only: set(CMAKE_OSX_ARCHITECTURES "x86_64")
endif ()

set(wxBUILD_SHARED OFF)

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
    # create bundle on apple compiles
    add_executable(usbc-power-osd MACOSX_BUNDLE ${SRCS})

    list(APPEND SRC_FILES ./wxmac.icns)

    # Set a custom plist file for the app bundle - needed for Mac OS Retina display
    set_target_properties(usbc-power-osd PROPERTIES
            MACOSX_BUNDLE_INFO_PLIST ${CMAKE_CURRENT_SOURCE_DIR}/Info.plist
            RESOURCE "./wxmac.icns"
            MACOSX_BUNDLE_ICON_FILE wxmac.icns
            MACOSX_BUNDLE_COPYRIGHT "Copyright MacWake.de"
            MACOSX_BUNDLE_GUI_IDENTIFIER "de.macwake.usbosd"
    )
else()
    # the WIN32 is needed for Windows in order for it to look for WinMain
    # instead of the main function. This is ignored on other systems,
    # so it works on all platforms
    add_executable(usbc-power-osd WIN32 ${SRCS} usbc-power-osd.exe.manifest)
endif()

target_link_libraries(usbc-power-osd PRIVATE wxcore wxnet)