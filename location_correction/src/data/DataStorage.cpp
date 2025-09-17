// DataStorage.cpp - 数据存储实现

#include "DataStorage.h"
#include "Logger.h"
#include "Utils.h"
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <chrono>

// DataStorage构造函数
DataStorage::DataStorage() : 
    initialized(false),
    enabled(true),
    storageCapacity(10000) // 默认存储容量
{
}

// DataStorage析构函数
DataStorage::~DataStorage() {
    if (initialized) {
        close();
    }
}

// 初始化存储
bool DataStorage::initialize(const StorageConfig& config) {
    if (initialized) {
        LOG_WARNING("Storage already initialized");
        return true;
    }
    
    try {
        // 保存配置
        this->config = config;
        
        // 设置存储容量
        if (config.capacity > 0) {
            storageCapacity = config.capacity;
        }
        
        initialized = true;
        LOG_INFO("Storage initialized successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize storage: %s", e.what());
        return false;
    }
}

// 关闭存储
bool DataStorage::close() {
    if (!initialized) {
        return true;
    }
    
    try {
        initialized = false;
        LOG_INFO("Storage closed successfully");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to close storage: %s", e.what());
        return false;
    }
}

// 检查存储是否已初始化
bool DataStorage::isInitialized() const {
    return initialized;
}

// 设置存储是否启用
void DataStorage::setEnabled(bool enable) {
    enabled = enable;
}

// 检查存储是否启用
bool DataStorage::isEnabled() const {
    return enabled;
}

// 设置存储容量
void DataStorage::setStorageCapacity(size_t capacity) {
    if (capacity > 0) {
        storageCapacity = capacity;
        LOG_INFO("Storage capacity set to %zu", capacity);
    }
}

// 获取存储容量
size_t DataStorage::getStorageCapacity() const {
    return storageCapacity;
}

// MemoryStorage构造函数
MemoryStorage::MemoryStorage() : DataStorage(), locations() {
}

// 初始化内存存储
bool MemoryStorage::initialize(const StorageConfig& config) {
    std::lock_guard<std::mutex> lock(mutex);
    return DataStorage::initialize(config);
}

// 关闭内存存储
bool MemoryStorage::close() {
    std::lock_guard<std::mutex> lock(mutex);
    locations.clear();
    return DataStorage::close();
}

// 存储单个位置数据
bool MemoryStorage::store(const LocationInfo& location) {
    if (!isInitialized() || !isEnabled()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        // 检查存储容量
        if (locations.size() >= storageCapacity) {
            // 删除最早的位置数据
            locations.pop_front();
        }
        
        // 存储位置数据
        locations.push_back(location);
        
        LOG_DEBUG("Stored location in memory: %s", location.toString().c_str());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to store location in memory: %s", e.what());
        return false;
    }
}

// 批量存储位置数据
bool MemoryStorage::batchStore(const std::vector<LocationInfo>& locations) {
    if (!isInitialized() || !isEnabled()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        for (const auto& location : locations) {
            // 检查存储容量
            if (this->locations.size() >= storageCapacity) {
                // 删除最早的位置数据
                this->locations.pop_front();
            }
            
            // 存储位置数据
            this->locations.push_back(location);
        }
        
        LOG_DEBUG("Batch stored %zu locations in memory", locations.size());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to batch store locations in memory: %s", e.what());
        return false;
    }
}

// 根据时间范围查询位置数据
std::vector<LocationInfo> MemoryStorage::queryByTimeRange(long long startTime, long long endTime) {
    std::vector<LocationInfo> result;
    
    if (!isInitialized() || !isEnabled()) {
        return result;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        for (const auto& location : locations) {
            if (location.timestamp >= startTime && location.timestamp <= endTime) {
                result.push_back(location);
            }
        }
        
        LOG_DEBUG("Query by time range returned %zu results", result.size());
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to query locations by time range: %s", e.what());
    }
    
    return result;
}

// 根据数据源类型查询位置数据
std::vector<LocationInfo> MemoryStorage::queryByDataSource(DataSourceType sourceType) {
    std::vector<LocationInfo> result;
    
    if (!isInitialized() || !isEnabled()) {
        return result;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        for (const auto& location : locations) {
            if (location.sourceType == sourceType) {
                result.push_back(location);
            }
        }
        
        LOG_DEBUG("Query by data source returned %zu results", result.size());
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to query locations by data source: %s", e.what());
    }
    
    return result;
}

// 获取最新的位置数据
std::optional<LocationInfo> MemoryStorage::getLatestLocation() {
    if (!isInitialized() || !isEnabled() || locations.empty()) {
        return std::nullopt;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // 返回最新的位置数据（最后添加的）
    return locations.back();
}

// 获取存储的位置数据总数
size_t MemoryStorage::getStoredCount() const {
    std::lock_guard<std::mutex> lock(mutex);
    return locations.size();
}

// 清空存储的数据
bool MemoryStorage::clearAll() {
    if (!isInitialized() || !isEnabled()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        locations.clear();
        LOG_INFO("Memory storage cleared");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to clear memory storage: %s", e.what());
        return false;
    }
}

// FileStorage构造函数
FileStorage::FileStorage() : 
    DataStorage(), 
    fileStream(nullptr),
    fileSize(0),
    rotationInterval(3600000), // 默认1小时轮转一次
    maxFileSize(10 * 1024 * 1024) // 默认最大文件大小10MB
{
}

// FileStorage析构函数
FileStorage::~FileStorage() {
    if (fileStream) {
        fileStream->close();
        delete fileStream;
        fileStream = nullptr;
    }
}

// 初始化文件存储
bool FileStorage::initialize(const StorageConfig& config) {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!DataStorage::initialize(config)) {
        return false;
    }
    
    try {
        // 确保目录存在
        std::filesystem::path directory = config.storagePath;
        if (!std::filesystem::exists(directory)) {
            if (!std::filesystem::create_directories(directory)) {
                LOG_ERROR("Failed to create storage directory: %s", config.storagePath.c_str());
                return false;
            }
        }
        
        // 打开文件流
        openFileStream();
        
        LOG_INFO("File storage initialized successfully: %s", config.storagePath.c_str());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to initialize file storage: %s", e.what());
        return false;
    }
}

// 打开文件流
void FileStorage::openFileStream() {
    if (fileStream) {
        fileStream->close();
        delete fileStream;
        fileStream = nullptr;
    }
    
    // 生成文件名
    std::string timestamp = currentDateTimeString();
    std::string fileName = config.storagePath + "/locations_" + timestamp + ".log";
    
    // 打开文件流（追加模式）
    fileStream = new std::ofstream(fileName, std::ios::out | std::ios::app);
    
    if (!fileStream->is_open()) {
        LOG_ERROR("Failed to open file stream: %s", fileName.c_str());
        delete fileStream;
        fileStream = nullptr;
        return;
    }
    
    // 获取文件大小
    fileStream->seekp(0, std::ios::end);
    fileSize = fileStream->tellp();
    fileStream->seekp(0, std::ios::beg);
    
    // 记录上次轮转时间
    lastRotationTime = getCurrentTimestampMs();
    
    LOG_INFO("File stream opened: %s", fileName.c_str());
}

// 检查并执行文件轮转
void FileStorage::checkAndRotateFile() {
    // 检查是否需要轮转
    long long currentTime = getCurrentTimestampMs();
    
    if (currentTime - lastRotationTime >= rotationInterval || fileSize >= maxFileSize) {
        LOG_INFO("Rotating log file (time: %lld, size: %zu)", 
                currentTime - lastRotationTime, fileSize);
        openFileStream();
    }
}

// 关闭文件存储
bool FileStorage::close() {
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        if (fileStream) {
            fileStream->close();
            delete fileStream;
            fileStream = nullptr;
        }
        
        LOG_INFO("File storage closed successfully");
        return DataStorage::close();
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to close file storage: %s", e.what());
        return false;
    }
}

// 设置文件轮转间隔
void FileStorage::setRotationInterval(long long intervalMs) {
    if (intervalMs > 0) {
        rotationInterval = intervalMs;
        LOG_INFO("File rotation interval set to %lld ms", intervalMs);
    }
}

// 设置最大文件大小
void FileStorage::setMaxFileSize(size_t sizeBytes) {
    if (sizeBytes > 0) {
        maxFileSize = sizeBytes;
        LOG_INFO("Max file size set to %zu bytes", sizeBytes);
    }
}

// 存储单个位置数据
bool FileStorage::store(const LocationInfo& location) {
    if (!isInitialized() || !isEnabled() || !fileStream) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        // 检查文件轮转
        checkAndRotateFile();
        
        if (!fileStream->is_open()) {
            LOG_ERROR("File stream is not open");
            return false;
        }
        
        // 序列化位置数据并写入文件
        std::string serializedData = serializeLocation(location);
        *fileStream << serializedData << std::endl;
        fileStream->flush();
        
        // 更新文件大小
        fileSize += serializedData.size() + 1; // +1 for newline
        
        LOG_DEBUG("Stored location to file");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to store location to file: %s", e.what());
        return false;
    }
}

// 批量存储位置数据
bool FileStorage::batchStore(const std::vector<LocationInfo>& locations) {
    if (!isInitialized() || !isEnabled() || !fileStream) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        for (const auto& location : locations) {
            // 检查文件轮转
            checkAndRotateFile();
            
            if (!fileStream->is_open()) {
                LOG_ERROR("File stream is not open");
                return false;
            }
            
            // 序列化位置数据并写入文件
            std::string serializedData = serializeLocation(location);
            *fileStream << serializedData << std::endl;
            
            // 更新文件大小
            fileSize += serializedData.size() + 1; // +1 for newline
        }
        
        // 批量写入后刷新流
        fileStream->flush();
        
        LOG_DEBUG("Batch stored %zu locations to file", locations.size());
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to batch store locations to file: %s", e.what());
        return false;
    }
}

// 根据时间范围查询位置数据
std::vector<LocationInfo> FileStorage::queryByTimeRange(long long startTime, long long endTime) {
    std::vector<LocationInfo> result;
    
    if (!isInitialized() || !isEnabled()) {
        return result;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        // 获取目录下的所有日志文件
        std::vector<std::string> logFiles = getLogFilesInDirectory(config.storagePath);
        
        // 遍历所有日志文件
        for (const auto& fileName : logFiles) {
            // 打开文件进行读取
            std::ifstream file(fileName);
            if (!file.is_open()) {
                LOG_WARNING("Failed to open log file for reading: %s", fileName.c_str());
                continue;
            }
            
            // 逐行读取并解析位置数据
            std::string line;
            while (std::getline(file, line)) {
                try {
                    LocationInfo location = deserializeLocation(line);
                    
                    // 检查是否在时间范围内
                    if (location.timestamp >= startTime && location.timestamp <= endTime) {
                        result.push_back(location);
                    }
                } catch (const std::exception& e) {
                    LOG_WARNING("Failed to parse location data: %s", e.what());
                }
            }
            
            file.close();
        }
        
        LOG_DEBUG("Query by time range returned %zu results", result.size());
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to query locations by time range: %s", e.what());
    }
    
    return result;
}

// 根据数据源类型查询位置数据
std::vector<LocationInfo> FileStorage::queryByDataSource(DataSourceType sourceType) {
    std::vector<LocationInfo> result;
    
    if (!isInitialized() || !isEnabled()) {
        return result;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        // 获取目录下的所有日志文件
        std::vector<std::string> logFiles = getLogFilesInDirectory(config.storagePath);
        
        // 遍历所有日志文件
        for (const auto& fileName : logFiles) {
            // 打开文件进行读取
            std::ifstream file(fileName);
            if (!file.is_open()) {
                LOG_WARNING("Failed to open log file for reading: %s", fileName.c_str());
                continue;
            }
            
            // 逐行读取并解析位置数据
            std::string line;
            while (std::getline(file, line)) {
                try {
                    LocationInfo location = deserializeLocation(line);
                    
                    // 检查数据源类型
                    if (location.sourceType == sourceType) {
                        result.push_back(location);
                    }
                } catch (const std::exception& e) {
                    LOG_WARNING("Failed to parse location data: %s", e.what());
                }
            }
            
            file.close();
        }
        
        LOG_DEBUG("Query by data source returned %zu results", result.size());
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to query locations by data source: %s", e.what());
    }
    
    return result;
}

// 获取最新的位置数据
std::optional<LocationInfo> FileStorage::getLatestLocation() {
    if (!isInitialized() || !isEnabled()) {
        return std::nullopt;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        // 获取目录下的所有日志文件（按修改时间排序）
        std::vector<std::string> logFiles = getLogFilesInDirectory(config.storagePath, true);
        
        if (logFiles.empty()) {
            return std::nullopt;
        }
        
        // 从最新的文件开始查找
        for (const auto& fileName : logFiles) {
            // 打开文件进行读取
            std::ifstream file(fileName);
            if (!file.is_open()) {
                LOG_WARNING("Failed to open log file for reading: %s", fileName.c_str());
                continue;
            }
            
            // 读取文件的最后一行
            std::string line, lastLine;
            while (std::getline(file, line)) {
                lastLine = line;
            }
            
            file.close();
            
            if (!lastLine.empty()) {
                try {
                    LocationInfo location = deserializeLocation(lastLine);
                    return location;
                } catch (const std::exception& e) {
                    LOG_WARNING("Failed to parse latest location data: %s", e.what());
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get latest location: %s", e.what());
    }
    
    return std::nullopt;
}

// 获取存储的位置数据总数
size_t FileStorage::getStoredCount() const {
    // 文件存储无法高效地获取总数量，这里返回-1表示不支持
    return static_cast<size_t>(-1);
}

// 清空存储的数据
bool FileStorage::clearAll() {
    if (!isInitialized() || !isEnabled()) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    try {
        // 关闭当前文件流
        if (fileStream) {
            fileStream->close();
            delete fileStream;
            fileStream = nullptr;
        }
        
        // 删除所有日志文件
        std::vector<std::string> logFiles = getLogFilesInDirectory(config.storagePath);
        for (const auto& fileName : logFiles) {
            if (!std::filesystem::remove(fileName)) {
                LOG_WARNING("Failed to delete log file: %s", fileName.c_str());
            }
        }
        
        // 重新打开文件流
        openFileStream();
        
        LOG_INFO("File storage cleared");
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to clear file storage: %s", e.what());
        return false;
    }
}

// 序列化位置数据
std::string FileStorage::serializeLocation(const LocationInfo& location) {
    std::stringstream ss;
    
    // 使用CSV格式序列化
    ss << location.timestamp << "," 
       << location.latitude << "," 
       << location.longitude << "," 
       << location.altitude << "," 
       << location.accuracy << "," 
       << static_cast<int>(location.sourceType) << "," 
       << static_cast<int>(location.status);
    
    // 添加额外信息
    for (const auto& [key, value] : location.getExtras()) {
        ss << ",[" << key << ":" << value << "]";
    }
    
    return ss.str();
}

// 反序列化位置数据
LocationInfo FileStorage::deserializeLocation(const std::string& data) {
    LocationInfo location;
    
    std::stringstream ss(data);
    std::string token;
    
    // 解析基本字段
    if (std::getline(ss, token, ',')) location.timestamp = std::stoll(token);
    if (std::getline(ss, token, ',')) location.latitude = std::stod(token);
    if (std::getline(ss, token, ',')) location.longitude = std::stod(token);
    if (std::getline(ss, token, ',')) location.altitude = std::stod(token);
    if (std::getline(ss, token, ',')) location.accuracy = std::stod(token);
    if (std::getline(ss, token, ',')) location.sourceType = static_cast<DataSourceType>(std::stoi(token));
    if (std::getline(ss, token, ',')) location.status = static_cast<LocationStatus>(std::stoi(token));
    
    // 解析额外信息
    while (std::getline(ss, token, ',')) {
        if (token.size() >= 3 && token[0] == '[' && token.back() == ']') {
            // 提取键值对
            std::string kv = token.substr(1, token.size() - 2);
            size_t colonPos = kv.find(':');
            if (colonPos != std::string::npos) {
                std::string key = kv.substr(0, colonPos);
                std::string value = kv.substr(colonPos + 1);
                location.setExtra(key, value);
            }
        }
    }
    
    return location;
}

// 获取目录下的所有日志文件
std::vector<std::string> FileStorage::getLogFilesInDirectory(const std::string& directoryPath, bool sortByTime) {
    std::vector<std::string> logFiles;
    
    try {
        // 检查目录是否存在
        if (!std::filesystem::exists(directoryPath)) {
            return logFiles;
        }
        
        // 遍历目录中的所有文件
        for (const auto& entry : std::filesystem::directory_iterator(directoryPath)) {
            if (entry.is_regular_file()) {
                std::string fileName = entry.path().string();
                
                // 检查文件名是否符合日志文件格式
                if (fileName.find("locations_") != std::string::npos && 
                    fileName.find(".log") != std::string::npos) {
                    logFiles.push_back(fileName);
                }
            }
        }
        
        // 如果需要按时间排序
        if (sortByTime) {
            std::sort(logFiles.begin(), logFiles.end(), 
                [](const std::string& a, const std::string& b) {
                    // 按修改时间降序排序（最新的文件在前）
                    return std::filesystem::last_write_time(a) > std::filesystem::last_write_time(b);
                });
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Failed to get log files in directory: %s", e.what());
    }
    
    return logFiles;
}

// StorageManager构造函数
StorageManager::StorageManager() : 
    defaultStorage(nullptr),
    namedStorages() {
}

// 获取StorageManager单例
StorageManager& StorageManager::getInstance() {
    static StorageManager instance;
    return instance;
}

// 注册存储实现
bool StorageManager::registerStorage(const std::string& name, std::shared_ptr<DataStorage> storage) {
    if (!storage) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // 检查是否已存在同名存储
    if (namedStorages.find(name) != namedStorages.end()) {
        LOG_WARNING("Storage with name '%s' already exists", name.c_str());
        return false;
    }
    
    // 注册存储
    namedStorages[name] = storage;
    
    // 如果是第一个注册的存储，设为默认存储
    if (!defaultStorage) {
        defaultStorage = storage;
    }
    
    LOG_INFO("Storage '%s' registered successfully", name.c_str());
    return true;
}

// 注销存储实现
bool StorageManager::unregisterStorage(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = namedStorages.find(name);
    if (it == namedStorages.end()) {
        LOG_WARNING("Storage with name '%s' not found", name.c_str());
        return false;
    }
    
    // 如果是默认存储，取消默认设置
    if (it->second == defaultStorage) {
        defaultStorage = nullptr;
        // 如果还有其他存储，设第一个为默认
        if (!namedStorages.empty()) {
            defaultStorage = namedStorages.begin()->second;
        }
    }
    
    // 注销存储
    namedStorages.erase(it);
    LOG_INFO("Storage '%s' unregistered successfully", name.c_str());
    return true;
}

// 获取指定名称的存储实现
std::shared_ptr<DataStorage> StorageManager::getStorage(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = namedStorages.find(name);
    if (it != namedStorages.end()) {
        return it->second;
    }
    
    return nullptr;
}

// 获取默认存储实现
std::shared_ptr<DataStorage> StorageManager::getDefaultStorage() {
    std::lock_guard<std::mutex> lock(mutex);
    return defaultStorage;
}

// 设置默认存储实现
bool StorageManager::setDefaultStorage(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = namedStorages.find(name);
    if (it == namedStorages.end()) {
        LOG_WARNING("Storage with name '%s' not found", name.c_str());
        return false;
    }
    
    defaultStorage = it->second;
    LOG_INFO("Default storage set to '%s'", name.c_str());
    return true;
}

// 获取所有注册的存储名称
std::vector<std::string> StorageManager::getRegisteredStorages() {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<std::string> names;
    for (const auto& [name, storage] : namedStorages) {
        names.push_back(name);
    }
    
    return names;
}