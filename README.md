# XCPlite

## 什么是 XCP？

XCP（Universal Measurement and Calibration Protocol）是汽车行业常用的测量和参数调整（标定）协议（ASAM 标准）。它支持通过各种传输协议进行实时信号采集和参数调整，对目标系统的影响极小。

**XCP 新手？** 请参阅 [XCP 详细介绍](docs/XCP_INTRODUCTION.md) 或访问：
- [Vector XCP 书籍](https://www.vector.com/int/en/know-how/protocols/xcp-measurement-and-calibration-protocol/xcp-book#)
- [Vector 虚拟学院在线学习](https://elearning.vector.com/)

## 关于 XCPlite

XCPlite 将 XCP 的应用场景从传统的嵌入式微控制器扩展到了**现代多核微处理器**和运行 POSIX 兼容操作系统（Linux, QNX）或实时操作系统（RTOS，如 ThreadX）的 SoC。

XCPlite 专为 **以太网传输**（支持巨型帧的 TCP/UDP）设计，解决了具有真正并行性和多线程特性的系统中的测量和标定挑战：

- **线程安全且无锁** - 跨多核的一致数据采集和参数修改
- **内存安全** - 测量和标定任何存储位置的变量：栈、堆、线程局部变量和全局变量
- **运行时 A2L 生成** - 以代码形式定义事件、参数和元数据；自动生成并上传 A2L 描述文件
- **复杂类型支持** - 处理基本类型、结构体、数组和嵌套结构
- **标定段（Calibration Segments）** - 支持页面切换、一致的原子修改和参数持久化（冻结）
- **PTP 时间戳** - 为高精度 PTP 同步时间戳做好准备

API 提供了仪表化宏，供开发人员定义测量点、标定参数和元数据。  
无锁实现确保了线程安全和数据一致性，没有阻塞延迟，即使在多核系统上的高争用情况下也是如此。

XCPlite 针对 64 位架构进行了优化，兼容 32 位平台。需要 C11（C++ 支持需 C++20）。它是 [XCP-Lite Rust](https://github.com/vectorgrp/xcp-lite) 的 C 库基础。

**其他 XCP 实现：**
- **XCPbasic** - 面向小型微控制器（8 位以上）的免费实现，针对 CAN 进行了优化
- **XCPprof** - Vector AUTOSAR MICROSAR 和 CANbedded 产品组合中的商业产品


## 快速开始

### 示例

[examples](examples/) 文件夹中提供了演示不同功能的多个示例。

**从这里开始：**
- `hello_xcp` - C 语言中的基本 XCP 服务器设置和仪表化
- `hello_xcp_cpp` - C++ 中的基本仪表化
- `xcp_slave_demo` - **新增**：基于 hello_xcp 的从机示例，包含对应的 Python 主机脚本

**高级示例：**
- `c_demo` - 复杂数据对象、标定对象和页面切换
- `cpp_demo` - C++ 类仪表化和 RAII 包装器
- `struct_demo` - 嵌套结构体和多维数组
- `multi_thread_demo` - 线程间的多线程测量和参数共享
- `point_cloud_demo` - 在 CANape 3D 场景窗口中可视化动态数据结构
- `ptp_demo` - 带有 XCP 仪表化的 PTP 观察者和 PTP 时间服务器
- `no_a2l_demo` - 无需运行时 A2L 生成的工作流（实验性）
- `bpf_demo` - 基于 eBPF 的系统调用跟踪（实验性）

有关每个示例的详细信息以及如何设置 CANape 项目，请参阅 [示例文档](examples/README.md)。

**需求：**

XCPlite 示例旨在展示高级 XCP 功能，并已通过 **CANape 23+**（提供免费演示版）测试。  
这些示例利用了：

- **运行时 A2L 上传** - 无需手动管理 A2L 文件
- **A2L TYPEDEFs** - 具有可重用类型定义的复杂数据结构
- **地址扩展** - 支持相对寻址和多个内存空间
- **Typedef 中的共享轴** - 高级标定结构（CANape 24+，参见 `cpp_demo`）

这些功能为现代多核 HPC 应用程序实现了高效的工作流。虽然 XCPlite 符合 XCP 标准并适用于任何 XCP 工具，但这些示例充分利用了 CANape 对动态系统和高级 A2L 功能的支持。

**下载：** [CANape 演示版](https://www.vector.com/de/de/support-downloads/download-center)

### 构建

XCPlite 使用 CMake 作为构建系统。  
要快速构建所有示例，请使用提供的构建脚本。  
有关如何为 Linux、QNX、macOS 和 Windows 构建的详细信息，请参阅 [构建文档](docs/BUILDING.md)。


## 文档

- **[API 参考](docs/xcplib.md)** - XCP 仪表化 API
- **[配置](docs/xcplib_cfg.md)** - 配置选项
- **[示例](examples/README.md)** - 示例应用程序和 CANape 设置
- **[技术细节](docs/TECHNICAL.md)** - 寻址模式、A2L 生成、仪表化成本
- **[构建](docs/BUILDING.md)** - 详细的构建说明和故障排除
- **[XCP 介绍](docs/XCP_INTRODUCTION.md)** - 什么是 XCP？
- **[更新日志](CHANGELOG.md)** - 版本历史

## 许可证

有关详细信息，请参阅 [LICENSE](LICENSE) 文件。
