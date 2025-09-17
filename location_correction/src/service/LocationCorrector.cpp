#include "LocationCorrector.h"
#include "Logger.h"
#include "Utils.h"
#include <algorithm>
#include <chrono>
#include <thread>

namespace location_correction {

// BaseLocationCorrector实现
BaseLocationCorrector::BaseLocationCorrector() {
    config_ = std::make_shared<CorrectionConfig>();
    anomalyDetector_ = nullptr;
    dataFusion_ = nullptr;
    lastCorrectionTime_ = 0;
    Logger::getInstance().info("BaseLocationCorrector initialized");
}

BaseLocationCorrector::~BaseLocationCorrector() {
    Logger::getInstance().info("BaseLocationCorrector destroyed");
}

void BaseLocationCorrector::initialize(const CorrectionConfig& config) {
    config_ = std::make_shared<CorrectionConfig>(config);
    Logger::getInstance().info("BaseLocationCorrector configured");
}

void BaseLocationCorrector::setAnomalyDetector(std::shared_ptr<AnomalyDetector> detector) {
    anomalyDetector_ = detector;
}

void BaseLocationCorrector::setDataFusion(std::shared_ptr<DataFusion> fusion) {
    dataFusion_ = fusion;
}

std::shared_ptr<CorrectedLocation> BaseLocationCorrector::correctLocation(const LocationInfo& location) {
    auto startTime = std::chrono::system_clock::now();
    auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
        startTime.time_since_epoch()).count();
    
    // 检查是否需要进行位置纠偏（基于时间阈值）
    if (timestamp - lastCorrectionTime_ < config_->minCorrectionInterval) {
        Logger::getInstance().debug("Location correction skipped due to time interval");
        return nullptr;
    }
    
    // 创建待返回的纠偏位置对象
    auto correctedLocation = std::make_shared<CorrectedLocation>();
    correctedLocation->originalLocation = location;
    correctedLocation->correctionTime = timestamp;
    
    // 默认情况下，直接使用原始位置作为纠偏结果
    correctedLocation->latitude = location.latitude;
    correctedLocation->longitude = location.longitude;
    correctedLocation->accuracy = location.accuracy;
    correctedLocation->altitude = location.altitude;
    correctedLocation->speed = location.speed;
    correctedLocation->direction = location.direction;
    correctedLocation->sourceType = location.sourceType;
    correctedLocation->timestamp = location.timestamp;
    
    // 标记为已处理
    correctedLocation->processed = true;
    
    // 更新最后纠偏时间
    lastCorrectionTime_ = timestamp;
    
    Logger::getInstance().debug("Base location correction completed");
    return correctedLocation;
}

// AdaptiveLocationCorrector实现
AdaptiveLocationCorrector::AdaptiveLocationCorrector() : BaseLocationCorrector() {
    currentScene_ = LocationScene::UNKNOWN;
    sceneCheckInterval = 10000; // 默认10秒检查一次场景
    lastSceneCheckTime_ = 0;
    Logger::getInstance().info("AdaptiveLocationCorrector initialized");
}

AdaptiveLocationCorrector::~AdaptiveLocationCorrector() {
    Logger::getInstance().info("AdaptiveLocationCorrector destroyed");
}

void AdaptiveLocationCorrector::initialize(const CorrectionConfig& config) {
    BaseLocationCorrector::initialize(config);
    
    // 初始化场景配置
    if (config.sceneConfigs.empty()) {
        // 如果没有提供场景配置，创建默认配置
        SceneConfig defaultConfig;
        defaultConfig.sceneType = LocationScene::OUTDOOR;
        defaultConfig.maxSpeed = 120.0; // 120km/h
        defaultConfig.minAccuracy = 5.0; // 5米
        defaultConfig.weightForGPS = 0.8;
        defaultConfig.weightForWifi = 0.1;
        defaultConfig.weightForBaseStation = 0.1;
        
        SceneConfig indoorConfig;
        indoorConfig.sceneType = LocationScene::INDOOR;
        indoorConfig.maxSpeed = 5.0; // 5km/h
        indoorConfig.minAccuracy = 10.0; // 10米
        indoorConfig.weightForGPS = 0.3;
        indoorConfig.weightForWifi = 0.5;
        indoorConfig.weightForBaseStation = 0.2;
        
        sceneConfigs_[LocationScene::OUTDOOR] = defaultConfig;
        sceneConfigs_[LocationScene::INDOOR] = indoorConfig;
        
        Logger::getInstance().info("AdaptiveLocationCorrector: Default scene configurations created");
    } else {
        // 使用提供的场景配置
        for (const auto& sceneConfig : config.sceneConfigs) {
            sceneConfigs_[sceneConfig.sceneType] = sceneConfig;
        }
        Logger::getInstance().info("AdaptiveLocationCorrector: Scene configurations loaded");
    }
}

void AdaptiveLocationCorrector::addSceneConfig(const SceneConfig& sceneConfig) {
    std::lock_guard<std::mutex> lock(mutex_);
    sceneConfigs_[sceneConfig.sceneType] = sceneConfig;
    Logger::getInstance().info("Scene configuration added for type: " + std::to_string(static_cast<int>(sceneConfig.sceneType)));
}

LocationScene AdaptiveLocationCorrector::detectScene(const LocationInfo& location) {
    auto currentTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    
    // 检查是否需要进行场景检测
    if (currentTime - lastSceneCheckTime_ < sceneCheckInterval) {
        return currentScene_;
    }
    
    // 更新场景检查时间
    lastSceneCheckTime_ = currentTime;
    
    // 基于位置信息和速度检测场景
    LocationScene detectedScene = LocationScene::UNKNOWN;
    
    // 基本场景检测逻辑：根据速度判断
    if (location.speed > 10.0) { // >10km/h 认为是室外移动场景
        detectedScene = LocationScene::OUTDOOR;
    } else if (location.speed <= 10.0 && location.speed >= 0) { // <=10km/h 可能是室内或室外静止
        // 检查GPS信号质量
        if (location.sourceType == LocationSource::GPS && location.accuracy < 10.0) {
            detectedScene = LocationScene::OUTDOOR;
        } else {
            detectedScene = LocationScene::INDOOR;
        }
    } else {
        // 无法确定，保持原有场景
        detectedScene = currentScene_;
    }
    
    // 如果检测到新场景，更新当前场景并记录日志
    if (detectedScene != currentScene_) {
        currentScene_ = detectedScene;
        Logger::getInstance().info("Scene changed to: " + getSceneName(detectedScene));
    }
    
    return detectedScene;
}

std::string AdaptiveLocationCorrector::getSceneName(LocationScene scene) {
    switch (scene) {
        case LocationScene::INDOOR:
            return "INDOOR";
        case LocationScene::OUTDOOR:
            return "OUTDOOR";
        case LocationScene::UNDERGROUND:
            return "UNDERGROUND";
        case LocationScene::HIGHWAY:
            return "HIGHWAY";
        case LocationScene::URBAN_CANYON:
            return "URBAN_CANYON";
        default:
            return "UNKNOWN";
    }
}

std::shared_ptr<CorrectedLocation> AdaptiveLocationCorrector::correctLocation(const LocationInfo& location) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 检测当前场景
    LocationScene currentScene = detectScene(location);
    
    // 检查是否有该场景的配置
    auto sceneConfigIt = sceneConfigs_.find(currentScene);
    if (sceneConfigIt == sceneConfigs_.end()) {
        Logger::getInstance().warning("No configuration found for current scene, using base correction");
        return BaseLocationCorrector::correctLocation(location);
    }
    
    const SceneConfig& sceneConfig = sceneConfigIt->second;
    
    // 创建纠偏位置对象
    auto correctedLocation = std::make_shared<CorrectedLocation>();
    correctedLocation->originalLocation = location;
    correctedLocation->correctionTime = std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
    correctedLocation->sceneType = currentScene;
    
    // 根据场景配置应用不同的纠偏策略
    if (currentScene == LocationScene::INDOOR) {
        // 室内场景：降低GPS权重，增加WiFi和基站权重
        applyIndoorCorrection(location, *correctedLocation, sceneConfig);
    } else if (currentScene == LocationScene::OUTDOOR) {
        // 室外场景：增加GPS权重
        applyOutdoorCorrection(location, *correctedLocation, sceneConfig);
    } else if (currentScene == LocationScene::URBAN_CANYON) {
        // 城市峡谷：可能需要更复杂的处理，如多路径效应消除
        applyUrbanCanyonCorrection(location, *correctedLocation, sceneConfig);
    } else if (currentScene == LocationScene::HIGHWAY) {
        // 高速公路：更注重速度平滑和预测
        applyHighwayCorrection(location, *correctedLocation, sceneConfig);
    } else {
        // 其他场景使用基本纠偏
        return BaseLocationCorrector::correctLocation(location);
    }
    
    // 更新最后纠偏时间
    lastCorrectionTime_ = correctedLocation->correctionTime;
    
    Logger::getInstance().debug("Adaptive location correction completed for scene: " + getSceneName(currentScene));
    return correctedLocation;
}

void AdaptiveLocationCorrector::applyIndoorCorrection(const LocationInfo& location, CorrectedLocation& correctedLocation, const SceneConfig& config) {
    // 复制原始位置信息
    correctedLocation.latitude = location.latitude;
    correctedLocation.longitude = location.longitude;
    correctedLocation.altitude = location.altitude;
    correctedLocation.speed = location.speed;
    correctedLocation.direction = location.direction;
    correctedLocation.sourceType = location.sourceType;
    correctedLocation.timestamp = location.timestamp;
    
    // 室内场景特殊处理：降低GPS精度要求
    if (location.sourceType == LocationSource::GPS) {
        // 室内GPS信号可能较弱，适当放宽精度要求
        correctedLocation.accuracy = std::max(location.accuracy, config.minAccuracy);
    } else {
        correctedLocation.accuracy = location.accuracy;
    }
    
    // 标记为已处理
    correctedLocation.processed = true;
}

void AdaptiveLocationCorrector::applyOutdoorCorrection(const LocationInfo& location, CorrectedLocation& correctedLocation, const SceneConfig& config) {
    // 复制原始位置信息
    correctedLocation.latitude = location.latitude;
    correctedLocation.longitude = location.longitude;
    correctedLocation.altitude = location.altitude;
    correctedLocation.speed = location.speed;
    correctedLocation.direction = location.direction;
    correctedLocation.sourceType = location.sourceType;
    correctedLocation.timestamp = location.timestamp;
    
    // 室外场景特殊处理：提高精度要求
    correctedLocation.accuracy = location.accuracy;
    
    // 标记为已处理
    correctedLocation.processed = true;
}

void AdaptiveLocationCorrector::applyUrbanCanyonCorrection(const LocationInfo& location, CorrectedLocation& correctedLocation, const SceneConfig& config) {
    // 默认使用室外纠偏，实际项目中可以添加城市峡谷特有的处理逻辑
    applyOutdoorCorrection(location, correctedLocation, config);
    Logger::getInstance().debug("Using outdoor correction for urban canyon scene");
}

void AdaptiveLocationCorrector::applyHighwayCorrection(const LocationInfo& location, CorrectedLocation& correctedLocation, const SceneConfig& config) {
    // 默认使用室外纠偏，实际项目中可以添加高速公路特有的处理逻辑（如速度平滑）
    applyOutdoorCorrection(location, correctedLocation, config);
    Logger::getInstance().debug("Using outdoor correction for highway scene");
}

// MultiModeLocationCorrector实现
MultiModeLocationCorrector::MultiModeLocationCorrector() {
    currentMode_ = CorrectionMode::NORMAL;
    Logger::getInstance().info("MultiModeLocationCorrector initialized");
}

MultiModeLocationCorrector::~MultiModeLocationCorrector() {
    Logger::getInstance().info("MultiModeLocationCorrector destroyed");
}

void MultiModeLocationCorrector::initialize(const CorrectionConfig& config) {
    BaseLocationCorrector::initialize(config);
    Logger::getInstance().info("MultiModeLocationCorrector configured");
}

void MultiModeLocationCorrector::setCorrectionMode(CorrectionMode mode) {
    std::lock_guard<std::mutex> lock(mutex_);
    if (currentMode_ != mode) {
        currentMode_ = mode;
        Logger::getInstance().info("Correction mode changed to: " + getModeName(mode));
    }
}

CorrectionMode MultiModeLocationCorrector::getCorrectionMode() const {
    return currentMode_;
}

std::string MultiModeLocationCorrector::getModeName(CorrectionMode mode) {
    switch (mode) {
        case CorrectionMode::NORMAL:
            return "NORMAL";
        case CorrectionMode::HIGH_ACCURACY:
            return "HIGH_ACCURACY";
        case CorrectionMode::LOW_POWER:
            return "LOW_POWER";
        case CorrectionMode::FAST_UPDATE:
            return "FAST_UPDATE";
        case CorrectionMode::OFFLINE:
            return "OFFLINE";
        default:
            return "UNKNOWN";
    }
}

std::shared_ptr<CorrectedLocation> MultiModeLocationCorrector::correctLocation(const LocationInfo& location) {
    std::lock_guard<std::mutex> lock(mutex_);
    
    // 根据当前模式应用不同的纠偏策略
    switch (currentMode_) {
        case CorrectionMode::NORMAL:
            return applyNormalModeCorrection(location);
        case CorrectionMode::HIGH_ACCURACY:
            return applyHighAccuracyModeCorrection(location);
        case CorrectionMode::LOW_POWER:
            return applyLowPowerModeCorrection(location);
        case CorrectionMode::FAST_UPDATE:
            return applyFastUpdateModeCorrection(location);
        case CorrectionMode::OFFLINE:
            return applyOfflineModeCorrection(location);
        default:
            Logger::getInstance().warning("Unknown correction mode, using normal mode");
            return applyNormalModeCorrection(location);
    }
}

std::shared_ptr<CorrectedLocation> MultiModeLocationCorrector::applyNormalModeCorrection(const LocationInfo& location) {
    // 标准模式：平衡精度和性能
    Logger::getInstance().debug("Applying normal mode correction");
    return BaseLocationCorrector::correctLocation(location);
}

std::shared_ptr<CorrectedLocation> MultiModeLocationCorrector::applyHighAccuracyModeCorrection(const LocationInfo& location) {
    // 高精度模式：优先考虑精度
    Logger::getInstance().debug("Applying high accuracy mode correction");
    
    // 高精度模式下，降低时间间隔要求，增加处理强度
    auto originalInterval = config_->minCorrectionInterval;
    config_->minCorrectionInterval = std::max(100LL, originalInterval / 2); // 至少100ms
    
    auto result = BaseLocationCorrector::correctLocation(location);
    
    // 恢复原始配置
    config_->minCorrectionInterval = originalInterval;
    
    return result;
}

std::shared_ptr<CorrectedLocation> MultiModeLocationCorrector::applyLowPowerModeCorrection(const LocationInfo& location) {
    // 低功耗模式：降低处理频率和强度
    Logger::getInstance().debug("Applying low power mode correction");
    
    // 低功耗模式下，增加时间间隔要求，减少处理强度
    auto originalInterval = config_->minCorrectionInterval;
    config_->minCorrectionInterval = std::max(1000LL, originalInterval * 2); // 至少1秒
    
    auto result = BaseLocationCorrector::correctLocation(location);
    
    // 恢复原始配置
    config_->minCorrectionInterval = originalInterval;
    
    return result;
}

std::shared_ptr<CorrectedLocation> MultiModeLocationCorrector::applyFastUpdateModeCorrection(const LocationInfo& location) {
    // 快速更新模式：优先考虑更新频率
    Logger::getInstance().debug("Applying fast update mode correction");
    
    // 快速更新模式下，大幅降低时间间隔要求
    auto originalInterval = config_->minCorrectionInterval;
    config_->minCorrectionInterval = std::max(50LL, originalInterval / 4); // 至少50ms
    
    auto result = BaseLocationCorrector::correctLocation(location);
    
    // 恢复原始配置
    config_->minCorrectionInterval = originalInterval;
    
    return result;
}

std::shared_ptr<CorrectedLocation> MultiModeLocationCorrector::applyOfflineModeCorrection(const LocationInfo& location) {
    // 离线模式：只使用本地数据进行纠偏
    Logger::getInstance().debug("Applying offline mode correction");
    
    // 简化处理，实际项目中可以添加离线地图数据支持等
    return BaseLocationCorrector::correctLocation(location);
}

} // namespace location_correction