cmake_minimum_required(VERSION 3.5)

project(parpargui VERSION 0.1 LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # our Release builds are statically linked
    list(PREPEND CMAKE_FIND_LIBRARY_SUFFIXES .a .lib)
    #set(CMAKE_EXE_LINKER_FLAGS "-static")
endif()
configure_file("config.h.in" "config.h")

find_package(QT NAMES Qt6 Qt5 COMPONENTS Widgets REQUIRED )
find_package(QT NAMES Qt6 Qt5 OPTIONAL_COMPONENTS LinguistTools)
find_package(Qt${QT_VERSION_MAJOR} COMPONENTS Widgets REQUIRED)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS LinguistTools)

set(TS_FILES parpargui_en_GB.ts)

set(PROJECT_SOURCES
        main.cpp
        mainwindow.cpp
        mainwindow.h
        mainwindow.ui
        optionsdialog.h optionsdialog.cpp optionsdialog.ui
        sizeedit.h sizeedit.cpp
        settings.h settings.cpp
        util.cpp util.h
        par2calc.h par2calc.cpp
        createprogress.h createprogress.cpp createprogress.ui
        progressdialog.h progressdialog.cpp
        slicecountspinbox.h slicecountspinbox.cpp
        par2outinfo.h par2outinfo.cpp
        outpreviewlistitem.h outpreviewlistitem.cpp
        sourcefilelistitem.h sourcefilelistitem.cpp
        sourcefile.h sourcefile.cpp
        sourcefileframe.h sourcefileframe.cpp
        clientinfo.h clientinfo.cpp
        res.qrc
        ${TS_FILES}
)

set(APP_ICON_RESOURCE_WINDOWS "${CMAKE_CURRENT_SOURCE_DIR}/res/parpargui.rc")

if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
    qt_add_executable(parpargui
        MANUAL_FINALIZATION
        ${PROJECT_SOURCES}
        ${APP_ICON_RESOURCE_WINDOWS}
    )
# Define target properties for Android with Qt 6 as:
#    set_property(TARGET parpargui APPEND PROPERTY QT_ANDROID_PACKAGE_SOURCE_DIR
#                 ${CMAKE_CURRENT_SOURCE_DIR}/android)
# For more information, see https://doc.qt.io/qt-6/qt-add-executable.html#target-creation

    if(Qt6LinguistTools_FOUND)
        qt_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    endif()
else()
    if(ANDROID)
        add_library(parpargui SHARED
            ${PROJECT_SOURCES}
        )
# Define properties for Android with Qt 5 after find_package() calls as:
#    set(ANDROID_PACKAGE_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/android")
    else()
        add_executable(parpargui
            ${PROJECT_SOURCES}
            ${APP_ICON_RESOURCE_WINDOWS}
        )
    endif()

    if(Qt5LinguistTools_FOUND)
        qt5_create_translation(QM_FILES ${CMAKE_SOURCE_DIR} ${TS_FILES})
    endif()
endif()

target_link_libraries(parpargui PRIVATE Qt${QT_VERSION_MAJOR}::Widgets)

set_target_properties(parpargui PROPERTIES
    MACOSX_BUNDLE_GUI_IDENTIFIER my.example.com
    MACOSX_BUNDLE_BUNDLE_VERSION ${PROJECT_VERSION}
    MACOSX_BUNDLE_SHORT_VERSION_STRING ${PROJECT_VERSION_MAJOR}.${PROJECT_VERSION_MINOR}
)

# Release desirables
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    # TODO: proper LTO with Qt source
    set_property(TARGET ${PROJECT_NAME} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
    if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
        add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
            COMMAND ${CMAKE_STRIP} ${PROJECT_NAME}${CMAKE_EXECUTABLE_SUFFIX})
    endif()
endif()
set_property(TARGET ${PROJECT_NAME} PROPERTY WIN32_EXECUTABLE true)

if(QT_VERSION_MAJOR EQUAL 6)
    qt_finalize_executable(parpargui)
endif()
