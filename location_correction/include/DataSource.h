// DataSource.h - 定位数据源接口定义

#ifndef DATA_SOURCE_H
#define DATA_SOURCE_H

#include <memory>
#include <vector>
#include <mutex>
#include "LocationModel.h"
#include "Logger.h"

// 数据源接口
class DataSource {
public:
    virtual ~DataSource() = default;
    
    // 初始化数据源
    virtual bool initialize() = 0;
    
    // 反初始化数据源
    virtual void release() = 0;
    
    // 获取当前位置信息
    virtual std::shared_ptr<LocationInfo> getLocation() = 0;
    
    // 获取数据源类型
    virtual DataSourceType getType() const = 0;
    
    // 获取数据源ID
    virtual std::string getSourceId() const = 0;
    
    // 检查数据源是否可用
    virtual bool isAvailable() const = 0;
    
    // 设置位置变化监听器
    virtual void setLocationListener(std::shared_ptr<LocationChangeListener> listener) = 0;
};

// GNSS数据源
class GNSSDataSource : public DataSource {
private:
    std::string sourceId; // 数据源ID
    std::shared_ptr<LocationChangeListener> listener; // 位置变化监听器
    mutable std::mutex mutex; // 互斥锁
    bool initialized; // 是否已初始化

public:
    GNSSDataSource(const std::string& id = "GNSS_DEFAULT");
    ~GNSSDataSource() override;

    bool initialize() override;
    void release() override;
    std::shared_ptr<LocationInfo> getLocation() override;
    DataSourceType getType() const override { return DataSourceType::GNSS; }
    std::string getSourceId() const override { return sourceId; }
    bool isAvailable() const override;
    void setLocationListener(std::shared_ptr<LocationChangeListener> listener) override;
};

// Wi-Fi数据源
class WifiDataSource : public DataSource {
private:
    std::string sourceId; // 数据源ID
    std::shared_ptr<LocationChangeListener> listener; // 位置变化监听器
    mutable std::mutex mutex; // 互斥锁
    bool initialized; // 是否已初始化

public:
    WifiDataSource(const std::string& id = "WIFI_DEFAULT");
    ~WifiDataSource() override;

    bool initialize() override;
    void release() override;
    std::shared_ptr<LocationInfo> getLocation() override;
    DataSourceType getType() const override { return DataSourceType::WIFI; }
    std::string getSourceId() const override { return sourceId; }
    bool isAvailable() const override;
    void setLocationListener(std::shared_ptr<LocationChangeListener> listener) override;
};

// 基站数据源
class BaseStationDataSource : public DataSource {
private:
    std::string sourceId; // 数据源ID
    std::shared_ptr<LocationChangeListener> listener; // 位置变化监听器
    mutable std::mutex mutex; // 互斥锁
    bool initialized; // 是否已初始化

public:
    BaseStationDataSource(const std::string& id = "BASE_STATION_DEFAULT");
    ~BaseStationDataSource() override;

    bool initialize() override;
    void release() override;
    std::shared_ptr<LocationInfo> getLocation() override;
    DataSourceType getType() const override { return DataSourceType::BASE_STATION; }
    std::string getSourceId() const override { return sourceId; }
    bool isAvailable() const override;
    void setLocationListener(std::shared_ptr<LocationChangeListener> listener) override;
};

// 数据源管理器
class DataSourceManager {
private:
    static DataSourceManager* instance; // 单例实例
    static std::mutex instanceMutex; // 单例互斥锁
    std::vector<std::shared_ptr<DataSource>> dataSources; // 数据源列表
    mutable std::mutex dataSourcesMutex; // 数据源列表互斥锁

    // 私有构造函数
    DataSourceManager();

public:
    // 获取单例实例
    static DataSourceManager* getInstance();

    // 添加数据源
    bool addDataSource(std::shared_ptr<DataSource> dataSource);

    // 移除数据源
    bool removeDataSource(const std::string& sourceId);

    // 获取所有数据源
    std::vector<std::shared_ptr<DataSource>> getAllDataSources() const;

    // 根据类型获取数据源
    std::vector<std::shared_ptr<DataSource>> getDataSourcesByType(DataSourceType type) const;

    // 根据ID获取数据源
    std::shared_ptr<DataSource> getDataSourceById(const std::string& sourceId) const;

    // 初始化所有数据源
    bool initializeAllDataSources();

    // 释放所有数据源
    void releaseAllDataSources();

    // 获取所有可用的数据源
    std::vector<std::shared_ptr<DataSource>> getAvailableDataSources() const;

    // 设置所有数据源的位置监听器
    void setLocationListenerForAll(std::shared_ptr<LocationChangeListener> listener);
};

#endif // DATA_SOURCE_H