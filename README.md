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
- Tag 发版时自动生成：
  - 普通 Linux x86_64 版本归档
  - 内存检查版 Linux x86_64 归档

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
  - Tag 发版归档普通版与内存检查版 release 附件

## 项目结构

```text
.
├── .github/workflows/
│   ├── ci.yml
│   └── release.yml
├── app/
│   ├── app.pro
│   ├── main.cpp
│   ├── mainwindow.cpp
│   ├── mainwindow.h
│   ├── monitor.cpp
│   └── monitor.h
├── scripts/
│   ├── run_memory_check.sh
│   └── run_valgrind_memcheck.sh
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
  xvfb \
  zip
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

## Release 附件说明

当你 push 一个形如 `v1.0.0` 的 tag 时，会自动触发 `release.yml`，并在 GitHub Release 中上传两个附件：

### 1. 普通版

- 文件名：`qt-linux-resource-monitor-linux-x86_64.zip`
- 内容：
  - 正常编译的主程序
  - 测试程序
  - README
  - 内存检查脚本

适合拿来正常运行和验证基础功能。

### 2. 内存检查版

- 文件名：`qt-linux-resource-monitor-linux-x86_64-memory-check.zip`
- 内容：
  - 开启 `-fsanitize=address,undefined` 编译的主程序
  - 开启 `-fsanitize=address,undefined` 编译的测试程序
  - README
  - 内存检查脚本
  - `MEMORY_CHECK_USAGE.txt`

这个包不是给普通最终用户用的，而是给开发 / 测试 / 质量保障场景用的。

## CI 中的内存检查方案

这个项目同时使用两类方案：

1. **编译期/运行期插桩**：AddressSanitizer + LeakSanitizer + UndefinedBehaviorSanitizer
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

#### 为什么会有 LSan suppression

在 GitHub Actions 的无头 Linux 环境里，Qt 的 `offscreen` 平台插件有时会在进程退出时留下少量框架级分配痕迹，例如 `QScreen` / `QPlatformScreen` / `QtSharedPointer` 相关对象。这类问题通常属于 **Qt 平台插件初始化/退出噪音**，不代表业务代码本身存在泄漏。

因此仓库提供了 `ci/lsan.supp`，只抑制少量已知的 Qt headless 平台插件泄漏栈；项目自身代码路径的泄漏检查仍然保留。


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

## 如何进行内存检查

### 方法一：直接看 CI 结果

最直接的方法就是看 GitHub Actions：

- `ci` workflow 是否通过
- `release` workflow 在打 tag 时是否通过

只要下面任一环节失败，都说明当前版本存在待处理的内存风险或未定义行为风险：

- Sanitizer 测试失败
- Valgrind Memcheck 返回非零退出码
- 测试程序崩溃

### 方法二：使用 release 附件做检查

推荐把两个附件分工看待：

- `qt-linux-resource-monitor-linux-x86_64-memory-check.zip`
  - 用于 **ASan / LSan / UBSan** 检查
- `qt-linux-resource-monitor-linux-x86_64.zip`
  - 用于普通运行验证
  - 如需本地跑 Valgrind，建议对这个包里的测试二进制，或者你本地重新编译的 debug 二进制执行

#### 使用内存检查版运行 Sanitizer 检查

下载 `qt-linux-resource-monitor-linux-x86_64-memory-check.zip` 后，解压进入目录：

```bash
chmod +x scripts/run_memory_check.sh
./scripts/run_memory_check.sh ./tests/tst_monitor_asan
```

如果要直接运行带 ASan 的主程序：

```bash
QT_QPA_PLATFORM=offscreen ./bin/qt_linux_resource_monitor_asan
```

若程序存在非法内存访问、use-after-free、部分泄漏等问题，ASan/LSan/UBSan 通常会：

- 直接打印错误类型
- 输出调用栈
- 让进程以非零状态退出

#### 使用普通版或本地 debug 版运行 Valgrind Memcheck

如果你下载的是普通版附件，或者你本地重新编译了一份带调试符号但未启用 ASan 的二进制，可以执行：

```bash
chmod +x scripts/run_valgrind_memcheck.sh
./scripts/run_valgrind_memcheck.sh ./tests/tst_monitor
```

> 注意：**Valgrind 不建议直接运行在 ASan 二进制上**。通常应当对“带调试符号、但不启用 ASan 的测试二进制”执行 Valgrind。

如果存在确定性泄漏、非法读写、未初始化值传播等问题，Valgrind 会输出详细报告，并因为 `--error-exitcode=101` 导致命令失败。

## 如何确认一个版本“内存没有问题”

严格来说，**不能因为一次检查通过，就数学意义上证明“绝对没有任何内存问题”**。这类工具能做的是：

- 在当前代码版本
- 在当前测试覆盖范围
- 在当前运行路径下

**没有发现已知可检测的内存问题**。

所以更准确的表述应该是：

> 该版本在当前 CI 和测试覆盖下，未发现 Sanitizer / Valgrind 可检测到的内存错误与泄漏问题。

### 实际工程里，通常用下面标准判断

一个版本可以认为“内存检查通过”，通常需要同时满足：

1. `ci` workflow 全绿
2. `release` workflow 全绿
3. Sanitizer 测试无报错、无崩溃、无非零退出
4. Valgrind Memcheck 在普通/debug 测试二进制上无 `definitely lost` / `indirectly lost` / `invalid read` / `invalid write` 等关键错误
5. 关键业务路径已经被测试覆盖到

### 反过来说，下面这些情况说明版本不能算“内存健康”

- ASan 报 `heap-buffer-overflow`
- ASan 报 `use-after-free`
- LSan 报泄漏
- UBSan 报未定义行为
- Valgrind 报 `Invalid read` / `Invalid write`
- Valgrind 报 `definitely lost` 大于 0
- 测试覆盖不足，关键路径根本没执行到

## 为什么测试程序而不是直接在 GUI 主程序上做泄漏检查

CI 中优先对 **测试二进制** 做内存检查，而不是让 GUI 主程序无限运行，原因有三个：

1. 测试程序执行路径稳定，结果更可重复
2. 测试结束时进程自然退出，便于泄漏汇总
3. 无头环境（GitHub Actions）更容易跑通

当前仓库里的 `tests/tst_monitor` 只验证资源采集与解析逻辑，因此专门使用 **app-less Qt Test**（不初始化 `QApplication` / Widgets 平台层）。这样可以显著减少 Qt GUI 平台插件在 headless CI 下带来的噪音，让内存检查更聚焦在业务逻辑本身。

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

### `ci.yml`

执行顺序：

1. 安装 Qt 5.15 构建依赖
2. 常规 `qmake + make`
3. 运行 Qt Test
4. 使用 ASan/UBSan 重新构建
5. 再次运行测试，触发 Sanitizer 检测
6. 使用 Valgrind Memcheck 运行测试

### `release.yml`

当推送 `v*` tag 时执行：

1. 构建普通 Linux x86_64 归档
2. 构建带 Sanitizer 的内存检查版归档
3. 在内存检查版上运行 Sanitizer 测试
4. 重新构建非 ASan 的调试测试二进制
5. 对非 ASan 测试二进制运行 Valgrind Memcheck
6. 把两个 zip 上传到 GitHub Release 附件

只要其中任一步发现问题，工作流就会失败，release 附件也不会被当作“健康版本”发布成功。

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
- 把关键场景的长稳测试也纳入 tag release 前校验

## 适用场景

这个项目尤其适合：

- 想搭一个 **Qt C++ Linux 项目** 的 CI 模板
- 想在 GitHub Actions 里跑 **QMake + Qt Test + 内存检查**
- 想在发版时同时生成 **普通版 + 内存检查版** 附件
- 想系统化验证 **内存采样、内存泄漏、非法访问等常见问题**

---

如果你是要把这套方案迁移到自己的老项目，优先保留的核心思路是：

- **测试可自动运行**
- **Sanitizer 作为快速主检查**
- **Valgrind 作为补充检查**
- **tag 发版时产出 normal / memory-check 两类附件**
- **CI 失败即阻断问题合入和带病发布**
