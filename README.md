# qt-linux-resource-monitor

基于 **Qt 5.15 + C++ + QMake** 的 Linux 系统资源监控示例项目，提供一个 `QWidget` 图形界面，并重点演示：

- GitHub Actions 自动构建
- Qt 单元测试自动运行
- CI 中的常见内存问题检查
  - 内存泄漏（memory leak）
  - 非法访问
  - 越界访问
  - use-after-free
  - 未定义行为导致的潜在内存错误

仓库目标不是做一个功能庞杂的监控器，而是做一个 **适合 Qt C++ Linux 项目直接复用的 CI + 内存检查模板**。

## 功能概览

当前实现包含：

- `QWidget` 主窗口
- 每秒刷新一次系统资源信息
- 基于 Linux `/proc` 文件系统采集：
  - CPU 使用率：`/proc/stat`
  - 系统内存：`/proc/meminfo`
  - 当前进程内存：`/proc/self/status`
- Qt Test 单元测试
- GitHub Actions 自动执行：
  - 常规构建
  - 单元测试
  - Sanitizer 构建与测试
  - Valgrind Memcheck 检查

## 项目结构

```text
.
├── .github/workflows/ci.yml
├── app/
│   ├── app.pro
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── mainwindow.h
│   ├── monitor.cpp
│   └── monitor.h
├── tests/
│   ├── test_monitor.cpp
│   └── tests.pro
└── qt-linux-resource-monitor.pro
```

## 本地构建

### 依赖

Ubuntu / Debian 示例：

```bash
sudo apt-get update
sudo apt-get install -y \
  qtbase5-dev \
  qtbase5-dev-tools \
  qtchooser \
  qt5-qmake \
  g++ \
  valgrind \
  xvfb
```

### 构建

```bash
qmake qt-linux-resource-monitor.pro
make -j$(nproc)
```

### 运行程序

```bash
./app/qt_linux_resource_monitor
```

### 运行测试

```bash
./tests/tst_monitor -txt
```

如果是在无桌面环境（如 CI / 服务器）下运行 Qt 测试：

```bash
QT_QPA_PLATFORM=offscreen xvfb-run -a ./tests/tst_monitor -txt
```

## CI 中的内存检查方案

这个项目同时使用两类方案：

1. **编译期/运行期插桩**：AddressSanitizer + UndefinedBehaviorSanitizer
2. **动态二进制内存检查**：Valgrind Memcheck

这样做的原因很简单：**两类工具互补**。

- Sanitizer 速度更快，适合每次提交都跑
- Valgrind 更偏“保守型体检”，尤其适合发现泄漏和非法内存使用的上下文

### 1) AddressSanitizer / LeakSanitizer / UndefinedBehaviorSanitizer

CI 中会额外做一次带 Sanitizer 的构建：

```bash
qmake \
  QMAKE_CXXFLAGS+="-O1 -g -fno-omit-frame-pointer -fsanitize=address,undefined" \
  QMAKE_LFLAGS+="-fsanitize=address,undefined" \
  qt-linux-resource-monitor.pro
make -j$(nproc)
```

#### 实现原理

Sanitizer 的核心不是“事后分析日志”，而是 **编译阶段插桩（instrumentation）**：

- 编译器在生成目标代码时，把额外的检测逻辑插入到内存读写、栈对象、堆对象和部分未定义行为相关的位置
- 程序运行时，这些检测逻辑会配合运行库判断当前访问是否合法
- 一旦发现问题，进程直接报错退出，并输出堆栈信息

#### 能抓到什么

AddressSanitizer（ASan）常见能抓到：

- heap-buffer-overflow
- stack-buffer-overflow
- use-after-free
- double free
- wild pointer / 非法地址访问
- 部分栈生命周期问题

LeakSanitizer（LSan，通常集成在 ASan 运行时中）常见能抓到：

- 堆内存泄漏
- 测试结束后仍然不可达、未释放的分配块

UndefinedBehaviorSanitizer（UBSan）常见能抓到：

- 整数溢出中的一部分问题
- 非法类型转换
- 对齐错误
- 其他未定义行为

#### 为什么 `-fno-omit-frame-pointer`

它能让崩溃/泄漏堆栈更稳定，更利于 CI 日志定位具体源码位置。

---

### 2) Valgrind Memcheck

CI 中还会执行：

```bash
valgrind \
  --tool=memcheck \
  --leak-check=full \
  --show-leak-kinds=all \
  --track-origins=yes \
  --error-exitcode=101 \
  ./tests/tst_monitor -txt
```

#### 实现原理

Valgrind 不依赖编译器插桩，它的主要思路是：

- 在程序运行时，接管并翻译机器指令
- 为内存字节和寄存器状态维护“有效性/可达性”影子信息
- 跟踪每次分配、释放、读写和未初始化值传播
- 进程退出时分析哪些堆对象仍然存活、是否存在不可达块、是否存在非法访问

#### 能抓到什么

Memcheck 常见能抓到：

- definitely lost / indirectly lost / possibly lost 泄漏
- invalid read / invalid write
- use of uninitialised value
- mismatched free / delete
- 重复释放
- 越界访问的部分情况

#### 为什么 Sanitizer 之外还要跑 Valgrind

因为两者的检测机制不同：

- **Sanitizer 更快**，适合成为主力 CI 检查
- **Valgrind 更细**，尤其在泄漏归因、未初始化值传播分析上很有帮助

在 Qt/C++ Linux 项目里，这种组合非常实用。

## 为什么测试程序而不是直接在 GUI 主程序上做泄漏检查

CI 中优先对 **测试二进制** 做内存检查，而不是让 GUI 主程序无限运行，原因有三个：

1. 测试程序执行路径稳定，结果更可重复
2. 测试结束时进程自然退出，便于泄漏汇总
3. 无头环境（GitHub Actions）更容易跑通

也就是说，**内存检查的关键不是“有没有窗口”，而是“是否有可重复、可自动化的执行路径”**。

## Linux 资源采集实现说明

程序自身展示的资源监控主要基于 Linux `/proc`：

### CPU

读取 `/proc/stat` 第一行：

- `user`
- `nice`
- `system`
- `idle`
- `iowait`
- ...

通过两次采样做差：

- `totalDelta = current.total - previous.total`
- `idleDelta = current.idle - previous.idle`
- `cpuUsage = (totalDelta - idleDelta) / totalDelta`

### 系统内存

读取 `/proc/meminfo` 中：

- `MemTotal`
- `MemAvailable`

计算：

- `used = MemTotal - MemAvailable`
- `usagePercent = used / MemTotal`

### 当前进程内存

读取 `/proc/self/status` 中：

- `VmRSS`：驻留集大小，接近进程真实物理内存占用
- `VmSize`：虚拟内存空间大小

## GitHub Actions 流程

`ci.yml` 执行顺序：

1. 安装 Qt 5.15 构建依赖
2. 常规 `qmake + make`
3. 运行 Qt Test
4. 使用 ASan/UBSan 重新构建
5. 再次运行测试，触发 Sanitizer 检测
6. 使用 Valgrind Memcheck 运行测试

只要其中任一步发现问题，工作流就会失败。

## 后续可扩展方向

如果你想把它做成更完整的工程，接下来可以继续加：

- 磁盘使用率监控
- 网络吞吐速率监控
- 多核 CPU 曲线图
- 进程列表与 Top N 内存排序
- 导出日志 / CSV
- 更复杂的 Qt GUI 自动化测试
- 针对长期运行场景的 soak test / stress test
- 自定义 Valgrind suppression 文件，降低第三方库噪音

## 适用场景

这个项目尤其适合：

- 想搭一个 **Qt C++ Linux 项目** 的 CI 模板
- 想在 GitHub Actions 里跑 **QMake + Qt Test + 内存检查**
- 想系统化验证 **内存采样、内存泄漏、非法访问等常见问题**

---

如果你是要把这套方案迁移到自己的老项目，优先保留的核心思路是：

- **测试可自动运行**
- **Sanitizer 作为快速主检查**
- **Valgrind 作为补充检查**
- **CI 失败即阻断问题合入**
