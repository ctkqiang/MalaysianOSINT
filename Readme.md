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
  输入身份证号码，检测是否存在 `Tiada Halangan`（无禁止）状态。

* **马来西亚法庭记录 (E-Court) 查询**
  查询案件信息、法官、日期等。

* **内置 HTTP 服务**
  基于 **libmicrohttpd**，无需额外依赖即可启动 REST 风格接口。

* **JSON 格式化输出**
  使用 **cJSON** 提供结构化结果，方便二次开发或前端对接。

* **跨平台支持**
  支持 FreeBSD、Linux、macOS，Docker 镜像即开即用。

---

## 系统依赖

* **构建工具**

  * CMake (>= 3.10)
  * GCC 或 Clang (支持 C11)

* **系统库**

  * libcurl
  * libmicrohttpd
  * libcjson

* **运行环境**

  * FreeBSD 14+ / Linux / macOS
  * 或使用 Docker

---

## 安装与编译

### 方法一：源码编译

```bash
# 克隆仓库
git clone https://github.com/ctkqiang/MalaysianOSINT.git
cd MalaysianOSINT

# 安装依赖 (以 Ubuntu 为例)
sudo apt install cmake gcc libmicrohttpd-dev libcjson-dev libcurl4-openssl-dev

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
docker pull ctkqiang/malaysian-osint:latest

# 或本地构建
docker build -t malaysian-osint .

# 启动服务
docker run -p 8080:8080 malaysian-osint
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
🔖 版本: v0.1.0 (编译: 2025-09-05 22:00)
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
   http://localhost:8080/?q=0123456789
--------------------------------------------------
2. 移民局身份证信息查询 (SSPI)
   http://localhost:8080/?id=1234567890
--------------------------------------------------
3. 马来西亚法庭记录查询 (E-Court)
   http://localhost:8080/?name=姓名
--------------------------------------------------
4. 马来西亚皇家警察(PDRM)通缉名单核查 (PDRM Wanted List)
   http://localhost:8080/?wanted=身份证号
--------------------------------------------------
5. 马来西亚公司注册资料查询 (SSM)
   http://localhost:8080/?ssm=202001012345
==================================================
⌨️  操作指令:
   q → 安全关闭    r → 重新加载
   h → 帮助信息
==================================================
💡 提示: 使用浏览器或 curl 访问上述接口进行查询
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

### 查询身份证 (SSPI)

```bash
curl "http://localhost:8080/?id=IC_NUUMBER"
```

返回示例：

```plaintext
IC is clear: Tiada Halangan
```

### 查询法庭记录 (E-Court)

```bash
curl "http://localhost:8080/?name=JOHN_DOE"
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

### 通缉名单核查 (PDRM Wanted List)

```bash
curl "http://localhost:8080/?wanted=IC_NUUMBER"
```

返回示例：

```plaintext
Wanted: Yes
Name: XXX
Age: XX
Photo: <URL>
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

---

## 开发指南

### 目录结构

```plaintext
.
├── src/
│   ├── my_osint.c     # 主入口
│   ├── pdrm.c         # Semak Mule 查询逻辑
│   ├── sspi.c         # SSPI 查询逻辑
│   ├── ecourt.c       # 法庭记录查询逻辑
│   └── ...
├── include/           # 头文件
├── test/              # 单元测试
├── CMakeLists.txt
└── README.md
```

### 编译单元测试

```bash
gcc -Wall -I./include \
    -o test/test_osint test/test_osint.c src/pdrm.c src/sspi.c src/ecourt.c \
    -lmicrohttpd -lcjson -lcurl
```

---

## 贡献指南

* 提交 Pull Request 前请运行 `clang-format` 保持代码风格一致
* Issue 请附上日志和复现步骤
* 欢迎安全研究员、执法部门和政府单位合作

---

## 许可证

本项目采用 **专有许可证**：

* 禁止商业用途
* 禁止未授权的生产部署
* 允许学术与研究用途

---

## 联系方式

作者：钟智强
📧 邮箱：[johnmelodymel@qq.com](mailto:johnmelodymel@qq.com)
💬 微信：ctkqiang

