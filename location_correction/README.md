# 位置纠偏系统 (Location Correction System)

## 项目概述

位置纠偏系统是一个高精度的多源位置数据处理框架，能够整合GPS、WiFi、基站等多种定位数据源，并通过先进的算法进行位置数据的预处理、异常检测、多源融合和位置纠偏，最终提供更准确、更稳定的位置信息。

## 主要功能

- **多源数据采集**：支持GPS、WiFi、基站等多种定位数据源的实时采集
- **数据预处理**：包括精度过滤、时间过滤、异常点检测、坐标转换等
- **异常检测**：基于时间差异、距离偏移等多种异常检测算法
- **多源数据融合**：支持优先级融合、加权平均融合、自适应融合等多种融合算法
- **场景自适应**：能够自动识别室内、室外、高速公路等场景，并应用相应的纠偏策略
- **多模式切换**：支持高精度模式、低功耗模式、快速更新模式等多种工作模式
- **高性能处理**：支持批处理和位置缓存，提高系统性能
- **历史数据存储**：支持位置历史数据的存储和查询
- **实时位置更新**：提供位置更新监听器机制，实时获取位置更新

## 系统架构

![系统架构](resources/architecture.png)（注：实际项目中可添加系统架构图）

系统采用模块化设计，主要包含以下核心模块：

1. **数据模型层**：定义位置信息、配置信息等核心数据结构
2. **数据源层**：负责从各种定位源采集数据
3. **数据处理层**：对原始数据进行预处理和异常检测
4. **算法层**：实现各种融合算法和异常检测算法
5. **服务层**：提供位置服务接口和业务逻辑
6. **工具层**：提供日志、工具函数等通用功能

## 目录结构

```
location_correction/
├── include/           # 头文件目录
│   ├── AnomalyDetector.h     # 异常检测器接口及实现类
│   ├── ConfigModel.h         # 配置模型
│   ├── DataFusion.h          # 数据融合接口及实现类
│   ├── DataProcessor.h       # 数据处理器接口及实现类
│   ├── DataSource.h          # 数据源接口及实现类
│   ├── DataStorage.h         # 数据存储接口及实现类
│   ├── LocationCorrector.h   # 位置纠偏器接口及实现类
│   ├── LocationModel.h       # 位置模型
│   ├── LocationService.h     # 位置服务接口及实现类
│   ├── Logger.h              # 日志工具
│   ├── Logger.tpp            # 日志工具模板实现
│   └── Utils.h               # 通用工具函数
├── src/               # 源代码目录
│   ├── algorithm/            # 算法实现
│   ├── data/                 # 数据处理相关实现
│   ├── model/                # 数据模型实现
│   ├── service/              # 服务实现
│   ├── util/                 # 工具实现
│   └── main.cpp              # 主程序入口
├── test/              # 测试代码目录
├── resources/         # 资源文件目录
├── build/             # 构建输出目录
├── CMakeLists.txt     # CMake构建配置
└── README.md          # 项目说明文档
```

## 技术栈

- **编程语言**：C++17
- **构建工具**：CMake 3.10+ 
- **测试框架**：Google Test
- **线程安全**：使用C++11标准线程库
- **平台支持**：跨平台（Windows、Linux、macOS）

## 构建指南

### 前提条件

- CMake 3.10或更高版本
- C++17兼容的编译器（如GCC 7+、Clang 5+、MSVC 2017+）
- Google Test（用于构建测试）

### 构建步骤

1. **创建构建目录**

```bash
mkdir build
cd build
```

2. **运行CMake配置**

```bash
cmake ..
```

3. **编译项目**

```bash
# Linux/macOS
make

# Windows (Visual Studio)
msbuild location_correction.sln /p:Configuration=Release

# Windows (MinGW)
mingw32-make
```

4. **运行测试（可选）**

```bash
ctest
```

5. **安装（可选）**

```bash
# Linux/macOS
sudo make install

# Windows\ nmsbuild INSTALL.vcxproj /p:Configuration=Release
```

## 使用指南

### 基本使用

1. **初始化位置服务**

```cpp
#include "LocationService.h"

using namespace location_correction;

// 创建位置服务
auto serviceFactory = LocationServiceFactory::getInstance();
auto locationService = serviceFactory.createLocationService(ServiceType::HIGH_PERFORMANCE);

// 配置位置服务
LocationServiceConfig config;
config.enableGPS = true;
config.enableWifi = true;
config.enableBaseStation = true;
config.enableHistoryStorage = true;

// 初始化位置服务
locationService->initialize(config);
```

2. **设置位置更新监听器**

```cpp
locationService->setLocationUpdateListener([](const LocationInfo& location) {
    // 处理位置更新
    std::cout << "位置更新: 纬度=" << location.latitude 
              << ", 经度=" << location.longitude << std::endl;
});
```

3. **启动/停止位置服务**

```cpp
// 启动服务
locationService->start();

// 停止服务
locationService->stop();
```

4. **获取当前位置**

```cpp
auto currentLocation = locationService->getCurrentLocation();
if (currentLocation) {
    std::cout << "当前位置: " << currentLocation->latitude 
              << ", " << currentLocation->longitude << std::endl;
}
```

5. **查询位置历史**

```cpp
// 获取最近10个位置记录
auto history = locationService->getLocationHistory(10);
for (const auto& location : history) {
    std::cout << "历史位置: " << location->latitude 
              << ", " << location->longitude << std::endl;
}
```

### 命令行界面

项目提供了命令行界面，可以通过以下命令与系统交互：

```
h/help           - 显示帮助信息
s/start          - 启动位置服务
t/stop           - 停止位置服务
g/get            - 获取当前位置
l/list           - 列出最近的位置历史记录
mode [normal|high|low|fast|offline] - 设置纠偏模式
log [debug|info|warning|error]      - 设置日志级别
q/quit           - 退出程序
```

## 配置说明

### 位置服务配置 (LocationServiceConfig)

- `enableGPS`: 是否启用GPS数据源
- `enableWifi`: 是否启用WiFi数据源
- `enableBaseStation`: 是否启用基站数据源
- `enableHistoryStorage`: 是否启用历史存储
- `maxQueueSize`: 数据队列最大大小
- `cacheSize`: 位置缓存大小（高性能模式下）
- `batchProcessingSize`: 批处理大小（高性能模式下）

### 位置纠偏配置 (CorrectionConfig)

- `minCorrectionInterval`: 最小纠偏间隔（毫秒）
- `enableAnomalyDetection`: 是否启用异常检测
- `enableDataFusion`: 是否启用数据融合
- `enableAdaptiveCorrection`: 是否启用自适应纠偏
- `anomalyThresholds`: 异常检测阈值配置
- `sceneConfigs`: 场景配置列表

### 场景配置 (SceneConfig)

- `sceneType`: 场景类型（室内、室外、高速公路等）
- `maxSpeed`: 最大速度阈值
- `minAccuracy`: 最小精度阈值
- `weightForGPS`: GPS权重
- `weightForWifi`: WiFi权重
- `weightForBaseStation`: 基站权重

## 日志系统

系统使用自定义的日志系统，支持以下日志级别：
- DEBUG: 调试信息
- INFO: 一般信息
- WARNING: 警告信息
- ERROR: 错误信息

可以通过以下方式配置日志系统：

```cpp
// 设置日志文件
Logger::getInstance().setLogFile("location_correction.log");

// 设置日志级别
Logger::getInstance().setLogLevel(LogLevel::INFO);
```

## 常见问题

1. **位置服务初始化失败**
   - 检查是否正确安装了相关依赖
   - 确认权限设置是否正确
   - 查看日志文件获取详细错误信息

2. **位置更新不及时**
   - 检查网络连接
   - 调整最小纠偏间隔参数
   - 切换到快速更新模式

3. **位置精度不高**
   - 确保启用了多种数据源
   - 切换到高精度模式
   - 检查场景识别是否准确

## 版本历史

- v1.0.0: 初始版本，实现基本功能

## 许可证

[MIT License](LICENSE)（注：实际项目中可添加许可证文件）

## 联系方式

如有任何问题或建议，请联系项目维护者。