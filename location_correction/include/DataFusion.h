// DataFusion.h - 多源数据融合算法

#ifndef DATA_FUSION_H
#define DATA_FUSION_H

#include <memory>
#include <vector>
#include <mutex>
#include "LocationModel.h"
#include "ConfigModel.h"
#include "Logger.h"
#include "Utils.h"

// 数据融合器接口
class DataFusion {
public:
    virtual ~DataFusion() = default;
    
    // 融合多个位置数据
    virtual std::shared_ptr<LocationInfo> fuse(const std::vector<std::shared_ptr<LocationInfo>>& locations) = 0;
    
    // 获取融合器名称
    virtual std::string getName() const = 0;
    
    // 设置配置
    virtual void setConfig(const CorrectionConfig& config) = 0;
    
    // 设置融合策略
    virtual void setFusionStrategy(FusionStrategy strategy) = 0;
};

// 基于优先级的融合器
class PriorityBasedFusion : public DataFusion {
private:
    std::vector<DataSourceType> priorityOrder; // 优先级顺序
    mutable std::mutex mutex; // 互斥锁

public:
    PriorityBasedFusion();
    
    std::shared_ptr<LocationInfo> fuse(const std::vector<std::shared_ptr<LocationInfo>>& locations) override;
    
    std::string getName() const override { return "PriorityBasedFusion"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    void setFusionStrategy(FusionStrategy strategy) override;
    
    // 设置优先级顺序
    void setPriorityOrder(const std::vector<DataSourceType>& order);
    
    // 添加优先级类型
    void addPriorityType(DataSourceType type, int priority);
};

// 加权平均融合器
class WeightedAverageFusion : public DataFusion {
private:
    std::map<DataSourceType, double> sourceWeights; // 数据源权重
    mutable std::mutex mutex; // 互斥锁

public:
    WeightedAverageFusion();
    
    std::shared_ptr<LocationInfo> fuse(const std::vector<std::shared_ptr<LocationInfo>>& locations) override;
    
    std::string getName() const override { return "WeightedAverageFusion"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    void setFusionStrategy(FusionStrategy strategy) override;
    
    // 设置数据源权重
    void setSourceWeight(DataSourceType type, double weight);
    
    // 获取数据源权重
    double getSourceWeight(DataSourceType type) const;
};

// 自适应融合器
class AdaptiveFusion : public DataFusion {
private:
    FusionStrategy currentStrategy; // 当前融合策略
    std::shared_ptr<PriorityBasedFusion> priorityFusion; // 优先级融合器
    std::shared_ptr<WeightedAverageFusion> weightedFusion; // 加权融合器
    mutable std::mutex mutex; // 互斥锁
    
    // 自适应参数
    struct AdaptiveParams {
        double confidenceThreshold; // 置信度阈值
        double accuracyWeight;      // 精度权重
        double timeWeight;          // 时间权重
        double consistencyWeight;   // 一致性权重
    } params;

public:
    AdaptiveFusion();
    
    std::shared_ptr<LocationInfo> fuse(const std::vector<std::shared_ptr<LocationInfo>>& locations) override;
    
    std::string getName() const override { return "AdaptiveFusion"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    void setFusionStrategy(FusionStrategy strategy) override;
    
    // 设置自适应参数
    void setAdaptiveParams(double confidenceThreshold, double accuracyWeight, double timeWeight, double consistencyWeight);
    
    // 计算位置数据的置信度
    double calculateConfidence(const LocationInfo& location) const;
    
    // 计算位置数据之间的一致性
    double calculateConsistency(const std::vector<std::shared_ptr<LocationInfo>>& locations) const;
};

// 足迹连贯性融合器
class FootprintCoherenceFusion : public DataFusion {
private:
    std::vector<std::shared_ptr<LocationInfo>> history; // 历史位置数据
    size_t maxHistorySize; // 最大历史记录数量
    double coherenceThreshold; // 连贯性阈值
    mutable std::mutex mutex; // 互斥锁

public:
    FootprintCoherenceFusion(size_t historySize = 20, double threshold = 0.7);
    
    std::shared_ptr<LocationInfo> fuse(const std::vector<std::shared_ptr<LocationInfo>>& locations) override;
    
    std::string getName() const override { return "FootprintCoherenceFusion"; }
    
    void setConfig(const CorrectionConfig& config) override;
    
    void setFusionStrategy(FusionStrategy strategy) override;
    
    // 设置历史记录最大数量
    void setMaxHistorySize(size_t size);
    
    // 设置连贯性阈值
    void setCoherenceThreshold(double threshold);
    
    // 清空历史数据
    void clearHistory();
    
    // 计算新位置与历史足迹的连贯性
    double calculateCoherence(const LocationInfo& newLocation) const;
};

#endif // DATA_FUSION_H