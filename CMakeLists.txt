# CMakeList.txt: XmlParser 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project(xmlparser)

SET(CMAKE_CXX_FLAGS "-std=c++11 -O3")

add_subdirectory(Pugixml)
add_subdirectory(INIFILE2)
add_subdirectory(src)

# 将源代码添加到此项目的可执行文件。
aux_source_directory(./src DIR_SRCS)
add_executable(src ${DIR_SRCS})

target_link_libraries(src Pugixml)
target_link_libraries(src INIFILE2)
# TODO: 如有需要，请添加测试并安装目标。
