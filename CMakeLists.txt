cmake_minimum_required(VERSION 3.10)
project(MalaysianOsint C)

# ================================================================
# 设置 C 标准
# ================================================================
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# verbose 选项（make 构建时显示命令）
set(CMAKE_VERBOSE_MAKEFILE OFF)

# ================================================================
# 打印块状 Logo
# ================================================================
message("\n")
message("███╗   ███╗██╗   ██╗ ██████╗ ███████╗██╗███╗   ██╗████████╗")
message("████╗ ████║╚██╗ ██╔╝██╔═══██╗██╔════╝██║████╗  ██║╚══██╔══╝")
message("██╔████╔██║ ╚████╔╝ ██║   ██║███████╗██║██╔██╗ ██║   ██║   ")
message("██║╚██╔╝██║  ╚██╔╝  ██║   ██║╚════██║██║██║╚██╗██║   ██║   ")
message("██║ ╚═╝ ██║   ██║   ╚██████╔╝███████║██║██║ ╚████║   ██║   ")
message("╚═╝     ╚═╝   ╚═╝    ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝   ╚═╝   ")
message("\n")

# ================================================================
# 包含头文件目录
# ================================================================
include_directories(${CMAKE_SOURCE_DIR}/include)

# ================================================================
# 源码文件
# ================================================================
set(SOURCES
    src/pdrm.c
    src/sspi.c
    src/mykad.c
    src/rmp_wanted.c
    src/ecourt.c
    src/ssm.c
    src/sprm.c
    src/company.c
    src/social.c
    src/bnm.c
    my_osint.c
)

# ================================================================
# 查找依赖库
# ================================================================
find_package(PkgConfig REQUIRED)

# libcurl
pkg_check_modules(LIBCURL REQUIRED libcurl)
# libmicrohttpd
pkg_check_modules(MICROHTTPD REQUIRED libmicrohttpd)
# libcjson
pkg_check_modules(CJSON REQUIRED libcjson)

# 添加库的头文件目录
include_directories(
    ${LIBCURL_INCLUDE_DIRS}
    ${MICROHTTPD_INCLUDE_DIRS}
    ${CJSON_INCLUDE_DIRS}
)

# 添加库的链接目录
link_directories(
    ${LIBCURL_LIBRARY_DIRS}
    ${MICROHTTPD_LIBRARY_DIRS}
    ${CJSON_LIBRARY_DIRS}
)

# ================================================================
# 创建可执行文件
# ================================================================
add_executable(mo ${SOURCES})

# 链接库
target_link_libraries(mo
    ${LIBCURL_LIBRARIES}
    ${MICROHTTPD_LIBRARIES}
    ${CJSON_LIBRARIES}
)

# ================================================================
# 安装规则
# ================================================================
install(TARGETS mo DESTINATION bin)
install(DIRECTORY include/ DESTINATION include)

# ================================================================
# 构建信息输出（中文简洁美化）
# ================================================================
if(LIBCURL_FOUND)
    set(LIBCURL_VER ${LIBCURL_VERSION})
else()
    set(LIBCURL_VER "未找到")
endif()

if(MICROHTTPD_FOUND)
    set(MICROHTTPD_VER "已找到")
else()
    set(MICROHTTPD_VER "未找到")
endif()

if(CJSON_FOUND)
    set(CJSON_VER ${CJSON_VERSION})
else()
    set(CJSON_VER "未找到")
endif()

message("\n================================================")
message("-- 项目信息")
message("-- 项目名称      : ${PROJECT_NAME}")
message("-- 平台          : ${CMAKE_SYSTEM_NAME}")
message("-- 架构          : ${CMAKE_SYSTEM_PROCESSOR}")
message("-- 构建类型      : ${CMAKE_BUILD_TYPE}")
message("-- 编译器        : ${CMAKE_C_COMPILER_ID}")
message("-- C 标准        : C${CMAKE_C_STANDARD}")
message("-- 安装路径      : ${CMAKE_INSTALL_PREFIX}")
message("--")
message("-- 依赖库信息")
message("-- libcurl        : ${LIBCURL_VER}")
message("-- libmicrohttpd  : ${MICROHTTPD_VER}")
message("-- libcjson       : ${CJSON_VER}")
message("================================================\n")

# 构建模式提示
if(CMAKE_BUILD_TYPE MATCHES "Debug")
    message("🐼 当前模式      : DEBUG（详细日志开启）\n")
else()
    message("✨ 当前模式      : ${CMAKE_BUILD_TYPE}\n")
endif()

# 成功提示
message("🎉 配置完成！可以运行 `make run` 构建项目\n")
message("================================================\n")
message("现在运行 make run 来执行应用程序\n")
