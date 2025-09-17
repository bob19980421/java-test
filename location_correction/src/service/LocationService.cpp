#include "LocationService.h"
#include "Logger.h"
#include "Utils.h"
#include <chrono>
#include <thread>

namespace location_correction {

// BaseLocationService实现
BaseLocationService::BaseLocationService() {
    isRunning_ = false;
    dataSourceManager_ = DataSourceManager::getInstance();
    processorChain_ = std::make_shared<ProcessorChain>();
    locationCorrector_ = std::make_shared<AdaptiveLocationCorrector>();
    storageManager_ = StorageManager::getInstance();
    Logger::getInstance().info("BaseLocationService initialized");
}

BaseLocationService::~BaseLocationService() {
    if (isRunning_) {
        stop();
    }
    Logger::getInstance().info("BaseLocationService destroyed");
}

bool BaseLocationService::initialize(const LocationServiceConfig& config) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        Logger::getInstance().warning("Cannot initialize service while running");
        return false;
    }
    
    config_ = config;
    
    // 初始化数据源
    if (!initializeDataSources()) {
        Logger::getInstance().error("Failed to initialize data sources");
        return false;
    }
    
    // 初始化处理器链
    if (!initializeProcessorChain()) {
        Logger::getInstance().error("Failed to initialize processor chain");
        return false;
    }
    
    // 初始化位置纠偏器
    if (!initializeLocationCorrector()) {
        Logger::getInstance().error("Failed to initialize location corrector");
        return false;
    }
    
    // 初始化存储
    if (!initializeStorage()) {
        Logger::getInstance().error("Failed to initialize storage");
        return false;
    }
    
    Logger::getInstance().info("BaseLocationService initialized successfully");
    return true;
}

bool BaseLocationService::initializeDataSources() {
    // 根据配置初始化不同的数据源
    if (config_.enableGPS) {
        auto gnssDataSource = std::make_shared<GNSSDataSource>();
        gnssDataSource->setDataUpdateListener(std::bind(&BaseLocationService::onLocationDataReceived, this, std::placeholders::_1));
        dataSourceManager_->registerDataSource(LocationSource::GPS, gnssDataSource);
        Logger::getInstance().info("GPS data source registered");
    }
    
    if (config_.enableWifi) {
        auto wifiDataSource = std::make_shared<WifiDataSource>();
        wifiDataSource->setDataUpdateListener(std::bind(&BaseLocationService::onLocationDataReceived, this, std::placeholders::_1));
        dataSourceManager_->registerDataSource(LocationSource::WIFI, wifiDataSource);
        Logger::getInstance().info("WiFi data source registered");
    }
    
    if (config_.enableBaseStation) {
        auto baseStationDataSource = std::make_shared<BaseStationDataSource>();
        baseStationDataSource->setDataUpdateListener(std::bind(&BaseLocationService::onLocationDataReceived, this, std::placeholders::_1));
        dataSourceManager_->registerDataSource(LocationSource::BASE_STATION, baseStationDataSource);
        Logger::getInstance().info("Base station data source registered");
    }
    
    return true;
}

bool BaseLocationService::initializeProcessorChain() {
    // 创建并添加处理器到链中
    auto accuracyFilter = std::make_shared<AccuracyFilterProcessor>();
    processorChain_->addProcessor(accuracyFilter);
    
    auto timeFilter = std::make_shared<TimeFilterProcessor>();
    processorChain_->addProcessor(timeFilter);
    
    auto outlierDetector = std::make_shared<OutlierDetectionProcessor>();
    processorChain_->addProcessor(outlierDetector);
    
    auto coordinateConverter = std::make_shared<CoordinateConverterProcessor>();
    processorChain_->addProcessor(coordinateConverter);
    
    Logger::getInstance().info("Processor chain initialized with 4 processors");
    return true;
}

bool BaseLocationService::initializeLocationCorrector() {
    // 配置位置纠偏器
    CorrectionConfig correctionConfig;
    correctionConfig.minCorrectionInterval = 500; // 500ms
    correctionConfig.enableAnomalyDetection = true;
    correctionConfig.enableDataFusion = true;
    correctionConfig.enableAdaptiveCorrection = true;
    
    // 添加场景配置
    SceneConfig outdoorConfig;
    outdoorConfig.sceneType = LocationScene::OUTDOOR;
    outdoorConfig.maxSpeed = 120.0;
    outdoorConfig.minAccuracy = 5.0;
    outdoorConfig.weightForGPS = 0.8;
    outdoorConfig.weightForWifi = 0.1;
    outdoorConfig.weightForBaseStation = 0.1;
    correctionConfig.sceneConfigs.push_back(outdoorConfig);
    
    SceneConfig indoorConfig;
    indoorConfig.sceneType = LocationScene::INDOOR;
    indoorConfig.maxSpeed = 5.0;
    indoorConfig.minAccuracy = 10.0;
    indoorConfig.weightForGPS = 0.3;
    indoorConfig.weightForWifi = 0.5;
    indoorConfig.weightForBaseStation = 0.2;
    correctionConfig.sceneConfigs.push_back(indoorConfig);
    
    // 初始化位置纠偏器
    locationCorrector_->initialize(correctionConfig);
    
    Logger::getInstance().info("Location corrector initialized");
    return true;
}

bool BaseLocationService::initializeStorage() {
    // 初始化存储管理器
    if (!storageManager_->initialize()) {
        Logger::getInstance().error("Failed to initialize storage manager");
        return false;
    }
    
    Logger::getInstance().info("Storage initialized");
    return true;
}

bool BaseLocationService::start() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (isRunning_) {
        Logger::getInstance().warning("Service is already running");
        return true;
    }
    
    // 启动所有数据源
    if (!dataSourceManager_->startAllDataSources()) {
        Logger::getInstance().error("Failed to start all data sources");
        return false;
    }
    
    // 创建并启动处理线程
    isRunning_ = true;
    processingThread_ = std::thread(&BaseLocationService::processingLoop, this);
    
    Logger::getInstance().info("BaseLocationService started successfully");
    return true;
}

bool BaseLocationService::stop() {
    std::lock_guard<std::mutex> lock(mutex_);
    
    if (!isRunning_) {
        Logger::getInstance().warning("Service is not running");
        return true;
    }
    
    // 停止处理线程
    isRunning_ = false;
    if (processingThread_.joinable()) {
        processingThread_.join();
    }
    
    // 停止所有数据源
    dataSourceManager_->stopAllDataSources();
    
    // 清空位置数据队列
    {}
    std::lock_guard<std::mutex> queueLock(queueMutex_);
    locationDataQueue_.clear();
    
    Logger::getInstance().info("BaseLocationService stopped successfully");
    return true;
}

std::shared_ptr<LocationInfo> BaseLocationService::getCurrentLocation() {
    std::lock_guard<std::mutex> lock(locationMutex_);
    
    if (lastLocation_) {
        return lastLocation_;
    }
    
    Logger::getInstance().warning("No location data available");
    return nullptr;
}

std::vector<std::shared_ptr<LocationInfo>> BaseLocationService::getLocationHistory(int count) {
    return storageManager_->queryLocations(count);
}

void BaseLocationService::setLocationUpdateListener(LocationUpdateListener listener) {
    std::lock_guard<std::mutex> lock(listenerMutex_);
    locationUpdateListener_ = listener;
    Logger::getInstance().info("Location update listener set");
}

void BaseLocationService::onLocationDataReceived(const LocationInfo& location) {
    std::lock_guard<std::mutex> lock(queueMutex_);
    locationDataQueue_.push_back(location);
    
    // 限制队列大小，防止内存溢出
    if (locationDataQueue_.size() > config_.maxQueueSize) {
        locationDataQueue_.pop_front();
    }
}

void BaseLocationService::processingLoop() {
    Logger::getInstance().info("Processing loop started");
    
    while (isRunning_) {
        // 检查队列中是否有数据
        LocationInfo location;
        bool hasData = false;
        
        {}
        std::lock_guard<std::mutex> queueLock(queueMutex_);
        if (!locationDataQueue_.empty()) {
            location = locationDataQueue_.front();
            locationDataQueue_.pop_front();
            hasData = true;
        }
        
        if (hasData) {
            // 处理位置数据
            processLocationData(location);
        } else {
            // 队列为空，短暂休眠
            std::this_thread::sleep_for(std::chrono::milliseconds(50));
        }
    }
    
    Logger::getInstance().info("Processing loop stopped");
}

void BaseLocationService::processLocationData(const LocationInfo& location) {
    // 1. 预处理数据
    auto processedLocation = processorChain_->process(location);
    if (!processedLocation) {
        Logger::getInstance().debug("Location data filtered out during preprocessing");
        return;
    }
    
    // 2. 执行位置纠偏
    auto correctedLocation = locationCorrector_->correctLocation(*processedLocation);
    if (!correctedLocation) {
        Logger::getInstance().debug("Location correction skipped");
        return;
    }
    
    // 3. 存储纠偏后的数据
    if (config_.enableHistoryStorage) {
        storageManager_->storeLocation(correctedLocation->toLocationInfo());
    }
    
    // 4. 更新最新位置
    {}
    std::lock_guard<std::mutex> locationLock(locationMutex_);
    lastLocation_ = std::make_shared<LocationInfo>(correctedLocation->toLocationInfo());
    
    // 5. 通知监听器
    {}
    std::lock_guard<std::mutex> listenerLock(listenerMutex_);
    if (locationUpdateListener_) {
        locationUpdateListener_(*lastLocation_);
    }
    
    Logger::getInstance().debug("Location data processed successfully: " + 
        std::to_string(correctedLocation->latitude) + ", " + 
        std::to_string(correctedLocation->longitude) + ", accuracy: " + 
        std::to_string(correctedLocation->accuracy));
}

// HighPerformanceLocationService实现
HighPerformanceLocationService::HighPerformanceLocationService() : BaseLocationService() {
    cacheSize_ = 100; // 默认缓存100个位置
    batchProcessingSize_ = 10; // 批处理大小
    Logger::getInstance().info("HighPerformanceLocationService initialized");
}

HighPerformanceLocationService::~HighPerformanceLocationService() {
    Logger::getInstance().info("HighPerformanceLocationService destroyed");
}

bool HighPerformanceLocationService::initialize(const LocationServiceConfig& config) {
    // 设置高性能相关配置
    cacheSize_ = config.cacheSize > 0 ? config.cacheSize : cacheSize_;
    batchProcessingSize_ = config.batchProcessingSize > 0 ? config.batchProcessingSize : batchProcessingSize_;
    
    // 调用基类初始化
    bool result = BaseLocationService::initialize(config);
    
    if (result) {
        Logger::getInstance().info("HighPerformanceLocationService initialized with cache size: " + std::to_string(cacheSize_) + ", batch size: " + std::to_string(batchProcessingSize_));
    }
    
    return result;
}

std::shared_ptr<LocationInfo> HighPerformanceLocationService::getCurrentLocation() {
    // 首先检查缓存中是否有最新位置
    std::lock_guard<std::mutex> cacheLock(cacheMutex_);
    if (!locationCache_.empty()) {
        return locationCache_.back();
    }
    
    // 如果缓存为空，调用基类方法
    return BaseLocationService::getCurrentLocation();
}

void HighPerformanceLocationService::onLocationDataReceived(const LocationInfo& location) {
    // 将位置数据添加到批处理队列，而不是直接处理
    std::lock_guard<std::mutex> batchLock(batchMutex_);
    batchLocationQueue_.push_back(location);
    
    // 当队列达到批处理大小时，一次性处理
    if (batchLocationQueue_.size() >= batchProcessingSize_) {
        // 创建一个副本进行处理，避免长时间持有锁
        std::vector<LocationInfo> batchData;
        batchData.swap(batchLocationQueue_);
        
        // 异步处理批数据
        std::thread batchThread(&HighPerformanceLocationService::processBatchData, this, batchData);
        batchThread.detach();
    }
}

void HighPerformanceLocationService::processBatchData(const std::vector<LocationInfo>& batchData) {
    std::vector<std::shared_ptr<LocationInfo>> processedLocations;
    
    // 预处理所有位置数据
    for (const auto& location : batchData) {
        auto processedLocation = processorChain_->process(location);
        if (processedLocation) {
            processedLocations.push_back(processedLocation);
        }
    }
    
    // 如果有处理后的位置数据，执行纠偏
    if (!processedLocations.empty()) {
        for (const auto& processedLocation : processedLocations) {
            auto correctedLocation = locationCorrector_->correctLocation(*processedLocation);
            if (correctedLocation) {
                // 更新缓存
                updateLocationCache(correctedLocation->toLocationInfo());
                
                // 存储数据
                if (config_.enableHistoryStorage) {
                    storageManager_->storeLocation(correctedLocation->toLocationInfo());
                }
                
                // 更新最新位置
                {}
                std::lock_guard<std::mutex> locationLock(locationMutex_);
                lastLocation_ = std::make_shared<LocationInfo>(correctedLocation->toLocationInfo());
                
                // 通知监听器
                {}
                std::lock_guard<std::mutex> listenerLock(listenerMutex_);
                if (locationUpdateListener_) {
                    locationUpdateListener_(*lastLocation_);
                }
            }
        }
    }
    
    Logger::getInstance().debug("Batch processed " + std::to_string(batchData.size()) + " locations, " + std::to_string(processedLocations.size()) + " processed successfully");
}

void HighPerformanceLocationService::updateLocationCache(const LocationInfo& location) {
    std::lock_guard<std::mutex> cacheLock(cacheMutex_);
    
    // 添加到缓存
    locationCache_.push_back(std::make_shared<LocationInfo>(location));
    
    // 限制缓存大小
    while (locationCache_.size() > cacheSize_) {
        locationCache_.erase(locationCache_.begin());
    }
}

// LocationServiceFactory实现
LocationServiceFactory::LocationServiceFactory() {
    Logger::getInstance().info("LocationServiceFactory initialized");
}

LocationServiceFactory::~LocationServiceFactory() {
    Logger::getInstance().info("LocationServiceFactory destroyed");
}

LocationServiceFactory& LocationServiceFactory::getInstance() {
    static LocationServiceFactory instance;
    return instance;
}

std::shared_ptr<LocationService> LocationServiceFactory::createLocationService(ServiceType type) {
    std::shared_ptr<LocationService> service;
    
    switch (type) {
        case ServiceType::BASIC:
            service = std::make_shared<BaseLocationService>();
            Logger::getInstance().info("Basic location service created");
            break;
        case ServiceType::HIGH_PERFORMANCE:
            service = std::make_shared<HighPerformanceLocationService>();
            Logger::getInstance().info("High performance location service created");
            break;
        default:
            service = std::make_shared<BaseLocationService>();
            Logger::getInstance().warning("Unknown service type, created basic location service");
            break;
    }
    
    return service;
}

} // namespace location_correction