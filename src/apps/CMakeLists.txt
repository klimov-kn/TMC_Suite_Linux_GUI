# Графические программы TMC Suite.
# Каждая программа: ядро (модули расчёта и файлов из win_src) + интерфейс на Qt.
add_subdirectory(tmcros)     # вьювер S-матриц (сигнал во времени)
add_subdirectory(tmcgrout)   # вьювер графиков по частоте
add_subdirectory(tmc_dn)     # диаграммы направленности
add_subdirectory(fieldview)  # визуализация полей (OpenGL)
add_subdirectory(planarrt_h) # оболочка счётного ядра H
add_subdirectory(planarrt_x) # оболочка счётного ядра X (+6 макросов)
