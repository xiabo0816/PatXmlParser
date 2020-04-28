# CMakeList.txt: XmlParser 的 CMake 项目，在此处包括源代码并定义
# 项目特定的逻辑。
#
cmake_minimum_required (VERSION 3.8)

project(xmlparser)

add_subdirectory(pugixml)
add_subdirectory(inifile2)
add_subdirectory(src)

# 将源代码添加到此项目的可执行文件。
aux_source_directory(./src DIR_SRCS)
add_executable(xmlparser ${DIR_SRCS})
find_package(Threads)
target_link_libraries(xmlparser ${CMAKE_THREAD_LIBS_INIT})

target_link_libraries(xmlparser Pugixml)
target_link_libraries(xmlparser INIFILE2)
# TODO: 如有需要，请添加测试并安装目标。
