// ConfigModel.h - 配置数据模型定义

#ifndef CONFIG_MODEL_H
#define CONFIG_MODEL_H

#include <string>
#include <vector>
#include <map>

// 融合策略枚举
enum class FusionStrategy {
    PRIORITY_BASED,  // 基于优先级的融合
    WEIGHTED_AVERAGE, // 加权平均融合
    ADAPTIVE         // 自适应融合
};

// 场景配置
struct SceneConfig {
    std::string sceneId;            // 场景ID
    std::string sceneName;          // 场景名称
    double latitude;                // 场景中心点纬度
    double longitude;               // 场景中心点经度
    double radius;                  // 场景半径（米）
    bool enableCorrection;          // 是否启用纠偏
    std::map<std::string, double> correctionParams; // 纠偏参数

    // 构造函数
    SceneConfig() : 
        latitude(0.0), 
        longitude(0.0), 
        radius(0.0), 
        enableCorrection(false) {}
};

// 异常阈值配置
struct AnomalyThresholds {
    double maxTimeDiff;             // 最大时间差（毫秒）
    double maxDistanceDiff;         // 最大距离差（米）
    double maxSpeed;                // 最大速度（米/秒）
    double minAccuracy;             // 最小精度要求（米）
    double accelerationThreshold;   // 加速度阈值（m/s²）

    // 构造函数
    AnomalyThresholds() : 
        maxTimeDiff(60000),      // 默认60秒
        maxDistanceDiff(500),    // 默认500米
        maxSpeed(70),            // 默认70m/s（约252km/h）
        minAccuracy(100),        // 默认100米
        accelerationThreshold(10) // 默认10m/s²
    {}
};

// 纠偏配置
struct CorrectionConfig {
    bool enableGnssCorrection;      // 启用GNSS纠偏
    bool enableWifiCorrection;      // 启用Wi-Fi纠偏
    bool enableBaseStationCorrection; // 启用基站纠偏
    FusionStrategy fusionStrategy;  // 融合策略
    std::vector<SceneConfig> sceneConfigs; // 场景配置
    AnomalyThresholds anomalyThresholds; // 异常阈值
    std::map<std::string, double> algorithmParams; // 算法参数

    // 构造函数
    CorrectionConfig() : 
        enableGnssCorrection(true),
        enableWifiCorrection(true),
        enableBaseStationCorrection(true),
        fusionStrategy(FusionStrategy::ADAPTIVE)
    {}
};

// 默认配置定义
static const CorrectionConfig DEFAULT_CONFIG = {
    true,  // enableGnssCorrection
    true,  // enableWifiCorrection
    true,  // enableBaseStationCorrection
    FusionStrategy::ADAPTIVE,  // fusionStrategy
    {},    // sceneConfigs (默认空)
    AnomalyThresholds(),  // 默认异常阈值
    {{"smoothingFactor", 0.7}, {"confidenceThreshold", 0.6}}  // 默认算法参数
};

#endif // CONFIG_MODEL_H