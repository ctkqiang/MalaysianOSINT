# 马来西亚 OSINT 半自动查询器 (Malaysian OSINT)

## 项目简介

马来西亚 OSINT 半自动查询器是一个 **基于 C 语言** 的轻量级 HTTP 服务，专注于反诈骗、社会工程学研究和执法辅助场景。
它通过对接 **马来西亚皇家警察 (PDRM) Semak Mule API**、**移民局 SSPI 系统** 和 **马来西亚法庭记录 (E-Court)**，实现对电话号码、银行账户、身份证号码以及法庭案件信息的半自动化查询。

本工具适合：

* 网络安全与反诈骗研究人员
* 学术和教育研究
* 执法机构辅助工具

⚠️ **法律声明**：
根据马来西亚法律，禁止将本工具用于任何恶意用途。
作者不对任何滥用行为承担责任。本项目仅供 **教育、研究与反诈骗场景** 使用。

---
<video src="https://github.com/ctkqiang/MalaysianOSINT/raw/assets/studio_video_1757091769573608.MP4" controls width="800" style="max-width:100%;">
  您的浏览器不支持视频播放
</video>

---

## 功能特性

* **PDRM Semak Mule 接口集成**
  查询电话号码或银行账户是否与诈骗案件关联，并返回 PDRM 报告数据。

* **移民局 SSPI 查询**
  输入身份证号码，检测是否存在 `Tiada Halangan`（无禁止）状态，同时返回 MyKad 详细信息。

* **马来西亚法庭记录 (E-Court) 查询**
  查询案件信息、法官、日期等，支持多种筛选条件。

* **马来西亚皇家警察通缉名单核查**
  通过身份证号码查询是否在通缉名单中，并返回详细信息。

* **马来西亚反贪污委员会 (SPRM) 记录查询**
  检查是否存在贪污犯罪记录，包含案件详情和判决信息。

* **公司注册信息查询 (SSM)**
  查询公司注册号的实体类型和基本信息。

* **公司详细信息搜索**
  通过公司名称搜索，返回包含地址、网站、分类等详细信息。

* **社交媒体用户名搜索**
  跨平台搜索社交媒体账号信息。

* **内置 HTTP 服务**
  基于 **libmicrohttpd**，无需额外依赖即可启动 REST 风格接口。

* **JSON 格式化输出**
  使用 **cJSON** 提供结构化结果，方便二次开发或前端对接。

* **访问日志记录**
  详细记录每次查询的时间、IP、请求方法和路径。

* **跨平台支持**
  支持 FreeBSD、Linux、macOS，Docker 镜像即开即用。

---

## 系统依赖

* **构建工具**

  * CMake (>= 3.10)
  * GCC 或 Clang (支持 C11)

* **系统库**

  * libcurl (HTTP客户端)
  * libmicrohttpd (HTTP服务器)
  * libcjson (JSON解析)
  * libsqlite3 (本地缓存)
  * libxml2 (XML解析)
  * OpenSSL (加密通信)

* **运行环境**

  * FreeBSD 14+ / Linux / macOS
  * 或使用 Docker

---

## 安装与编译

### 方法一：源码编译

在不同系统上安装依赖：

```bash
# Ubuntu/Debian
sudo apt install cmake gcc libmicrohttpd-dev libcjson-dev libcurl4-openssl-dev libssl-dev libsqlite3-dev libxml2-dev

# macOS
brew install cmake gcc libmicrohttpd cjson curl openssl sqlite libxml2

# FreeBSD
pkg install cmake gcc libmicrohttpd cjson curl openssl sqlite3 libxml2
```

构建项目：

```bash
# 克隆仓库
git clone https://github.com/ctkqiang/MalaysianOSINT.git
cd MalaysianOSINT

# 构建
mkdir -p build && cd build
cmake ..
make
```

启动服务：

```bash
./mo
```

或

```bash
make run
```

---

### 方法二：Docker 运行

```bash
# 拉取镜像
docker pull ctkqiang/malaysianosint:latest

# 或本地构建
docker build -t malaysianosint .

# 前台运行（方便查看日志）
docker run -p 8080:8080 malaysianosint

# 后台运行
docker run -d -p 8080:8080 malaysianosint

# 指定配置文件运行
docker run -d -p 8080:8080 -v /path/to/config.json:/app/config.json malaysianosint
```

---

## 启动成功后终端显示

```plaintext
███╗   ███╗██╗   ██╗ ██████╗ ███████╗██╗███╗   ██╗████████╗
████╗ ████║╚██╗ ██╔╝██╔═══██╗██╔════╝██║████╗  ██║╚══██╔══╝
██╔████╔██║ ╚████╔╝ ██║   ██║███████╗██║██╔██╗ ██║   ██║   
██║╚██╔╝██║  ╚██╔╝  ██║   ██║╚════██║██║██║╚██╗██║   ██║   
██║ ╚═╝ ██║   ██║   ╚██████╔╝███████║██║██║ ╚████║   ██║   
╚═╝     ╚═╝   ╚═╝    ╚═════╝ ╚══════╝╚═╝╚═╝  ╚═══╝   ╚═╝   
马来西亚 OSINT 半自动查询系统
==================================================
           Malaysian OSINT Query System
==================================================
👨💻 作者: 钟智强 <johnmelodymel@qq.com>
🔖 版本: v1.0.0
==================================================
⚠️  法律声明: 根据1997年电脑犯罪法令第5条文
   仅限授权安全研究及反诈骗调查使用
   严禁非法用途，违者必究
==================================================
🤝 政府合作: PDRM/MCMC/移民局欢迎技术合作
==================================================
🟢 服务状态: 已启动
🌐 服务地址: http://localhost:8080
==================================================
📡 查询接口:
==================================================
1. 马来西亚皇家警察(PDRM)反钱驴检查系统 (Semak Mule)
   http://localhost:8080/?q=手机号/银行账号
--------------------------------------------------
2. 移民局身份证信息查询 (SSPI)
   http://localhost:8080/?id=身份证号码
--------------------------------------------------
3. 马来西亚法庭记录查询 (E-Court)
   http://localhost:8080/?name=姓名
--------------------------------------------------
4. 马来西亚皇家警察(PDRM)通缉名单核查 (PDRM Wanted List)
   http://localhost:8080/?wanted=身份证号
--------------------------------------------------
5. 马来西亚反贪污委员会(SPRM)记录查询
   http://localhost:8080/?sprm=身份证号码
--------------------------------------------------
6. 马来西亚公司注册资料查询 (SSM)
   http://localhost:8080/?ssm=注册号
--------------------------------------------------
7. 公司详细信息搜索
   http://localhost:8080/?comp=公司名称
--------------------------------------------------
8. 社交媒体账号搜索
   http://localhost:8080/?social=用户名
==================================================
⌨️  操作指令:
   q → 安全关闭    r → 重新加载
   h → 帮助信息
==================================================
💡 提示: 使用浏览器或 curl 访问上述接口进行查询
📝 日志: 所有查询将记录在 ./logs/access.log
==================================================
```

---

## API 示例

### 查询电话号码 / 银行账户 (PDRM)

```bash
curl "http://localhost:8080/?q=0123456789"
```

返回示例：

```json
{
  "count": 5,
  "cases": [
    {
      "bank": "Maybank",
      "account": "1234567890",
      "reported": 3
    }
  ]
}
```

### 查询身份证 (SSPI + MyKad + PDRM + SPRM)

```bash
curl "http://localhost:8080/?id=IC_NUMBER"
```

返回示例：

```plaintext
IC: 123456789012
SSPI Status: Tiada Halangan
MyKad Info: {"name":"张三","address":"...","status":"active"}
Wanted: No
SPRM Pesalah: No
```

如果是通缉犯，将返回额外信息：

```plaintext
Wanted Person Details:
Name: 张三
Age: 35
Photo: https://example.com/photo.jpg
```

如果有贪污记录，将返回详细信息：

```plaintext
SPRM Pesalah Details:
Name: 张三
IC: 123456789012
Employer: XX公司
Position: 经理
Case: SPRM-2023-001
Law: 反贪污法第xx条
Sentence: 监禁2年
```

### 查询法庭记录 (E-Court)

```bash
curl "http://localhost:8080/?name=张三"
```

返回示例：

```json
{
  "total_results": 2,
  "cases": [
    {
      "case_number": "KA1234",
      "court": "Kuala Lumpur High Court",
      "judge": "Tan Sri Ahmad",
      "date_of_ap": "2025-01-15"
    }
  ]
}
```

### 公司注册资料查询 (SSM)

```bash
curl "http://localhost:8080/?ssm=202001012345"
```

返回示例：

```json
{
  "ssm_number": "202001012345",
  "entity_type": "Private Limited Company"
}
```

### 公司详细信息搜索

```bash
curl "http://localhost:8080/?comp=公司名称"
```

返回示例：

```json
[
  {
    "name": "XX科技有限公司",
    "category": "科技服务",
    "address": "吉隆坡市中心...",
    "website": "https://example.com",
    "source": "SSM数据库"
  }
]
```

### 社交媒体用户名搜索

```bash
curl "http://localhost:8080/?social=username"
```

返回示例：

```plaintext
正在搜索用户名: username
发现账号:
Facebook: https://facebook.com/username
Twitter: https://twitter.com/username
Instagram: https://instagram.com/username
...
```

---

## 开发指南

### 目录结构

```plaintext
.
├── src/                      # 源代码目录
│   ├── main.c                # 主程序入口
│   ├── http.c                # HTTP服务器实现
│   ├── logger.c              # 日志记录模块
│   └── query/                # 查询模块目录
│       ├── pdrm.c           # PDRM反钱驴查询
│       ├── sspi.c           # 移民局身份查询
│       ├── mykad.c          # MyKad信息查询
│       ├── ecourt.c         # 法庭记录查询
│       ├── wanted.c         # 通缉犯查询
│       ├── sprm.c           # 反贪污记录查询
│       ├── ssm.c            # 公司注册查询
│       ├── company.c        # 公司信息搜索
│       └── social.c         # 社交媒体搜索
├── include/                  # 头文件目录
│   ├── osint.h              # 主头文件
│   ├── http.h               # HTTP相关定义
│   ├── logger.h             # 日志相关定义
│   └── query/               # 查询模块头文件
│       └── *.h              # 各查询模块头文件
├── tests/                    # 测试目录
│   ├── test_http.c          # HTTP测试
│   ├── test_logger.c        # 日志测试
│   └── test_query.c         # 查询模块测试
├── docs/                     # 文档目录
│   └── api.md               # API文档
├── scripts/                  # 脚本目录
│   └── install_deps.sh      # 依赖安装脚本
└── logs/                     # 日志目录
    └── access.log           # 访问日志
```

### 开发规范

1. 代码风格
   - 使用4空格缩进
   - 函数名使用小写字母和下划线
   - 常量使用大写字母和下划线
   - 每行不超过80个字符

2. 提交规范
   - 提交信息使用中文
   - 格式：`[模块] 修改内容`
   - 例如：`[查询] 新增社交媒体搜索功能`

3. 文档规范
   - 新功能必须更新 README.md
   - 新接口必须更新 API 文档
   - 注释使用中文

### 编译和测试

```bash
# 编译项目
make run

# 运行所有测试
make test

```

## 贡献指南

1. Fork 项目到自己的账号
2. 创建功能分支：`git checkout -b feature/xxx`
3. 提交改动：`git commit -am '[xxx] 添加xxx功能'`
4. 推送分支：`git push origin feature/xxx`
5. 提交 Pull Request

## 许可证

本项目采用 **专有许可证**：

* 禁止商业用途
* 禁止未授权的生产部署
* 允许学术与研究用途

## 联系方式

- 作者：钟智强
- 邮箱：johnmelodymel@qq.com
- GitHub：@ctkqiang
- 微信：ctkqiang

## 致谢
- 马来西亚皇家警察总部 (PDRM)
- 马来西亚反贪污委员会 (SPRM)
- 马来西亚公司注册局 (SSM)
- 马来西亚移民局

