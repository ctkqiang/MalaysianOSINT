# 马来西亚 OSINT 半自动查询器 (Malaysian OSINT)

## 项目简介

这是一个用于反诈骗和追踪骗子的半自动化查询工具。本工具通过与马来西亚皇家警察（PDRM）的 Semak Mule API 进行交互，帮助识别和防范涉及银行账户的非法资金转移活动。

**重要提示：** 根据马来西亚法律，严禁将本工具用于恶意用途。本工具仅供反诈骗研究与教育使用。

## 技术要求

### 系统依赖
- CMake (>= 3.10)
- C编译器（支持C11标准）
- libcurl
- libmicrohttpd
- libcjson

### 基础设施
- 支持 FreeBSD 14.0 及以上版本
- Docker 支持
- HTTP服务运行在 8080 端口

## 安装设置

### 方法一：本地编译

```bash
# 克隆项目
git clone https://github.com/ctkqiang/MalaysianOSINT.git
cd MalaysianOSINT

# 创建构建目录并编译
mkdir -p build
cd build
cmake ..
make
```

### 方法二：Docker 部署

```bash
# 从 Docker Hub 拉取镜像
docker pull ctkqiang/malaysian-osint:latest

# 或者本地构建镜像
docker build -t malaysian-osint .

# 运行容器
docker run -p 8080:8080 malaysian-osint
```

## 使用说明

启动服务后，终端会显示如下信息：

```plaintext
============================================================
   马来西亚 OSINT 半自动查询器 (Malaysian OSINT)
============================================================
 作者: 钟智强 <johnmelodymel@qq.com>
 版本: v0.1.0
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
   2. 在 URL 后添加查询参数 ?q=电话号码 或 银行账号
      例如: http://localhost:8080/?q=0123456789
   3. 输入 'q' 并回车以安全关闭服务
============================================================
```

### API 使用示例

```bash
# 使用 curl 查询
curl "http://localhost:8080/?q=0123456789"
```

## 贡献指南

欢迎提交 Pull Requests 和 Issues。如果您是执法部门或政府机构，我们非常欢迎合作。

## 许可证

本项目采用专有许可证。未经授权，禁止用于商业用途。

## 联系方式

作者：钟智强  
邮箱：johnmelodymel@qq.com
微信：ctkqiang