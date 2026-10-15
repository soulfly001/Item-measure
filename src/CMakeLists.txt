set(CMAKE_INCLUDE_CURRENT_DIR ON)

# 启用 Qt 自动化工具
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 17)

# MSVC 编译选项
if(MSVC)
    add_compile_options(/utf-8)
endif()

# ==========================================
# 1. 配置 Qt6
# ==========================================
set(CMAKE_PREFIX_PATH "F:/QT/qt6/6.8.3/msvc2022_64/lib/cmake")
find_package(Qt6 REQUIRED COMPONENTS Core Gui Widgets Quick QuickControls2 Xml Concurrent)

# ==========================================
# 2. 手动配置 OpenCV (绕过 find_package 的版本检测)
# ==========================================
# 头文件路径
set(OpenCV_INCLUDE_DIRS "F:/OpenCV/opencv/build/include")
include_directories(${OpenCV_INCLUDE_DIRS})

# 库文件路径 (注意：这里指向 lib 目录，不是 bin)
# 请确保 F:/OpenCV/opencv/build/x64/vc15/lib 目录下有 .lib 文件
set(OpenCV_LIB_DIR "F:/OpenCV/opencv/build/x64/vc15/lib")
link_directories(${OpenCV_LIB_DIR})

# ==========================================
# 3. 查找源文件
# ==========================================
file(GLOB_RECURSE DIR_SRCS ${CMAKE_CURRENT_SOURCE_DIR}/*.cpp)
file(GLOB_RECURSE DIR_HEADERS ${CMAKE_CURRENT_SOURCE_DIR}/*.h)
file(GLOB_RECURSE DIR_HEADLESS ${CMAKE_CURRENT_SOURCE_DIR}/*.hpp)
file(GLOB_RECURSE DIR_QRC ${CMAKE_CURRENT_SOURCE_DIR}/*.qrc)

add_executable(${PROJECT_NAME}
    ${DIR_SRCS}
    ${DIR_HEADERS}
    ${DIR_HEADLESS}
    ${DIR_QRC}
)

# ==========================================
# 4. 链接库
# ==========================================
target_link_libraries(${PROJECT_NAME}
    # OpenCV 库：根据 Debug/Release 模式自动选择带 d 或不带 d 的库
    $<$<CONFIG:Debug>:opencv_world460d>
    $<$<CONFIG:Release>:opencv_world460>
    $<$<CONFIG:RelWithDebInfo>:opencv_world460>
    
    # Qt6 组件
    Qt6::Core
    Qt6::Gui
    Qt6::Widgets
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Xml
    Qt6::Concurrent
    
    # 系统库
    opengl32
)