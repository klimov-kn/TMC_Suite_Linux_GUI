# Общий слой интерфейса для всех программ TMC Suite:
#   * tmc_mfcqt — реализация классов MFC (рисование, окна, виды) поверх Qt.
#     Благодаря ей виды программ компилируются из Windows-версии без переписывания.
#   * theme     — оформление в палитре TPL Designer.
#   * виджет-хозяин, который отдаёт виду события мыши, клавиш и таймера.

add_library(tmc_gui_common STATIC
    ${CMAKE_SOURCE_DIR}/src/compat/mfc_gdi.cpp
    ${CMAKE_SOURCE_DIR}/src/compat/mfc_view.cpp
    ${CMAKE_SOURCE_DIR}/src/compat/mfc_dialog.cpp
    ${CMAKE_SOURCE_DIR}/src/compat/mfc_thread.cpp
    ${CMAKE_SOURCE_DIR}/src/compat/mfc_wgl.cpp
    theme.cpp
    aboutbox.cpp
    mfcviewwidget.cpp
    mfcglwidget.cpp
    printsupport.cpp
    recentfiles.cpp
    commandupdate.cpp    # состояние команд (ON_UPDATE_COMMAND_UI)
    dialogshot.cpp       # снимок диалога для проверки внешнего вида
    windowplacement.cpp  # положение окна между запусками
    mousewatchdog.cpp)   # защита от «залипшего» захвата мыши

target_include_directories(tmc_gui_common PUBLIC
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src/compat/include
    ${TMC_INCLUDE})

# OpenGLWidgets нужен виду FieldView; для остальных программ он безвреден.
# PrintSupport — печать вида тем же кодом, что рисует на экране.
target_link_libraries(tmc_gui_common PUBLIC Qt6::Widgets Qt6::OpenGLWidgets Qt6::PrintSupport tmc_platform)

# Проверка связки программ: запуск соседних программ по windows-именам.
# В поставку не входит — средство разработки.
add_executable(tmc_launchcheck ${CMAKE_SOURCE_DIR}/tools/dev/src/launchcheck.cpp)
target_link_libraries(tmc_launchcheck PRIVATE tmc_gui_common)
