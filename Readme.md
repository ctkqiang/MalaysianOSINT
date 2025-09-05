# 马来西亚 OSINT 半自动查询器 (Malaysian OSINT)

## 项目简介

马来西亚 OSINT 半自动查询器是一个 **基于 C 语言** 的轻量级 HTTP 服务，专注于反诈骗和社会工程学研究场景。
它通过对接 **马来西亚皇家警察（PDRM）Semak Mule API** 和 **马来西亚移民局 SSPI 系统**，实现对可疑电话号码、银行账户和身份证号码的半自动化查询。

本工具特别适合：

* 反诈骗研究人员
* 网络安全从业者
* 教育和学术研究
* 执法机构辅助工具

⚠️ **法律声明**：
根据马来西亚法律，禁止将本工具用于任何恶意用途。
作者不对任何滥用行为承担责任。本项目仅供 **教育、研究与反诈骗场景** 使用。

---

## 功能特性

* **Semak Mule 接口集成**
  查询电话号码或银行账户是否与诈骗案件关联，并返回 PDRM 报告数据。

* **SSPI 移民局查询**
  输入身份证号码，检测是否存在 `Tiada Halangan`（无禁止）状态。

* **内置 HTTP 服务**
  基于 **libmicrohttpd**，无需外部依赖即可启动 REST 风格接口。

* **JSON 格式化输出**
  使用 **cJSON** 提供结构化的结果，方便二次开发或接入前端。

* **跨平台支持**
  支持 FreeBSD、Linux、macOS，Docker 镜像即开即用。

---

## 系统依赖

在编译或运行前，确保系统已安装以下依赖：

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

启动：

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

## 使用说明

启动成功后，终端会显示：

```plaintext
============================================================
   马来西亚 OSINT 半自动查询器 (Malaysian OSINT)
============================================================
 作者: 钟智强 <johnmelodymel@qq.com>
 版本: v0.1.0 (built on 2025-09-05 22:00)
------------------------------------------------------------
 [警告] 根据马来西亚法律，禁止将本工具用于恶意用途。
        作者不对任何滥用行为承担责任。
        本工具仅供反诈骗、追踪骗子研究与教育使用。
------------------------------------------------------------
 Note: If you are PDRM or a government entity seeking
       solutions like this, we are open for collaboration.
============================================================
 服务已成功启动。
 运行地址:   http://localhost:8080
 使用说明:
   1. 在浏览器或命令行工具中访问上述地址
   2. 在 URL 后添加查询参数
      - 电话/银行:   ?q=0123456789
      - 身份证:       ?id=REMOVED_SENSITIVE_DATA
   3. 输入 'q' 并回车以安全关闭服务
============================================================
```

---

## API 示例

### 查询电话号码 / 银行账户

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
curl "http://localhost:8080/?id=REMOVED_SENSITIVE_DATA"
```

返回示例：

```plaintext
IC is clear: Tiada Halangan
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
│   └── ...
├── include/           # 头文件
├── test/              # 单元测试
├── CMakeLists.txt
└── README.md
```

### 编译单元测试

```bash
gcc -Wall -I./include \
    -o test/test_osint test/test_osint.c src/pdrm.c src/sspi.c \
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

