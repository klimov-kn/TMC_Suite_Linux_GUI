# TMC Suite Qt — графические программы TMC Suite для Linux.
#
# Математика компилируется из исходников Windows-версии (src/win_src) без правок:
# формулы, константы и форматы файлов должны совпадать байт в байт. На Qt
# переписан только интерфейс.
#
# Сборка (Ubuntu 24.04, g++ 13, Qt 6):
#   cmake -S . -B build -G Ninja -DCMAKE_BUILD_TYPE=Release
#   cmake --build build -j

cmake_minimum_required(VERSION 3.22)

project(TMC_Suite_Qt
        VERSION 1.0.0
        DESCRIPTION "TMC Suite: графические программы для Linux (Qt 6)"
        LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

# Бинарники и библиотеки — в одно место, чтобы запускать без поиска по дереву.
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)

# --- Арифметика с плавающей точкой -------------------------------------------
# g++ по умолчанию имеет право объединять умножение со сложением в одну операцию
# (FMA). Результат при этом чуть точнее, но ОТЛИЧАЕТСЯ в последних битах от того,
# что считала Windows-версия: MSVC такого объединения не делает. Нам нужны
# совпадающие числа, а не более точные, поэтому объединение запрещаем.
# Это не изменение математики: формулы и порядок операций остаются прежними.
add_compile_options(-ffp-contract=off)

if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Тип сборки" FORCE)
endif()

# Qt нужен только графическим программам. Библиотеки-математика собираются и без
# него — это позволяет проверять счёт на машине, где Qt ещё не установлен.
option(TMC_BUILD_GUI "Собирать графические программы (требуется Qt 6)" ON)

if(TMC_BUILD_GUI)
  find_package(Qt6 6.4 QUIET COMPONENTS Core Gui Widgets OpenGLWidgets PrintSupport)
  if(NOT Qt6_FOUND)
    message(WARNING
      "Qt 6 не найден — графические программы собираться не будут. "
      "Установите: sudo apt install qt6-base-dev qt6-base-dev-tools")
    set(TMC_BUILD_GUI OFF)
  else()
    message(STATUS "Qt ${Qt6_VERSION}")
    set(CMAKE_AUTOMOC ON)
    set(CMAKE_AUTOUIC ON)
    set(CMAKE_AUTORCC ON)
  endif()
endif()

add_subdirectory(src)

message(STATUS "TMC Suite Qt ${PROJECT_VERSION}: тип сборки ${CMAKE_BUILD_TYPE}, "
               "GUI=${TMC_BUILD_GUI}")
