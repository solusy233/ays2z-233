cmake_minimum_required(VERSION 3.16)

project("安阳市第二中学放学关机" VERSION 0.1 LANGUAGES CXX)

set(CMAKE_PREFIX_PATH "C:/Qt/6.9.1/mingw_64" ${CMAKE_PREFIX_PATH})
set(Qt6_DIR "C:/Qt/6.9.1/mingw_64/lib/cmake/Qt6")

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia Network)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia Network)

set(PROJECT_SOURCES
        main.cpp
        booth/booth.cpp
        booth/booth.h
        aboutwindows/mainwindows/mainwindow.cpp
        aboutwindows/mainwindows/mainwindow.h
        aboutwindows/mainwindows/mainwindow.ui
        aboutwindows/settings/settingswindow.cpp
        aboutwindows/settings/settingswindow.h
        aboutwindows/settings/settingswindow.ui
        Auto_poweroff233/Auto_poweroff.cpp
        Auto_poweroff233/Auto_poweroff.h
        weather/weatherapi.cpp
        weather/weatherapi.h
        weather/weather_background.cpp
        weather/weather_background.h
        weather/weatherwindow.cpp
        weather/weatherwindow.h
        weather/weatherwindow.ui
        aboutwindows/mainwindows/app_icon.rc
        resources.qrc
)

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(untitled
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET untitled APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation
else()
    if(ANDROID)
        add_library(untitled SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(untitled
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(untitled PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Network
)

add_custom_command(TARGET untitled POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Auto_poweroff233"
            "$<TARGET_FILE_DIR:untitled>/Auto_poweroff233"
        COMMAND ${CMAKE_COMMAND}
            -DSETTINGS_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/settings.json"
            -DSETTINGS_DESTINATION="$<TARGET_FILE_DIR:untitled>/settings.json"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopySettingsIfMissing.cmake"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:untitled>/booth"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/booth/booth.m4a"
            "$<TARGET_FILE_DIR:untitled>/booth/booth.m4a"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/weather/picture"
            "$<TARGET_FILE_DIR:untitled>/weather/picture"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Fout"
            "$<TARGET_FILE_DIR:untitled>/Fout"
)

# Qt for iOS sets MACOSX_BUNDLE_GUI_IDENTIFIER automatically since Qt 6.1.
# If you are developing for iOS or macOS you should consider setting an
# explicit, fixed bundle identifier manually though.
if(${QT_VERSION} VERSION_LESS 6.1.0)
  set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.untitled)
endif()
set_target_properties(untitled PROPERTIES
    ${BUNDLE_ID_OPTION}
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
    MACOSX_BUNDLE TRUE
    WIN32_EXECUTABLE TRUE
)

include(GNUInstallDirs)
install(TARGETS untitled
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(untitled)
endif()
