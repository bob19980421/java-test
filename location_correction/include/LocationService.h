// LocationService.h - 位置服务接口和实现

#ifndef LOCATION_SERVICE_H
#define LOCATION_SERVICE_H

#include <memory>
#include <vector>
#include <mutex>
#include <functional>
#include "LocationModel.h"
#include "ConfigModel.h"
#include "LocationCorrector.h"
#include "DataSource.h"
#include "DataStorage.h"
#include "Logger.h"
#include "Utils.h"

// 位置服务接口
class LocationService {
public:
    virtual ~LocationService() = default;
    
    // 初始化服务
    virtual bool initialize(const CorrectionConfig& config) = 0;
    
    // 启动服务
    virtual bool start() = 0;
    
    // 停止服务
    virtual void stop() = 0;
    
    // 获取当前位置
    virtual std::shared_ptr<CorrectedLocation> getCurrentLocation() = 0;
    
    // 获取历史位置
    virtual std::vector<std::shared_ptr<CorrectedLocation>> getHistoryLocations(long long startTime, long long endTime) = 0;
    
    // 添加位置变更监听器
    virtual void addLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) = 0;
    
    // 移除位置变更监听器
    virtual void removeLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) = 0;
    
    // 更新配置
    virtual void updateConfig(const CorrectionConfig& config) = 0;
    
    // 检查服务状态
    virtual bool isRunning() const = 0;
    
    // 重置服务
    virtual void reset() = 0;
};

// 基础位置服务实现
class BaseLocationService : public LocationService {
private:
    std::shared_ptr<LocationCorrector> locationCorrector; // 位置纠偏器
    std::shared_ptr<DataSourceManager> dataSourceManager; // 数据源管理器
    std::shared_ptr<DataStorage> dataStorage; // 数据存储
    std::vector<std::shared_ptr<LocationChangeListener>> listeners; // 位置变更监听器列表
    mutable std::mutex mutex; // 互斥锁
    bool running; // 服务运行状态
    std::shared_ptr<CorrectedLocation> lastCorrectedLocation; // 上次纠偏后的位置
    
    // 数据收集线程相关
    std::atomic<bool> stopDataCollection;
    std::thread dataCollectionThread;
    long long dataCollectionInterval; // 数据收集间隔（毫秒）
    
    // 执行数据收集任务
    void dataCollectionTask();
    
    // 处理新的位置数据
    void processNewLocation(std::shared_ptr<LocationInfo> location);

public:
    BaseLocationService(long long interval = 1000);
    
    ~BaseLocationService() override;
    
    bool initialize(const CorrectionConfig& config) override;
    
    bool start() override;
    
    void stop() override;
    
    std::shared_ptr<CorrectedLocation> getCurrentLocation() override;
    
    std::vector<std::shared_ptr<CorrectedLocation>> getHistoryLocations(long long startTime, long long endTime) override;
    
    void addLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) override;
    
    void removeLocationChangeListener(std::shared_ptr<LocationChangeListener> listener) override;
    
    void updateConfig(const CorrectionConfig& config) override;
    
    bool isRunning() const override;
    
    void reset() override;
    
    // 设置位置纠偏器
    void setLocationCorrector(std::shared_ptr<LocationCorrector> corrector);
    
    // 设置数据存储
    void setDataStorage(std::shared_ptr<DataStorage> storage);
    
    // 设置数据收集间隔
    void setDataCollectionInterval(long long interval);
    
    // 注册数据源
    bool registerDataSource(std::shared_ptr<DataSource> source);
    
    // 注销数据源
    bool unregisterDataSource(DataSourceType type);
};

// 高性能位置服务
class HighPerformanceLocationService : public BaseLocationService {
private:
    // 性能优化相关配置
    struct PerformanceConfig {
        bool enableBatching;         // 是否启用批处理
        size_t batchSize;            // 批处理大小
        long long batchTimeout;      // 批处理超时时间（毫秒）
        bool enableCaching;          // 是否启用缓存
        size_t cacheSize;            // 缓存大小
        long long cacheTimeout;      // 缓存超时时间（毫秒）
        bool enableBackgroundMode;   // 是否启用后台模式
    } performanceConfig;
    
    // 批处理队列
    std::vector<std::shared_ptr<LocationInfo>> batchQueue;
    
    // 缓存
    std::map<std::string, std::pair<std::shared_ptr<CorrectedLocation>, long long>> locationCache;
    
    // 批处理线程
    std::atomic<bool> stopBatchProcessing;
    std::thread batchProcessingThread;
    
    // 互斥锁
    mutable std::mutex batchMutex;
    mutable std::mutex cacheMutex;
    
    // 执行批处理任务
    void batchProcessingTask();
    
    // 从缓存获取位置
    std::shared_ptr<CorrectedLocation> getLocationFromCache(const std::string& key);
    
    // 添加位置到缓存
    void addLocationToCache(const std::string& key, std::shared_ptr<CorrectedLocation> location);
    
    // 清理过期缓存
    void cleanupExpiredCache();

public:
    HighPerformanceLocationService(long long interval = 1000);
    
    ~HighPerformanceLocationService() override;
    
    bool initialize(const CorrectionConfig& config) override;
    
    bool start() override;
    
    void stop() override;
    
    std::shared_ptr<CorrectedLocation> getCurrentLocation() override;
    
    void processNewLocation(std::shared_ptr<LocationInfo> location) override;
    
    // 设置性能配置
    void setPerformanceConfig(bool enableBatching, size_t batchSize, long long batchTimeout,
                              bool enableCaching, size_t cacheSize, long long cacheTimeout,
                              bool enableBackgroundMode);
    
    // 清空批处理队列
    void clearBatchQueue();
    
    // 清空缓存
    void clearCache();
};

// 服务工厂类（用于创建不同类型的服务实例）
class LocationServiceFactory {
public:
    static std::shared_ptr<LocationService> createService(const std::string& serviceType = "base");
    
    // 注册自定义服务创建器
    static bool registerServiceCreator(const std::string& serviceType, 
                                      std::function<std::shared_ptr<LocationService>()> creator);
    
    // 获取所有可用的服务类型
    static std::vector<std::string> getAvailableServiceTypes();
};

#endif // LOCATION_SERVICE_H