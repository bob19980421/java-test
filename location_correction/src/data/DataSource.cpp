// DataSource.cpp - 定位数据源实现

#include "DataSource.h"
#include "Logger.h"
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <thread>

// DataSource构造函数
DataSource::DataSource(DataSourceType type) : 
    dataSourceType(type),
    enabled(false),
    listeners(),
    mutex(),
    dataCollecting(false),
    dataCollectionThread(nullptr),
    dataCollectionInterval(1000), // 默认1秒
    lastLocation(nullptr) {
}

// DataSource析构函数
DataSource::~DataSource() {
    stopDataCollection();
}

// 启动数据收集
bool DataSource::start() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (enabled) {
        LOG_WARNING("Data source already started: %d", static_cast<int>(dataSourceType));
        return true;
    }
    
    enabled = true;
    
    // 启动数据收集线程
    if (!dataCollecting) {
        dataCollecting = true;
        dataCollectionThread = new std::thread(&DataSource::dataCollectionTask, this);
    }
    
    LOG_INFO("Data source started: %d", static_cast<int>(dataSourceType));
    return true;
}

// 停止数据收集
void DataSource::stop() {
    std::lock_guard<std::mutex> lock(mutex);
    
    if (!enabled) {
        return;
    }
    
    enabled = false;
    stopDataCollection();
    
    LOG_INFO("Data source stopped: %d", static_cast<int>(dataSourceType));
}

// 停止数据收集线程
void DataSource::stopDataCollection() {
    if (dataCollecting) {
        dataCollecting = false;
        
        if (dataCollectionThread != nullptr && dataCollectionThread->joinable()) {
            dataCollectionThread->join();
            delete dataCollectionThread;
            dataCollectionThread = nullptr;
        }
    }
}

// 数据收集任务（虚函数，子类需要实现具体的收集逻辑）
void DataSource::dataCollectionTask() {
    // 基类实现为空，子类需要重写此方法
    while (dataCollecting) {
        std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    }
}

// 添加位置更新监听器
void DataSource::addLocationListener(std::shared_ptr<LocationChangeListener> listener) {
    if (!listener) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // 检查监听器是否已经存在
    auto it = std::find(listeners.begin(), listeners.end(), listener);
    if (it == listeners.end()) {
        listeners.push_back(listener);
    }
}

// 移除位置更新监听器
void DataSource::removeLocationListener(std::shared_ptr<LocationChangeListener> listener) {
    if (!listener) {
        return;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    
    // 查找并移除监听器
    auto it = std::find(listeners.begin(), listeners.end(), listener);
    if (it != listeners.end()) {
        listeners.erase(it);
    }
}

// 通知所有监听器位置更新
void DataSource::notifyLocationUpdate(const LocationInfo& location) {
    std::vector<std::shared_ptr<LocationChangeListener>> listenersCopy;
    
    {{
        std::lock_guard<std::mutex> lock(mutex);
        listenersCopy = listeners;
        
        // 保存最新位置
        lastLocation = std::make_shared<LocationInfo>(location);
    }}
    
    // 在锁外通知监听器，避免死锁
    for (const auto& listener : listenersCopy) {
        try {
            listener->onLocationChanged(location);
        } catch (const std::exception& e) {
            LOG_ERROR("Exception in location listener: %s", e.what());
        }
    }
}

// 获取最新位置
std::shared_ptr<LocationInfo> DataSource::getLastLocation() const {
    std::lock_guard<std::mutex> lock(mutex);
    return lastLocation;
}

// 设置数据收集间隔
void DataSource::setDataCollectionInterval(long long intervalMs) {
    std::lock_guard<std::mutex> lock(mutex);
    dataCollectionInterval = intervalMs;
}

// 获取数据源类型
DataSourceType DataSource::getDataSourceType() const {
    return dataSourceType;
}

// 检查数据源是否启用
bool DataSource::isEnabled() const {
    std::lock_guard<std::mutex> lock(mutex);
    return enabled;
}

// GNSSDataSource构造函数
GNSSDataSource::GNSSDataSource() : DataSource(DataSourceType::GPS) {
    // 初始化GNSS相关参数
    minSatelliteCount = 4;
    maxErrorThreshold = 100.0; // 默认最大误差100米
    enableSatelliteFiltering = true;
    satelliteSystemMask = GNSS_SYSTEM_GPS | GNSS_SYSTEM_GLONASS | GNSS_SYSTEM_BEIDOU | GNSS_SYSTEM_GALILEO;
}

// 重写数据收集任务
void GNSSDataSource::dataCollectionTask() {
    LOG_INFO("Starting GNSS data collection task");
    
    while (dataCollecting) {
        try {
            // 模拟GNSS数据采集
            LocationInfo location = collectGNSSData();
            
            // 如果位置有效，通知更新
            if (location.isValid()) {
                notifyLocationUpdate(location);
                LOG_DEBUG("GNSS location updated: %s", location.toString().c_str());
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Error in GNSS data collection: %s", e.what());
        }
        
        // 等待指定的采集间隔
        std::this_thread::sleep_for(std::chrono::milliseconds(dataCollectionInterval));
    }
    
    LOG_INFO("GNSS data collection task stopped");
}

// 采集GNSS数据（在实际应用中，这里应该调用系统的GNSS API）
LocationInfo GNSSDataSource::collectGNSSData() {
    // 模拟GNSS数据
    // 注意：在实际应用中，这里应该调用设备的GNSS API获取真实位置数据
    LocationInfo location;
    
    // 随机生成一个位置（仅用于示例）
    // 在真实应用中，应该通过系统API获取位置
    location.latitude = 39.9042 + (rand() % 1000 - 500) / 100000.0; // 北京附近
    location.longitude = 116.4074 + (rand() % 1000 - 500) / 100000.0;
    location.accuracy = 5.0 + rand() % 100 / 10.0; // 5-15米的精度
    location.timestamp = getCurrentTimestampMs();
    location.dataSource = DataSourceType::GPS;
    location.status = LocationStatus::VALID;
    
    // 随机生成卫星数量
    location.satelliteCount = 4 + rand() % 12; // 4-16颗卫星
    location.signalStrength = 20 + rand() % 80; // 信号强度20-100
    
    // 根据卫星数量和精度过滤无效数据
    if (enableSatelliteFiltering && location.satelliteCount < minSatelliteCount) {
        location.status = LocationStatus::LOW_ACCURACY;
    }
    
    if (location.accuracy > maxErrorThreshold) {
        location.status = LocationStatus::LOW_ACCURACY;
    }
    
    return location;
}

// 设置最小卫星数量
void GNSSDataSource::setMinSatelliteCount(int count) {
    std::lock_guard<std::mutex> lock(mutex);
    minSatelliteCount = std::max(0, count);
}

// 设置最大误差阈值
void GNSSDataSource::setMaxErrorThreshold(double threshold) {
    std::lock_guard<std::mutex> lock(mutex);
    maxErrorThreshold = std::max(0.0, threshold);
}

// WifiDataSource构造函数
WifiDataSource::WifiDataSource() : DataSource(DataSourceType::WIFI) {
    // 初始化WiFi相关参数
    scanInterval = 5000; // 默认5秒扫描一次
    minRssiThreshold = -85; // RSSI阈值
    enableBSSIDFiltering = false;
}

// 重写数据收集任务
void WifiDataSource::dataCollectionTask() {
    LOG_INFO("Starting WiFi data collection task");
    
    while (dataCollecting) {
        try {
            // 模拟WiFi数据采集
            LocationInfo location = collectWifiData();
            
            // 如果位置有效，通知更新
            if (location.isValid()) {
                notifyLocationUpdate(location);
                LOG_DEBUG("WiFi location updated: %s", location.toString().c_str());
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Error in WiFi data collection: %s", e.what());
        }
        
        // 等待指定的采集间隔
        std::this_thread::sleep_for(std::chrono::milliseconds(dataCollectionInterval));
    }
    
    LOG_INFO("WiFi data collection task stopped");
}

// 采集WiFi数据（在实际应用中，这里应该调用系统的WiFi API）
LocationInfo WifiDataSource::collectWifiData() {
    // 模拟WiFi位置数据
    // 注意：在实际应用中，这里应该通过系统API获取WiFi信息并进行位置计算
    LocationInfo location;
    
    // 随机生成一个位置（仅用于示例）
    location.latitude = 39.9042 + (rand() % 2000 - 1000) / 100000.0; // 北京附近，误差范围更大
    location.longitude = 116.4074 + (rand() % 2000 - 1000) / 100000.0;
    location.accuracy = 10.0 + rand() % 100; // 10-110米的精度
    location.timestamp = getCurrentTimestampMs();
    location.dataSource = DataSourceType::WIFI;
    location.status = LocationStatus::VALID;
    
    // 设置WiFi特有信息
    location.setExtra("BSSID", generateUUID().substr(0, 17)); // 模拟BSSID
    location.setExtra("SSID", "WiFi-" + std::to_string(rand() % 1000)); // 模拟SSID
    location.setExtra("RSSI", std::to_string(minRssiThreshold + rand() % 50)); // 模拟信号强度
    
    // 根据信号强度过滤无效数据
    int rssi = parseDouble(location.getExtra("RSSI", "0"), 0);
    if (rssi < minRssiThreshold) {
        location.status = LocationStatus::LOW_ACCURACY;
    }
    
    return location;
}

// 设置最小RSSI阈值
void WifiDataSource::setMinRssiThreshold(int threshold) {
    std::lock_guard<std::mutex> lock(mutex);
    minRssiThreshold = threshold;
}

// BaseStationDataSource构造函数
BaseStationDataSource::BaseStationDataSource() : DataSource(DataSourceType::BASE_STATION) {
    // 初始化基站相关参数
    minSignalStrength = -100; // 信号强度阈值
    useLAC = true; // 使用位置区域码
    useMCC = true; // 使用移动国家码
    useMNC = true; // 使用移动网络码
}

// 重写数据收集任务
void BaseStationDataSource::dataCollectionTask() {
    LOG_INFO("Starting base station data collection task");
    
    while (dataCollecting) {
        try {
            // 模拟基站数据采集
            LocationInfo location = collectBaseStationData();
            
            // 如果位置有效，通知更新
            if (location.isValid()) {
                notifyLocationUpdate(location);
                LOG_DEBUG("Base station location updated: %s", location.toString().c_str());
            }
            
        } catch (const std::exception& e) {
            LOG_ERROR("Error in base station data collection: %s", e.what());
        }
        
        // 等待指定的采集间隔
        std::this_thread::sleep_for(std::chrono::milliseconds(dataCollectionInterval));
    }
    
    LOG_INFO("Base station data collection task stopped");
}

// 采集基站数据（在实际应用中，这里应该调用系统的基站API）
LocationInfo BaseStationDataSource::collectBaseStationData() {
    // 模拟基站位置数据
    // 注意：在实际应用中，这里应该通过系统API获取基站信息并进行位置计算
    LocationInfo location;
    
    // 随机生成一个位置（仅用于示例）
    location.latitude = 39.9042 + (rand() % 5000 - 2500) / 100000.0; // 北京附近，误差范围最大
    location.longitude = 116.4074 + (rand() % 5000 - 2500) / 100000.0;
    location.accuracy = 50.0 + rand() % 500; // 50-550米的精度
    location.timestamp = getCurrentTimestampMs();
    location.dataSource = DataSourceType::BASE_STATION;
    location.status = LocationStatus::VALID;
    
    // 设置基站特有信息
    location.setExtra("MCC", "460"); // 中国移动国家码
    location.setExtra("MNC", std::to_string(rand() % 3)); // 模拟移动网络码
    location.setExtra("LAC", std::to_string(10000 + rand() % 20000)); // 模拟位置区域码
    location.setExtra("CID", std::to_string(10000000 + rand() % 50000000)); // 模拟小区ID
    location.setExtra("RSSI", std::to_string(minSignalStrength + rand() % 40)); // 模拟信号强度
    
    // 根据信号强度过滤无效数据
    int rssi = parseDouble(location.getExtra("RSSI", "0"), 0);
    if (rssi < minSignalStrength) {
        location.status = LocationStatus::LOW_ACCURACY;
    }
    
    return location;
}

// 设置最小信号强度阈值
void BaseStationDataSource::setMinSignalStrength(int strength) {
    std::lock_guard<std::mutex> lock(mutex);
    minSignalStrength = strength;
}

// DataSourceManager单例初始化
std::shared_ptr<DataSourceManager> DataSourceManager::instance = nullptr;
std::mutex DataSourceManager::instanceMutex;

// 获取DataSourceManager单例实例
std::shared_ptr<DataSourceManager> DataSourceManager::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (instance == nullptr) {
        instance = std::shared_ptr<DataSourceManager>(new DataSourceManager());
    }
    return instance;
}

// DataSourceManager构造函数
DataSourceManager::DataSourceManager() : dataSources(), mutex() {
    // 初始化时可以创建一些默认的数据源
    addDefaultDataSources();
}

// 添加默认数据源
void DataSourceManager::addDefaultDataSources() {
    // 创建并添加常见的数据源
    addDataSource(std::make_shared<GNSSDataSource>());
    addDataSource(std::make_shared<WifiDataSource>());
    addDataSource(std::make_shared<BaseStationDataSource>());
}

// 添加数据源
bool DataSourceManager::addDataSource(std::shared_ptr<DataSource> source) {
    if (!source) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    DataSourceType type = source->getDataSourceType();
    
    // 检查是否已存在该类型的数据源
    auto it = dataSources.find(type);
    if (it != dataSources.end()) {
        LOG_WARNING("DataSource of type %d already exists", static_cast<int>(type));
        return false;
    }
    
    // 添加数据源
    dataSources[type] = source;
    LOG_INFO("Added data source: %d", static_cast<int>(type));
    
    return true;
}

// 移除数据源
bool DataSourceManager::removeDataSource(DataSourceType type) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = dataSources.find(type);
    if (it == dataSources.end()) {
        LOG_WARNING("DataSource of type %d not found", static_cast<int>(type));
        return false;
    }
    
    // 停止数据源
    it->second->stop();
    
    // 移除数据源
    dataSources.erase(it);
    LOG_INFO("Removed data source: %d", static_cast<int>(type));
    
    return true;
}

// 获取数据源
std::shared_ptr<DataSource> DataSourceManager::getDataSource(DataSourceType type) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = dataSources.find(type);
    if (it != dataSources.end()) {
        return it->second;
    }
    
    LOG_WARNING("DataSource of type %d not found", static_cast<int>(type));
    return nullptr;
}

// 启动指定类型的数据源
bool DataSourceManager::startDataSource(DataSourceType type) {
    auto source = getDataSource(type);
    if (!source) {
        return false;
    }
    
    return source->start();
}

// 停止指定类型的数据源
bool DataSourceManager::stopDataSource(DataSourceType type) {
    auto source = getDataSource(type);
    if (!source) {
        return false;
    }
    
    source->stop();
    return true;
}

// 启动所有数据源
bool DataSourceManager::startAllDataSources() {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& pair : dataSources) {
        pair.second->start();
    }
    
    return true;
}

// 停止所有数据源
bool DataSourceManager::stopAllDataSources() {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (auto& pair : dataSources) {
        pair.second->stop();
    }
    
    return true;
}

// 获取所有可用的数据源类型
std::vector<DataSourceType> DataSourceManager::getAvailableDataSourceTypes() {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<DataSourceType> types;
    for (const auto& pair : dataSources) {
        types.push_back(pair.first);
    }
    
    return types;
}

// 获取所有活跃的数据源类型
std::vector<DataSourceType> DataSourceManager::getActiveDataSourceTypes() {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<DataSourceType> types;
    for (const auto& pair : dataSources) {
        if (pair.second->isEnabled()) {
            types.push_back(pair.first);
        }
    }
    
    return types;
}

// 获取最新的位置数据（从所有活跃数据源中）
std::vector<std::shared_ptr<LocationInfo>> DataSourceManager::getLatestLocations() {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<std::shared_ptr<LocationInfo>> locations;
    
    for (const auto& pair : dataSources) {
        if (pair.second->isEnabled()) {
            auto location = pair.second->getLastLocation();
            if (location && location->isValid()) {
                locations.push_back(location);
            }
        }
    }
    
    return locations;
}