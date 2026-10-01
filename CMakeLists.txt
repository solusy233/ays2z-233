cmake_minimum_required(VERSION 3.16)

project("安阳市第二中学放学关机" VERSION 0.1 LANGUAGES CXX)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(QT NAMES Qt6 Qt5 REQUIRED COMPONENTS Widgets Multimedia Network Svg)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Widgets Multimedia Network Svg)


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
    qt_add_executable(SchoolPoweroff
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
    )
else()
    if(ANDROID)
        add_library(SchoolPoweroff SHARED
            ${PROJECT_SOURCES}
        )
    else()
        add_executable(SchoolPoweroff
            ${PROJECT_SOURCES}
        )
    endif()
endif()

target_link_libraries(SchoolPoweroff PRIVATE
    Qt${QT_VERSION_MAJOR}::Widgets
    Qt${QT_VERSION_MAJOR}::Multimedia
    Qt${QT_VERSION_MAJOR}::Network
    Qt${QT_VERSION_MAJOR}::Svg
)

add_custom_command(TARGET SchoolPoweroff POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Auto_poweroff233"
            "$<TARGET_FILE_DIR:SchoolPoweroff>/Auto_poweroff233"
        COMMAND ${CMAKE_COMMAND}
            -DSETTINGS_SOURCE="${CMAKE_CURRENT_SOURCE_DIR}/settings.json"
            -DSETTINGS_DESTINATION="$<TARGET_FILE_DIR:SchoolPoweroff>/settings.json"
            -P "${CMAKE_CURRENT_SOURCE_DIR}/cmake/CopySettingsIfMissing.cmake"
        COMMAND ${CMAKE_COMMAND} -E make_directory
            "$<TARGET_FILE_DIR:SchoolPoweroff>/booth"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/booth/booth.m4a"
            "$<TARGET_FILE_DIR:SchoolPoweroff>/booth/booth.m4a"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/weather/icons"
            "$<TARGET_FILE_DIR:SchoolPoweroff>/weather/icons"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/weather/picture"
            "$<TARGET_FILE_DIR:SchoolPoweroff>/weather/picture"
    COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/Fout"
            "$<TARGET_FILE_DIR:SchoolPoweroff>/Fout"
)

if(APPLE)
    if(${QT_VERSION} VERSION_LESS 6.1.0)
        set(BUNDLE_ID_OPTION MACOSX_BUNDLE_GUI_IDENTIFIER com.example.SchoolPoweroff)
    endif()
    set_target_properties(SchoolPoweroff PROPERTIES
        ${BUNDLE_ID_OPTION}
        MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
        MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
        MACOSX_BUNDLE TRUE
    )
elseif(WIN32)
    set_target_properties(SchoolPoweroff PROPERTIES
        WIN32_EXECUTABLE TRUE
    )
endif()

include(GNUInstallDirs)
install(TARGETS SchoolPoweroff
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(SchoolPoweroff)
endif()
