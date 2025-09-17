// ConfigModel.cpp - 配置数据模型实现

#include "ConfigModel.h"
#include <fstream>
#include <sstream>
#include <iomanip>
#include <algorithm>

// 构造函数实现
SceneConfig::SceneConfig() : 
    enableAnomalyDetection(true),
    enableDataFusion(true),
    enableSmoothing(false),
    enableTrajectoryAnalysis(false),
    fusionStrategy(FusionStrategy::WEIGHTED_AVERAGE),
    maxHistorySize(50),
    minAccuracyThreshold(100.0),
    maxSpeedThreshold(120.0),
    locationUpdateInterval(1000),
    stationaryThreshold(0.5),
    walkingThreshold(10.0),
    runningThreshold(20.0),
    drivingThreshold(60.0) {
}

// 复制构造函数
SceneConfig::SceneConfig(const SceneConfig& other) : 
    enableAnomalyDetection(other.enableAnomalyDetection),
    enableDataFusion(other.enableDataFusion),
    enableSmoothing(other.enableSmoothing),
    enableTrajectoryAnalysis(other.enableTrajectoryAnalysis),
    fusionStrategy(other.fusionStrategy),
    maxHistorySize(other.maxHistorySize),
    minAccuracyThreshold(other.minAccuracyThreshold),
    maxSpeedThreshold(other.maxSpeedThreshold),
    locationUpdateInterval(other.locationUpdateInterval),
    stationaryThreshold(other.stationaryThreshold),
    walkingThreshold(other.walkingThreshold),
    runningThreshold(other.runningThreshold),
    drivingThreshold(other.drivingThreshold) {
}

// 赋值运算符
SceneConfig& SceneConfig::operator=(const SceneConfig& other) {
    if (this != &other) {
        enableAnomalyDetection = other.enableAnomalyDetection;
        enableDataFusion = other.enableDataFusion;
        enableSmoothing = other.enableSmoothing;
        enableTrajectoryAnalysis = other.enableTrajectoryAnalysis;
        fusionStrategy = other.fusionStrategy;
        maxHistorySize = other.maxHistorySize;
        minAccuracyThreshold = other.minAccuracyThreshold;
        maxSpeedThreshold = other.maxSpeedThreshold;
        locationUpdateInterval = other.locationUpdateInterval;
        stationaryThreshold = other.stationaryThreshold;
        walkingThreshold = other.walkingThreshold;
        runningThreshold = other.runningThreshold;
        drivingThreshold = other.drivingThreshold;
    }
    return *this;
}

// 转换为字符串表示
std::string SceneConfig::toString() const {
    std::stringstream ss;
    ss << "SceneConfig{" 
       << "enableAnomalyDetection=" << (enableAnomalyDetection ? "true" : "false") << ", "
       << "enableDataFusion=" << (enableDataFusion ? "true" : "false") << ", "
       << "enableSmoothing=" << (enableSmoothing ? "true" : "false") << ", "
       << "enableTrajectoryAnalysis=" << (enableTrajectoryAnalysis ? "true" : "false") << ", "
       << "fusionStrategy=" << static_cast<int>(fusionStrategy) << ", "
       << "maxHistorySize=" << maxHistorySize << ", "
       << "minAccuracyThreshold=" << minAccuracyThreshold << ", "
       << "maxSpeedThreshold=" << maxSpeedThreshold
       << "}";
    return ss.str();
}

// AnomalyThresholds构造函数
AnomalyThresholds::AnomalyThresholds() : 
    maxTimeDifference(60000),  // 60秒
    maxDistanceDifference(500.0), // 500米
    maxAcceleration(10.0),    // 10m/s²
    minConfidenceScore(0.5),
    maxJerk(5.0),             // 5m/s³
    minSatelliteCount(4),
    minSignalStrength(20),
    maxPositionVariance(100.0) {
}

// 复制构造函数
AnomalyThresholds::AnomalyThresholds(const AnomalyThresholds& other) : 
    maxTimeDifference(other.maxTimeDifference),
    maxDistanceDifference(other.maxDistanceDifference),
    maxAcceleration(other.maxAcceleration),
    minConfidenceScore(other.minConfidenceScore),
    maxJerk(other.maxJerk),
    minSatelliteCount(other.minSatelliteCount),
    minSignalStrength(other.minSignalStrength),
    maxPositionVariance(other.maxPositionVariance) {
}

// 赋值运算符
AnomalyThresholds& AnomalyThresholds::operator=(const AnomalyThresholds& other) {
    if (this != &other) {
        maxTimeDifference = other.maxTimeDifference;
        maxDistanceDifference = other.maxDistanceDifference;
        maxAcceleration = other.maxAcceleration;
        minConfidenceScore = other.minConfidenceScore;
        maxJerk = other.maxJerk;
        minSatelliteCount = other.minSatelliteCount;
        minSignalStrength = other.minSignalStrength;
        maxPositionVariance = other.maxPositionVariance;
    }
    return *this;
}

// 转换为字符串表示
std::string AnomalyThresholds::toString() const {
    std::stringstream ss;
    ss << "AnomalyThresholds{" 
       << "maxTimeDifference=" << maxTimeDifference << ", "
       << "maxDistanceDifference=" << maxDistanceDifference << ", "
       << "maxAcceleration=" << maxAcceleration << ", "
       << "minConfidenceScore=" << minConfidenceScore
       << "}";
    return ss.str();
}

// CorrectionConfig构造函数
CorrectionConfig::CorrectionConfig() : 
    sceneConfig(),
    anomalyThresholds(),
    dataSourceWeights(),
    enableDebugMode(false),
    enableLogging(true),
    logLevel(3),
    cacheSize(100),
    cacheTimeout(300000), // 5分钟
    storagePath("./location_data"),
    enableEncryption(false),
    encryptionKey(""),
    enableAutoSave(true),
    saveInterval(60000), // 1分钟
    customParameters() {
    // 初始化默认数据源权重
    dataSourceWeights[DataSourceType::GPS] = 0.8;
    dataSourceWeights[DataSourceType::GLONASS] = 0.7;
    dataSourceWeights[DataSourceType::BEIDOU] = 0.75;
    dataSourceWeights[DataSourceType::GALILEO] = 0.7;
    dataSourceWeights[DataSourceType::WIFI] = 0.6;
    dataSourceWeights[DataSourceType::BASE_STATION] = 0.5;
    dataSourceWeights[DataSourceType::BLE] = 0.4;
    dataSourceWeights[DataSourceType::IMU] = 0.3;
}

// 复制构造函数
CorrectionConfig::CorrectionConfig(const CorrectionConfig& other) : 
    sceneConfig(other.sceneConfig),
    anomalyThresholds(other.anomalyThresholds),
    dataSourceWeights(other.dataSourceWeights),
    enableDebugMode(other.enableDebugMode),
    enableLogging(other.enableLogging),
    logLevel(other.logLevel),
    cacheSize(other.cacheSize),
    cacheTimeout(other.cacheTimeout),
    storagePath(other.storagePath),
    enableEncryption(other.enableEncryption),
    encryptionKey(other.encryptionKey),
    enableAutoSave(other.enableAutoSave),
    saveInterval(other.saveInterval),
    customParameters(other.customParameters) {
}

// 赋值运算符
CorrectionConfig& CorrectionConfig::operator=(const CorrectionConfig& other) {
    if (this != &other) {
        sceneConfig = other.sceneConfig;
        anomalyThresholds = other.anomalyThresholds;
        dataSourceWeights = other.dataSourceWeights;
        enableDebugMode = other.enableDebugMode;
        enableLogging = other.enableLogging;
        logLevel = other.logLevel;
        cacheSize = other.cacheSize;
        cacheTimeout = other.cacheTimeout;
        storagePath = other.storagePath;
        enableEncryption = other.enableEncryption;
        encryptionKey = other.encryptionKey;
        enableAutoSave = other.enableAutoSave;
        saveInterval = other.saveInterval;
        customParameters = other.customParameters;
    }
    return *this;
}

// 转换为字符串表示
std::string CorrectionConfig::toString() const {
    std::stringstream ss;
    ss << "CorrectionConfig{" 
       << "sceneConfig=[" << sceneConfig.toString() << "], "
       << "anomalyThresholds=[" << anomalyThresholds.toString() << "], "
       << "enableDebugMode=" << (enableDebugMode ? "true" : "false") << ", "
       << "enableLogging=" << (enableLogging ? "true" : "false") << ", "
       << "cacheSize=" << cacheSize
       << "}";
    return ss.str();
}

// 设置数据源权重
void CorrectionConfig::setDataSourceWeight(DataSourceType type, double weight) {
    // 确保权重在有效范围内
    weight = std::max(0.0, std::min(1.0, weight));
    dataSourceWeights[type] = weight;
}

// 获取数据源权重
double CorrectionConfig::getDataSourceWeight(DataSourceType type) const {
    auto it = dataSourceWeights.find(type);
    if (it != dataSourceWeights.end()) {
        return it->second;
    }
    return 0.0; // 默认权重为0
}

// 设置自定义参数
void CorrectionConfig::setCustomParameter(const std::string& key, const std::string& value) {
    customParameters[key] = value;
}

// 获取自定义参数
std::string CorrectionConfig::getCustomParameter(const std::string& key, const std::string& defaultValue) const {
    auto it = customParameters.find(key);
    if (it != customParameters.end()) {
        return it->second;
    }
    return defaultValue;
}

// 检查是否存在自定义参数
bool CorrectionConfig::hasCustomParameter(const std::string& key) const {
    return customParameters.find(key) != customParameters.end();
}

// 从文件加载配置
bool CorrectionConfig::loadFromFile(const std::string& filePath) {
    std::ifstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    std::string line;
    while (std::getline(file, line)) {
        // 跳过注释和空行
        if (line.empty() || line[0] == '#') {
            continue;
        }
        
        // 解析配置项
        size_t pos = line.find('=');
        if (pos != std::string::npos) {
            std::string key = line.substr(0, pos);
            std::string value = line.substr(pos + 1);
            
            // 去除首尾空格
            key.erase(0, key.find_first_not_of(" \t"));
            key.erase(key.find_last_not_of(" \t") + 1);
            value.erase(0, value.find_first_not_of(" \t"));
            value.erase(value.find_last_not_of(" \t") + 1);
            
            // 这里简化处理，实际应用中应该根据配置项名称进行更详细的解析
            setCustomParameter(key, value);
        }
    }
    
    file.close();
    return true;
}

// 保存配置到文件
bool CorrectionConfig::saveToFile(const std::string& filePath) const {
    std::ofstream file(filePath);
    if (!file.is_open()) {
        return false;
    }
    
    // 写入配置说明
    file << "# Location Correction Configuration File\n";
    file << "# Last updated: " << getCurrentTimestamp() << "\n\n";
    
    // 写入场景配置
    file << "# Scene Configuration\n";
    file << "scene.enableAnomalyDetection=" << (sceneConfig.enableAnomalyDetection ? "true" : "false") << "\n";
    file << "scene.enableDataFusion=" << (sceneConfig.enableDataFusion ? "true" : "false") << "\n";
    file << "scene.enableSmoothing=" << (sceneConfig.enableSmoothing ? "true" : "false") << "\n";
    file << "scene.enableTrajectoryAnalysis=" << (sceneConfig.enableTrajectoryAnalysis ? "true" : "false") << "\n";
    file << "scene.fusionStrategy=" << static_cast<int>(sceneConfig.fusionStrategy) << "\n";
    file << "scene.maxHistorySize=" << sceneConfig.maxHistorySize << "\n";
    file << "scene.minAccuracyThreshold=" << sceneConfig.minAccuracyThreshold << "\n";
    file << "scene.maxSpeedThreshold=" << sceneConfig.maxSpeedThreshold << "\n\n";
    
    // 写入异常阈值配置
    file << "# Anomaly Detection Thresholds\n";
    file << "anomaly.maxTimeDifference=" << anomalyThresholds.maxTimeDifference << "\n";
    file << "anomaly.maxDistanceDifference=" << anomalyThresholds.maxDistanceDifference << "\n";
    file << "anomaly.maxAcceleration=" << anomalyThresholds.maxAcceleration << "\n";
    file << "anomaly.minConfidenceScore=" << anomalyThresholds.minConfidenceScore << "\n\n";
    
    // 写入数据源权重
    file << "# Data Source Weights\n";
    for (const auto& pair : dataSourceWeights) {
        file << "source.weight." << static_cast<int>(pair.first) << "=" << pair.second << "\n";
    }
    file << "\n";
    
    // 写入自定义参数
    file << "# Custom Parameters\n";
    for (const auto& pair : customParameters) {
        file << pair.first << "=" << pair.second << "\n";
    }
    
    file.close();
    return true;
}

// 获取当前时间戳字符串
std::string CorrectionConfig::getCurrentTimestamp() {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_c), "%Y-%m-%d %H:%M:%S");
    return ss.str();
}

// 默认配置常量定义
const CorrectionConfig DEFAULT_CONFIG;