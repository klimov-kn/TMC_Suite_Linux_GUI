# Цели сборки: шесть библиотек-математика (.a) + графические программы.
#
# Источник математики — дерево Windows-версии, перенесённое в win_src/ без
# изменений (снимок src из TMC_Suite). CMake лишь компилирует те же файлы под
# g++. Состав каждой библиотеки взят из соответствующего .vcxproj Windows-проекта.

set(WIN_SRC     ${CMAKE_CURRENT_SOURCE_DIR}/win_src)
set(TMC_INCLUDE ${WIN_SRC}/include)

# --- Слой совместимости MFC/win ---------------------------------------------
# MFC на Linux нет. compat/include содержит заглушки заголовков (afx.h, afxwin.h,
# cstringt.h, windows.h -> tmc_mfc.h) с минимальным CString и win-типами. Каталог
# подключается только вне Windows, чтобы #include <afx.h> в исходниках находил
# наш шим без правки самих исходников.
add_library(tmc_compat INTERFACE)
if(NOT WIN32)
  target_include_directories(tmc_compat INTERFACE
      ${CMAKE_CURRENT_SOURCE_DIR}/compat/include)
endif()

# Слой платформы: то, чего на Linux нет в готовом виде и что нужно КАЖДОЙ
# программе — текстовый режим файлов (CRLF, как в Windows), классы документа,
# настройки вместо реестра.
add_library(tmc_platform STATIC
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/tmc_mfc_doc.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/compat/tmc_textmode.cpp)
target_link_libraries(tmc_platform PUBLIC tmc_compat)
target_include_directories(tmc_platform PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/compat/include)

# Перехват fopen нужен на КОМПОНОВКЕ каждой программы, поэтому флаг объявлен
# интерфейсным: он передаётся всем, кто линкуется с этим слоем.
target_link_options(tmc_platform INTERFACE -Wl,--wrap=fopen)

# Преамбула для файлов win_src: то, что MSVC давал неявно (тип _complex,
# макросы __max/__min, слой MFC). Подставляется компилятором в начало каждого
# такого файла, поэтому сами исходники править не приходится.
set(TMC_PRELUDE_FLAGS
    -include ${CMAKE_CURRENT_SOURCE_DIR}/compat/include/tmc_win_prelude.h)

# -----------------------------------------------------------------------------
# tmc_add_library(<name> DIRS <каталог>... FILES <имя.cpp>...)
#
# Компилирует в статическую библиотеку ТОЛЬКО перечисленные в FILES файлы — ровно
# те, что входят в соответствующий Windows-проект (.vcxproj). Имена сравниваются
# БЕЗ учёта регистра: в .vcxproj они записаны иначе, чем реальные файлы на диске
# (DEF_MACR.CPP против Def_macr.cpp), а Linux к регистру чувствителен.
function(tmc_add_library name)
  cmake_parse_arguments(A "" "" "DIRS;FILES" ${ARGN})

  set(allow "")
  foreach(f ${A_FILES})
    string(TOLOWER "${f}" lf)
    list(APPEND allow "${lf}")
  endforeach()

  set(srcs "")
  foreach(dir ${A_DIRS})
    file(GLOB part CONFIGURE_DEPENDS
         "${WIN_SRC}/${dir}/*.cpp"
         "${WIN_SRC}/${dir}/*.CPP")
    foreach(p ${part})
      get_filename_component(bn "${p}" NAME)
      string(TOLOWER "${bn}" lbn)
      if(lbn IN_LIST allow)
        list(APPEND srcs "${p}")
      endif()
    endforeach()
  endforeach()

  list(LENGTH srcs n_found)
  list(LENGTH allow n_want)
  if(NOT n_found EQUAL n_want)
    message(WARNING "${name}: найдено ${n_found} из ${n_want} файлов (.vcxproj). "
                    "Проверьте имена в FILES и наличие файлов на диске.")
  endif()

  add_library(${name} STATIC ${srcs})
  target_include_directories(${name} PUBLIC ${TMC_INCLUDE})
  foreach(dir ${A_DIRS})
    target_include_directories(${name} PUBLIC ${WIN_SRC}/${dir})
  endforeach()
  target_link_libraries(${name} PUBLIC tmc_compat)
  # Исходники содержат классические MSVC-вольности, которые g++ по стандарту
  # считает ошибками, но семантика от них не меняется (extra qualification,
  # приведение указателя к int при выравнивании, строковый литерал -> char*).
  # Флаг лишь понижает эти диагностики до предупреждений.
  target_compile_options(${name} PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})
endfunction()

# --- Ф1: шесть библиотек-математика ------------------------------------------
tmc_add_library(complex     DIRS libs/complex      FILES CMPLXALL.CPP)

tmc_add_library(sfile95     DIRS libs/sfile95      FILES
    Errors.cpp Frqdata.cpp Read_s.cpp S_file.cpp S_memf.cpp S_remt.cpp
    S_table.cpp S_wemt.cpp Sadd.cpp Sofftbl.cpp Unite_s.cpp Write_s.cpp Wrrd_s.cpp)

# Expr и Inter — один Windows-проект exprint.
tmc_add_library(exprint     DIRS libs/expr libs/inter FILES
    Expr_com.cpp Expr_err.cpp Expr_in1.cpp Expr_per.cpp Expremsg.cpp Get_err.cpp
    Inte_in1.cpp Inte_ope.cpp Inte_val.cpp)

tmc_add_library(TMCLibError DIRS libs/tmcliberror  FILES TmcLibError.cpp)

tmc_add_library(prepr       DIRS libs/prepr        FILES
    Def_dest.cpp Def_er.cpp Def_erd.cpp Def_macr.cpp Def_str.cpp Defmis.cpp
    Prep_ld.cpp Prep_mn.cpp Read_buf.cpp)

tmc_add_library(TMCIndan    DIRS libs/tmcindan     FILES
    FieldIntegrated.cpp TmcRTH_BolckList.cpp TmcRTH_Indan.cpp TmcRTH_IndanOutput.cpp
    TmcRTH_IndanParam.cpp TmcRTH_IndanTopology.cpp TmcRTH_Input.cpp TmcRTH_InputNode.cpp)

# --- Зависимости между библиотеками ------------------------------------------
# Направление «зависящая -> зависимость». Между prepr и sfile95 есть ВЗАИМНАЯ
# зависимость (alloc_mem_err/free_mem в SFILE95 зовут prepr, put_error_messege
# в prepr зовёт sfile95), поэтому при линковке программ набор оборачивается
# в --start-group/--end-group: однопроходный GNU ld цикл иначе не разрешает.
target_link_libraries(prepr    PUBLIC TMCLibError)
target_link_libraries(TMCIndan PUBLIC prepr exprint TMCLibError)

# Полный набор библиотек для любой программы TMC (порядок роли не играет —
# группа многопроходная).
set(TMC_MATH_LIBS sfile95 prepr exprint TMCIndan TMCLibError complex
    CACHE INTERNAL "Библиотеки-математика TMC")

# Готовый набор для target_link_libraries программы: цикл разрешается группой.
set(TMC_MATH_LINK
    -Wl,--start-group ${TMC_MATH_LIBS} -Wl,--end-group tmc_compat
    CACHE INTERNAL "Строка линковки математики TMC")

# Печать матрицы рассеяния из файла .s — тем же кодом, каким её читают
# программы пакета. Нужна для сверки результатов разных сборок по числам.
# В поставку не входит: средство разработки. Qt не требуется.
add_executable(sdump ${CMAKE_SOURCE_DIR}/tools/dev/src/sdump.cpp)
target_include_directories(sdump PRIVATE
    ${TMC_INCLUDE} ${CMAKE_SOURCE_DIR}/src/compat/include)
target_link_libraries(sdump PRIVATE ${TMC_MATH_LINK})
target_compile_options(sdump PRIVATE -fpermissive -w ${TMC_PRELUDE_FLAGS})

# --- Программы ----------------------------------------------------------------
# Ядра программ (документ, файлы, расчёт) собираются всегда: они не зависят от Qt
# и их можно проверять на машине без установленного Qt. Сам интерфейс — только
# когда Qt найден.
if(TMC_BUILD_GUI)
  add_subdirectory(gui_common)    # тема, общие виджеты, ресурсы
endif()
add_subdirectory(apps)            # TMCROS, TMCGROUT, TMC_DN, PlanarRT_*, FieldView
