project(vrtzmq)

set(VRTZMQ_SOURCES
    vrt-tools.cpp
    vrtzmqinput.cpp
    vrtzmqplugin.cpp
    vrtzmqsettings.cpp
#    vrtzmqwebapiadapter.cpp
    vrtzmqthread.cpp
)

set(VRTZMQ_HEADERS
    vrt-tools.h
    vrtzmqinput.h
    vrtzmqplugin.h
    vrtzmqsettings.h
#    vrtzmqwebapiadapter.h
    vrtzmqthread.h
)
include_directories(
    ${CMAKE_SOURCE_DIR}/swagger/sdrangel/code/qt5/client
    ${LIBVRTZMQ_INCLUDE_DIR}
    ${VRT_INCLUDE_DIR}
    ${ZMQ_INCLUDE_DIR}
)

if(NOT SERVER_MODE)
    set(VRTZMQ_SOURCES
        ${VRTZMQ_SOURCES}
        vrtzmqgui.cpp

        vrtzmqgui.ui
    )
    set(VRTZMQ_HEADERS
        ${VRTZMQ_HEADERS}
        vrtzmqgui.h
    )

    set(TARGET_NAME ${PLUGINS_PREFIX}inputvrtzmq)
    set(TARGET_LIB "Qt::Widgets")
    set(TARGET_LIB_GUI "sdrgui")
    set(INSTALL_FOLDER ${INSTALL_PLUGINS_DIR})
else()
    set(TARGET_NAME ${PLUGINSSRV_PREFIX}inputvrtzmqsrv)
    set(TARGET_LIB "")
    set(TARGET_LIB_GUI "")
    set(INSTALL_FOLDER ${INSTALL_PLUGINSSRV_DIR})
endif()

if(NOT Qt6_FOUND)
    add_library(${TARGET_NAME} ${VRTZMQ_SOURCES})
else()
    qt_add_plugin(${TARGET_NAME} CLASS_NAME VRTZMQPlugin ${VRTZMQ_SOURCES})
endif()

if(NOT BUILD_SHARED_LIBS)
    set_property(GLOBAL APPEND PROPERTY STATIC_PLUGINS_PROPERTY ${TARGET_NAME})
endif()

target_link_libraries(${TARGET_NAME} PRIVATE
        Qt::Core
        ${TARGET_LIB}
	sdrbase
	${TARGET_LIB_GUI}
        swagger
        ${VRT_LIBRARIES}
        ${ZMQ_LIBRARIES}
)

install(TARGETS ${TARGET_NAME} DESTINATION ${INSTALL_FOLDER})

# Install debug symbols
if (WIN32)
    install(FILES $<TARGET_PROPERTY:${TARGET_NAME},RUNTIME_OUTPUT_DIRECTORY>/${TARGET_NAME}stripped.pdb CONFIGURATIONS Release DESTINATION ${INSTALL_FOLDER} RENAME ${TARGET_NAME}.pdb )
    install(FILES $<TARGET_PDB_FILE:${TARGET_NAME}> CONFIGURATIONS Debug RelWithDebInfo DESTINATION ${INSTALL_FOLDER} )
endif()
