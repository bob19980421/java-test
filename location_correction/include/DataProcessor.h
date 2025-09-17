// DataProcessor.h - 数据预处理模块

#ifndef DATA_PROCESSOR_H
#define DATA_PROCESSOR_H

#include <memory>
#include <vector>
#include <functional>
#include "LocationModel.h"
#include "ConfigModel.h"
#include "Logger.h"

// 数据处理器接口
class DataProcessor {
public:
    virtual ~DataProcessor() = default;
    
    // 处理单个位置数据
    virtual std::shared_ptr<LocationInfo> process(std::shared_ptr<LocationInfo> location) = 0;
    
    // 批量处理位置数据
    virtual std::vector<std::shared_ptr<LocationInfo>> batchProcess(const std::vector<std::shared_ptr<LocationInfo>>& locations) = 0;
    
    // 获取处理器名称
    virtual std::string getName() const = 0;
    
    // 设置处理器配置
    virtual void setConfig(const CorrectionConfig& config) = 0;
    
    // 检查是否启用
    virtual bool isEnabled() const = 0;
    
    // 设置启用状态
    virtual void setEnabled(bool enabled) = 0;
};

// 基础数据处理器
class BaseDataProcessor : public DataProcessor {
protected:
    bool enabled; // 是否启用
    CorrectionConfig config; // 配置信息

public:
    BaseDataProcessor() : enabled(true) {}
    virtual ~BaseDataProcessor() override = default;
    
    std::vector<std::shared_ptr<LocationInfo>> batchProcess(const std::vector<std::shared_ptr<LocationInfo>>& locations) override;
    
    void setConfig(const CorrectionConfig& config) override;
    
    bool isEnabled() const override { return enabled; }
    
    void setEnabled(bool enabled) override { this->enabled = enabled; }
};

// 精度过滤器
class AccuracyFilterProcessor : public BaseDataProcessor {
private:
    double minAccuracyThreshold; // 最小精度阈值

public:
    AccuracyFilterProcessor(double minAccuracy = 100.0);
    
    std::shared_ptr<LocationInfo> process(std::shared_ptr<LocationInfo> location) override;
    
    std::string getName() const override { return "AccuracyFilterProcessor"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    // 设置最小精度阈值
    void setMinAccuracyThreshold(double threshold);
};

// 时间过滤器
class TimeFilterProcessor : public BaseDataProcessor {
private:
    long long maxTimeDiff; // 最大时间差（毫秒）

public:
    TimeFilterProcessor(long long maxDiff = 60000);
    
    std::shared_ptr<LocationInfo> process(std::shared_ptr<LocationInfo> location) override;
    
    std::string getName() const override { return "TimeFilterProcessor"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    // 设置最大时间差
    void setMaxTimeDiff(long long diff);
};

// 异常值检测器
class OutlierDetectionProcessor : public BaseDataProcessor {
private:
    double maxDistanceDiff; // 最大距离差（米）
    double maxSpeed;        // 最大速度（米/秒）
    std::vector<std::shared_ptr<LocationInfo>> history; // 历史位置数据
    size_t maxHistorySize;  // 最大历史记录数量

public:
    OutlierDetectionProcessor(double maxDist = 500.0, double maxSpd = 70.0, size_t historySize = 10);
    
    std::shared_ptr<LocationInfo> process(std::shared_ptr<LocationInfo> location) override;
    
    std::string getName() const override { return "OutlierDetectionProcessor"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    // 清空历史数据
    void clearHistory();
};

// 坐标转换器
class CoordinateConverterProcessor : public BaseDataProcessor {
private:
    // 坐标系转换参数
    struct ConversionParams {
        double a; // 长半轴
        double f; // 扁率
        double dx; // X轴偏差
        double dy; // Y轴偏差
        double dz; // Z轴偏差
    };
    
    ConversionParams params; // 转换参数

public:
    CoordinateConverterProcessor();
    
    std::shared_ptr<LocationInfo> process(std::shared_ptr<LocationInfo> location) override;
    
    std::string getName() const override { return "CoordinateConverterProcessor"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    // 设置转换参数
    void setConversionParams(double a, double f, double dx, double dy, double dz);
};

// 处理器链
class ProcessorChain {
private:
    std::vector<std::shared_ptr<DataProcessor>> processors; // 处理器列表
    mutable std::mutex mutex; // 互斥锁

public:
    // 添加处理器
    bool addProcessor(std::shared_ptr<DataProcessor> processor);
    
    // 移除处理器
    bool removeProcessor(const std::string& processorName);
    
    // 获取处理器
    std::shared_ptr<DataProcessor> getProcessor(const std::string& processorName) const;
    
    // 设置所有处理器的配置
    void setConfigForAll(const CorrectionConfig& config);
    
    // 处理单个位置数据
    std::shared_ptr<LocationInfo> process(std::shared_ptr<LocationInfo> location);
    
    // 批量处理位置数据
    std::vector<std::shared_ptr<LocationInfo>> batchProcess(const std::vector<std::shared_ptr<LocationInfo>>& locations);
    
    // 启用所有处理器
    void enableAll();
    
    // 禁用所有处理器
    void disableAll();
    
    // 清空处理器链
    void clear();
};

#endif // DATA_PROCESSOR_H