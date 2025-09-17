// DataStorage.h - 数据存储模块

#ifndef DATA_STORAGE_H
#define DATA_STORAGE_H

#include <memory>
#include <vector>
#include <string>
#include <mutex>
#include "LocationModel.h"
#include "ConfigModel.h"
#include "Logger.h"

// 数据存储接口
class DataStorage {
public:
    virtual ~DataStorage() = default;
    
    // 初始化存储
    virtual bool initialize() = 0;
    
    // 释放存储资源
    virtual void release() = 0;
    
    // 保存单个位置数据
    virtual bool saveLocation(const LocationInfo& location) = 0;
    
    // 批量保存位置数据
    virtual bool saveLocations(const std::vector<LocationInfo>& locations) = 0;
    
    // 获取最近的位置数据
    virtual std::shared_ptr<LocationInfo> getLatestLocation() = 0;
    
    // 获取指定时间范围内的位置数据
    virtual std::vector<std::shared_ptr<LocationInfo>> getLocationsByTimeRange(long long startTime, long long endTime) = 0;
    
    // 获取指定数量的历史位置数据
    virtual std::vector<std::shared_ptr<LocationInfo>> getRecentLocations(size_t count) = 0;
    
    // 清空所有位置数据
    virtual bool clearAllLocations() = 0;
    
    // 获取存储名称
    virtual std::string getName() const = 0;
};

// 内存存储实现
class MemoryStorage : public DataStorage {
private:
    std::vector<std::shared_ptr<LocationInfo>> locations; // 内存中的位置数据
    mutable std::mutex mutex; // 互斥锁
    size_t maxCapacity; // 最大容量
    bool initialized; // 是否已初始化

public:
    MemoryStorage(size_t capacity = 1000);
    ~MemoryStorage() override;
    
    bool initialize() override;
    
    void release() override;
    
    bool saveLocation(const LocationInfo& location) override;
    
    bool saveLocations(const std::vector<LocationInfo>& locations) override;
    
    std::shared_ptr<LocationInfo> getLatestLocation() override;
    
    std::vector<std::shared_ptr<LocationInfo>> getLocationsByTimeRange(long long startTime, long long endTime) override;
    
    std::vector<std::shared_ptr<LocationInfo>> getRecentLocations(size_t count) override;
    
    bool clearAllLocations() override;
    
    std::string getName() const override { return "MemoryStorage"; }
    
    // 获取当前存储的位置数量
    size_t getLocationCount() const;
    
    // 设置最大容量
    void setMaxCapacity(size_t capacity);
};

// 文件存储实现
class FileStorage : public DataStorage {
private:
    std::string filePath; // 文件路径
    mutable std::mutex mutex; // 互斥锁
    size_t maxFileSize; // 最大文件大小（字节）
    bool initialized; // 是否已初始化

public:
    FileStorage(const std::string& path = "location_data.dat", size_t maxSize = 10 * 1024 * 1024);
    ~FileStorage() override;
    
    bool initialize() override;
    
    void release() override;
    
    bool saveLocation(const LocationInfo& location) override;
    
    bool saveLocations(const std::vector<LocationInfo>& locations) override;
    
    std::shared_ptr<LocationInfo> getLatestLocation() override;
    
    std::vector<std::shared_ptr<LocationInfo>> getLocationsByTimeRange(long long startTime, long long endTime) override;
    
    std::vector<std::shared_ptr<LocationInfo>> getRecentLocations(size_t count) override;
    
    bool clearAllLocations() override;
    
    std::string getName() const override { return "FileStorage"; }
    
    // 设置文件路径
    void setFilePath(const std::string& path);
    
    // 设置最大文件大小
    void setMaxFileSize(size_t size);
    
    // 检查并切换文件
    void checkAndRotateFile();
};

// 存储管理器
class StorageManager {
private:
    static StorageManager* instance; // 单例实例
    static std::mutex instanceMutex; // 单例互斥锁
    std::vector<std::shared_ptr<DataStorage>> storages; // 存储列表
    mutable std::mutex storagesMutex; // 存储列表互斥锁

    // 私有构造函数
    StorageManager();

public:
    // 获取单例实例
    static StorageManager* getInstance();
    
    // 添加存储
    bool addStorage(std::shared_ptr<DataStorage> storage);
    
    // 移除存储
    bool removeStorage(const std::string& storageName);
    
    // 获取所有存储
    std::vector<std::shared_ptr<DataStorage>> getAllStorages() const;
    
    // 根据名称获取存储
    std::shared_ptr<DataStorage> getStorageByName(const std::string& storageName) const;
    
    // 初始化所有存储
    bool initializeAllStorages();
    
    // 释放所有存储
    void releaseAllStorages();
    
    // 保存位置数据到所有存储
    bool saveLocationToAll(const LocationInfo& location);
    
    // 保存位置数据到指定存储
    bool saveLocationTo(const std::string& storageName, const LocationInfo& location);
};

#endif // DATA_STORAGE_H