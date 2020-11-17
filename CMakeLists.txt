cmake_minimum_required(VERSION 3.10)
project(QdsPyCli)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_BUILD_TYPE RELEASE)



# --------------QdsPyCli python-----------------
add_library(QdsPyCli MODULE
        client.cpp
        Qage.cpp
        qdsport.cpp
        SPacker.cpp
        XType.cpp
        extmain.cpp)

set_target_properties(QdsPyCli
        PROPERTIES COMPILE_FLAGS "-mrdrnd -Wall -Wextra -fPIC -g")

target_link_libraries(QdsPyCli
        ${PYTHON_LIBRARIES}
        PUBLIC crypto
        )

find_package (Python3 COMPONENTS Development)

target_include_directories(QdsPyCli PUBLIC
        ${Python3_INCLUDE_DIRS})

set_target_properties(QdsPyCli
        PROPERTIES PREFIX ""
        )

set(TESTDIR ${CMAKE_BINARY_DIR}/../)
set(TESTDIR2 ${CMAKE_BINARY_DIR}/../../myprojects/if4admin/api/CppExt/)
add_custom_command(TARGET QdsPyCli
        POST_BUILD
        COMMAND cp QdsPyCli.so ${TESTDIR}
        COMMAND cp QdsPyCli.so ${TESTDIR2}
        )
