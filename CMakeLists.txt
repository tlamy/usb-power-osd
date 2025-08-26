cmake_minimum_required(VERSION 3.30 FATAL_ERROR)
project(usb_meter_osd LANGUAGES CXX C OBJCXX)

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

# Add X11 dependency for Linux
if(UNIX AND NOT APPLE)
    find_package(X11 REQUIRED)
endif()

include(FetchContent)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Add SimpleBLE from submodule
if (EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/third-party/SimpleBLE/simpleble/CMakeLists.txt")
    message(STATUS "Found SimpleBLE submodule, enabling BLE support")

    # Configure SimpleBLE options
    set(SIMPLEBLE_PLAIN OFF CACHE BOOL "" FORCE)
    set(SIMPLEBLE_LOG_LEVEL "FATAL" CACHE STRING "" FORCE)

    # Add SimpleBLE subdirectory
    add_subdirectory(third-party/SimpleBLE/simpleble)

    set(BLE_ENABLED TRUE)
else ()
    message(WARNING "SimpleBLE submodule not found. BLE support will be disabled.")
    set(BLE_ENABLED FALSE)
endif ()

# Basic build options
set(wxBUILD_SHARED OFF)
set(wxBUILD_USE_STATIC_RUNTIME ON)
set(wxBUILD_PRECOMP OFF)
set(wxBUILD_SAMPLES OFF)
set(wxBUILD_TESTS OFF)
set(wxBUILD_DEMOS OFF)
set(wxBUILD_INSTALL OFF)

# Required features
set(wxUSE_GUI ON)
set(wxUSE_UNICODE ON)
set(wxUSE_DATAVIEWCTRL ON)  # Required for wxTreeListCtrl
set(wxUSE_TREELISTCTRL ON)

# Disabled features
set(wxUSE_LIBWEBP OFF)
set(wxUSE_WEBVIEW OFF)
set(wxUSE_LIBJPEG OFF)
set(wxUSE_REGEX OFF)
set(wxUSE_EXPAT OFF)
set(wxUSE_LIBTIFF OFF)
set(wxUSE_STC OFF)
set(wxUSE_RIBBON OFF)
set(wxUSE_RICHTEXT OFF)
set(wxUSE_HTML OFF)
set(wxUSE_MEDIACTRL OFF)
set(wxUSE_PROPGRID OFF)
set(wxUSE_GRID OFF)
set(wxUSE_WEBSOCKET OFF)
set(wxUSE_GLCANVAS OFF)
set(wxUSE_SOUND OFF)
set(wxUSE_XRC OFF)
set(wxUSE_XML OFF)
set(wxUSE_PRINTING_ARCHITECTURE OFF)
set(wxUSE_HELP OFF)
set(wxUSE_WXHTML_HELP OFF)
set(wxUSE_HELPHTML OFF)
message(STATUS "Fetching wxWidgets...")

FetchContent_Declare(
        wxWidgets
        GIT_REPOSITORY https://github.com/wxWidgets/wxWidgets.git
        GIT_TAG v3.2.8.1
        GIT_SHALLOW ON)
FetchContent_MakeAvailable(wxWidgets)
set(EXECUTABLE_NAME "USB-C Power")

set(SRCS
        Application.cpp
        BLEDeviceEnumerator.cpp
        BLEDeviceEnumerator.h
        BLEDriver.h
        CommThread.cpp
        CommThread.h
        DeviceSelectionDialog.cpp
        DeviceSelectionDialog.h
        Events.cpp
        Events.h
        GraphPanel.cpp
        GraphPanel.h
        MeasurementEvent.cpp
        MeasurementEvent.h
        OsdSettings.cpp
        OsdSettings.h
        PowerDelivery.cpp
        PowerDelivery.h
        SelectedDevice.cpp
        SelectedDevice.h
        serialib.cpp
        serialib.h
        SerialPortEnumerator.cpp
        SerialPortEnumerator.h
        SettingsDialog.cpp
        SettingsDialog.h
        JsonParser.h
        JsonParser.cpp
)

# Add Windows resource file
if(WIN32)
    set(SRCS ${SRCS} resources.rc)
endif()

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

target_compile_definitions(usbc-power-osd PRIVATE USE_SIMPLEBLE)
target_link_libraries(usbc-power-osd PRIVATE simpleble::simpleble)
message(STATUS "BLE support enabled")

# Platform-specific linking
if(UNIX AND NOT APPLE)
    target_link_libraries(usbc-power-osd PRIVATE wxcore wxnet X11::X11)
else()
    target_link_libraries(usbc-power-osd PRIVATE wxcore wxnet)
endif()

if(Windows)
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static-libgcc -static-libstdc++")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -static")
    add_definitions(-DSTATIC_BUILD)
endif()