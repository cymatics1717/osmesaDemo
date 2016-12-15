project(ARmarker)
cmake_minimum_required(VERSION 2.8)

find_package(PkgConfig REQUIRED)

pkg_check_modules(MESA REQUIRED osmesa)
include_directories(${MESA_INCLUDE_DIRS})
link_directories(${MESA_LIBRARY_DIRS})

pkg_check_modules(CV REQUIRED opencv)
include_directories(${CV_INCLUDE_DIRS})
link_directories(${CV_LIBRARY_DIRS})

pkg_check_modules(GLU REQUIRED glu)
include_directories(${GLU_INCLUDE_DIRS})
link_directories(${GLU_LIBRARY_DIRS})

pkg_check_modules(ASSIMP REQUIRED assimp)
include_directories(${ASSIMP_INCLUDE_DIRS})
link_directories(${ASSIMP_LIBRARY_DIRS})

aux_source_directory(. SRC_LIST)

add_executable(${PROJECT_NAME} ${SRC_LIST})

target_link_libraries(${PROJECT_NAME}
    ${MESA_LIBRARIES}
    ${CV_LIBRARIES}
    ${GLU_LIBRARIES}
    ${ASSIMP_LIBRARIES})

set(LIB_SRC arwidget.cc)
add_library(ARmesa SHARED ${LIB_SRC})

#g++ main.cc -I. -L. -lARmesa  `pkg-config --libs --cflags opencv assimp osmesa` && LD_LIBRARY_PATH=.  ./a.out
