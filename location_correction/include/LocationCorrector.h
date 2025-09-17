// LocationCorrector.h - 位置纠偏核心功能接口

#ifndef LOCATION_CORRECTOR_H
#define LOCATION_CORRECTOR_H

#include <memory>
#include <vector>
#include <mutex>
#include "LocationModel.h"
#include "ConfigModel.h"
#include "AnomalyDetector.h"
#include "DataFusion.h"
#include "DataProcessor.h"
#include "DataStorage.h"
#include "DataSource.h"
#include "Logger.h"
#include "Utils.h"

// 位置纠偏器接口
class LocationCorrector {
public:
    virtual ~LocationCorrector() = default;
    
    // 初始化纠偏器
    virtual bool initialize(const CorrectionConfig& config) = 0;
    
    // 纠偏单个位置数据
    virtual std::shared_ptr<CorrectedLocation> correct(const LocationInfo& location) = 0;
    
    // 批量纠偏位置数据
    virtual std::vector<std::shared_ptr<CorrectedLocation>> batchCorrect(const std::vector<LocationInfo>& locations) = 0;
    
    // 实时纠偏（流式处理）
    virtual std::shared_ptr<CorrectedLocation> realTimeCorrect(std::shared_ptr<LocationInfo> location) = 0;
    
    // 更新配置
    virtual void updateConfig(const CorrectionConfig& config) = 0;
    
    // 获取当前配置
    virtual CorrectionConfig getCurrentConfig() const = 0;
    
    // 重置纠偏器状态
    virtual void reset() = 0;
    
    // 设置异常检测器
    virtual void setAnomalyDetector(std::shared_ptr<AnomalyDetector> detector) = 0;
    
    // 设置数据融合器
    virtual void setDataFusion(std::shared_ptr<DataFusion> fusion) = 0;
    
    // 添加数据处理器
    virtual void addDataProcessor(std::shared_ptr<DataProcessor> processor) = 0;
    
    // 添加位置变更监听器
    virtual void addLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) = 0;
    
    // 移除位置变更监听器
    virtual void removeLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) = 0;
};

// 基础位置纠偏器实现
class BaseLocationCorrector : public LocationCorrector {
private:
    CorrectionConfig currentConfig; // 当前配置
    std::shared_ptr<AnomalyDetector> anomalyDetector; // 异常检测器
    std::shared_ptr<DataFusion> dataFusion; // 数据融合器
    ProcessorChain processorChain; // 数据处理器链
    std::vector<std::shared_ptr<LocationChangeListener>> listeners; // 位置变更监听器列表
    mutable std::mutex mutex; // 互斥锁
    bool initialized; // 是否已初始化
    std::shared_ptr<DataStorage> storage; // 数据存储

protected:
    // 通知位置变更
    void notifyLocationChanged(const CorrectedLocation& correctedLocation);
    
    // 验证位置数据
    bool validateLocation(const LocationInfo& location);
    
    // 预处理位置数据
    std::shared_ptr<LocationInfo> preprocessLocation(const LocationInfo& location);

public:
    BaseLocationCorrector();
    
    bool initialize(const CorrectionConfig& config) override;
    
    std::shared_ptr<CorrectedLocation> correct(const LocationInfo& location) override;
    
    std::vector<std::shared_ptr<CorrectedLocation>> batchCorrect(const std::vector<LocationInfo>& locations) override;
    
    std::shared_ptr<CorrectedLocation> realTimeCorrect(std::shared_ptr<LocationInfo> location) override;
    
    void updateConfig(const CorrectionConfig& config) override;
    
    CorrectionConfig getCurrentConfig() const override;
    
    void reset() override;
    
    void setAnomalyDetector(std::shared_ptr<AnomalyDetector> detector) override;
    
    void setDataFusion(std::shared_ptr<DataFusion> fusion) override;
    
    void addDataProcessor(std::shared_ptr<DataProcessor> processor) override;
    
    void addLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) override;
    
    void removeLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) override;
    
    // 设置数据存储
    void setStorage(std::shared_ptr<DataStorage> dataStorage);
};

// 自适应位置纠偏器
class AdaptiveLocationCorrector : public BaseLocationCorrector {
private:
    // 场景识别相关
    enum class UserScene {
        STATIONARY, // 静止
        WALKING,    // 步行
        RUNNING,    // 跑步
        DRIVING,    // 驾车
        UNKNOWN     // 未知
    } currentScene;
    
    std::map<UserScene, CorrectionConfig> sceneConfigs; // 不同场景的配置
    std::vector<std::shared_ptr<LocationInfo>> recentLocations; // 最近位置历史
    size_t maxHistorySize; // 最大历史记录数量
    long long sceneCheckInterval; // 场景检测间隔（毫秒）
    long long lastSceneCheckTime; // 上次场景检测时间
    mutable std::mutex mutex; // 互斥锁

    // 识别用户场景
    UserScene recognizeScene();
    
    // 估算移动速度
    double estimateSpeed();
    
    // 估算移动加速度
    double estimateAcceleration();
    
    // 根据场景获取对应的配置
    CorrectionConfig getConfigForScene(UserScene scene);

public:
    AdaptiveLocationCorrector(size_t historySize = 50, long long checkInterval = 5000);
    
    std::shared_ptr<CorrectedLocation> correct(const LocationInfo& location) override;
    
    std::shared_ptr<CorrectedLocation> realTimeCorrect(std::shared_ptr<LocationInfo> location) override;
    
    void updateConfig(const CorrectionConfig& config) override;
    
    void reset() override;
    
    // 添加场景配置
    void addSceneConfig(UserScene scene, const CorrectionConfig& config);
    
    // 获取当前场景
    UserScene getCurrentScene() const;
    
    // 强制设置场景
    void forceSetScene(UserScene scene);
};

// 多模式位置纠偏器
class MultiModeLocationCorrector : public LocationCorrector {
private:
    std::map<std::string, std::shared_ptr<LocationCorrector>> correctors; // 不同模式的纠偏器
    std::string currentMode; // 当前模式
    CorrectionConfig defaultConfig; // 默认配置
    mutable std::mutex mutex; // 互斥锁

public:
    MultiModeLocationCorrector(const std::string& defaultMode = "default");
    
    bool initialize(const CorrectionConfig& config) override;
    
    std::shared_ptr<CorrectedLocation> correct(const LocationInfo& location) override;
    
    std::vector<std::shared_ptr<CorrectedLocation>> batchCorrect(const std::vector<LocationInfo>& locations) override;
    
    std::shared_ptr<CorrectedLocation> realTimeCorrect(std::shared_ptr<LocationInfo> location) override;
    
    void updateConfig(const CorrectionConfig& config) override;
    
    CorrectionConfig getCurrentConfig() const override;
    
    void reset() override;
    
    void setAnomalyDetector(std::shared_ptr<AnomalyDetector> detector) override;
    
    void setDataFusion(std::shared_ptr<DataFusion> fusion) override;
    
    void addDataProcessor(std::shared_ptr<DataProcessor> processor) override;
    
    void addLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) override;
    
    void removeLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) override;
    
    // 添加模式纠偏器
    bool addModeCorrector(const std::string& mode, std::shared_ptr<LocationCorrector> corrector);
    
    // 切换模式
    bool switchMode(const std::string& mode);
    
    // 获取当前模式
    std::string getCurrentMode() const;
};

#endif // LOCATION_CORRECTOR_H