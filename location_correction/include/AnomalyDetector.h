// AnomalyDetector.h - 异常点检测算法

#ifndef ANOMALY_DETECTOR_H
#define ANOMALY_DETECTOR_H

#include <memory>
#include <vector>
#include <mutex>
#include "LocationModel.h"
#include "ConfigModel.h"
#include "Logger.h"

// 异常点检测器接口
class AnomalyDetector {
public:
    virtual ~AnomalyDetector() = default;
    
    // 检测单个位置是否异常
    virtual bool isAnomaly(const LocationInfo& location) = 0;
    
    // 批量检测位置异常
    virtual std::vector<bool> batchDetect(const std::vector<LocationInfo>& locations) = 0;
    
    // 获取检测器名称
    virtual std::string getName() const = 0;
    
    // 设置配置
    virtual void setConfig(const CorrectionConfig& config) = 0;
    
    // 获取异常类型
    virtual std::string getAnomalyType(const LocationInfo& location) = 0;
};

// 时间差异常检测器
class TimeDifferenceAnomalyDetector : public AnomalyDetector {
private:
    long long maxTimeDiff; // 最大时间差（毫秒）
    long long lastTimestamp; // 上次位置的时间戳
    mutable std::mutex mutex; // 互斥锁

public:
    TimeDifferenceAnomalyDetector(long long maxDiff = 60000);
    
    bool isAnomaly(const LocationInfo& location) override;
    
    std::vector<bool> batchDetect(const std::vector<LocationInfo>& locations) override;
    
    std::string getName() const override { return "TimeDifferenceAnomalyDetector"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    std::string getAnomalyType(const LocationInfo& location) override;
    
    // 设置最大时间差
    void setMaxTimeDiff(long long diff);
    
    // 重置状态
    void reset();
};

// 距离偏移异常检测器
class DistanceOffsetAnomalyDetector : public AnomalyDetector {
private:
    double maxDistanceDiff; // 最大距离差（米）
    LocationInfo lastLocation; // 上次位置
    bool hasLastLocation; // 是否有上次位置
    mutable std::mutex mutex; // 互斥锁

public:
    DistanceOffsetAnomalyDetector(double maxDist = 500.0);
    
    bool isAnomaly(const LocationInfo& location) override;
    
    std::vector<bool> batchDetect(const std::vector<LocationInfo>& locations) override;
    
    std::string getName() const override { return "DistanceOffsetAnomalyDetector"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    std::string getAnomalyType(const LocationInfo& location) override;
    
    // 设置最大距离差
    void setMaxDistanceDiff(double diff);
    
    // 重置状态
    void reset();
};

// 加速度异常检测器
class AccelerationAnomalyDetector : public AnomalyDetector {
private:
    double accelerationThreshold; // 加速度阈值（m/s²）
    LocationInfo lastLocation; // 上次位置
    bool hasLastLocation; // 是否有上次位置
    mutable std::mutex mutex; // 互斥锁

public:
    AccelerationAnomalyDetector(double threshold = 10.0);
    
    bool isAnomaly(const LocationInfo& location) override;
    
    std::vector<bool> batchDetect(const std::vector<LocationInfo>& locations) override;
    
    std::string getName() const override { return "AccelerationAnomalyDetector"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    std::string getAnomalyType(const LocationInfo& location) override;
    
    // 设置加速度阈值
    void setAccelerationThreshold(double threshold);
    
    // 重置状态
    void reset();
};

// 低精度异常检测器
class LowAccuracyAnomalyDetector : public AnomalyDetector {
private:
    double minAccuracy; // 最小精度要求（米）

public:
    LowAccuracyAnomalyDetector(double accuracy = 100.0);
    
    bool isAnomaly(const LocationInfo& location) override;
    
    std::vector<bool> batchDetect(const std::vector<LocationInfo>& locations) override;
    
    std::string getName() const override { return "LowAccuracyAnomalyDetector"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    std::string getAnomalyType(const LocationInfo& location) override;
    
    // 设置最小精度要求
    void setMinAccuracy(double accuracy);
};

// 综合异常检测器
class CompositeAnomalyDetector : public AnomalyDetector {
private:
    std::vector<std::shared_ptr<AnomalyDetector>> detectors; // 检测器列表
    mutable std::mutex mutex; // 互斥锁
    bool useAnyDetection; // 是否任一检测器检测到就判定为异常

public:
    CompositeAnomalyDetector(bool useAny = true);
    
    bool addDetector(std::shared_ptr<AnomalyDetector> detector);
    
    bool removeDetector(const std::string& detectorName);
    
    bool isAnomaly(const LocationInfo& location) override;
    
    std::vector<bool> batchDetect(const std::vector<LocationInfo>& locations) override;
    
    std::string getName() const override { return "CompositeAnomalyDetector"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    std::string getAnomalyType(const LocationInfo& location) override;
    
    // 设置检测模式
    void setUseAnyDetection(bool useAny);
    
    // 清空所有检测器
    void clearDetectors();
};

#endif // ANOMALY_DETECTOR_H