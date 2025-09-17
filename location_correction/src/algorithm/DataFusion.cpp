// DataFusion.cpp - 多源数据融合算法实现

#include "DataFusion.h"
#include "Logger.h"
#include "Utils.h"
#include <algorithm>
#include <numeric>
#include <cmath>
#include <functional>
#include <sstream>

// DataFusion构造函数
DataFusion::DataFusion() : 
    enabled(true),
    minRequiredSources(2),
    fusionStrategy(FusionStrategy::WEIGHTED_AVERAGE)
{
}

// DataFusion析构函数
DataFusion::~DataFusion() {
}

// 设置融合器是否启用
void DataFusion::setEnabled(bool enable) {
    enabled = enable;
}

// 检查融合器是否启用
bool DataFusion::isEnabled() const {
    return enabled;
}

// 设置最小需要的数据源数量
void DataFusion::setMinRequiredSources(size_t count) {
    minRequiredSources = std::max(static_cast<size_t>(1), count);
}

// 获取最小需要的数据源数量
size_t DataFusion::getMinRequiredSources() const {
    return minRequiredSources;
}

// 设置融合策略
void DataFusion::setFusionStrategy(FusionStrategy strategy) {
    fusionStrategy = strategy;
}

// 获取融合策略
FusionStrategy DataFusion::getFusionStrategy() const {
    return fusionStrategy;
}

// 融合多个位置数据
std::shared_ptr<LocationInfo> DataFusion::fuse(const std::vector<LocationInfo>& locations) {
    // 检查融合器是否启用
    if (!isEnabled()) {
        LOG_WARNING("Data fusion is disabled");
        return nullptr;
    }
    
    // 检查位置数据数量是否满足最小要求
    if (locations.size() < minRequiredSources) {
        LOG_WARNING("Not enough location sources for fusion: %zu < %zu", 
                   locations.size(), minRequiredSources);
        return nullptr;
    }
    
    try {
        // 过滤无效的位置数据
        std::vector<LocationInfo> validLocations;
        for (const auto& location : locations) {
            if (location.isValid() && location.status != LocationStatus::ANOMALY) {
                validLocations.push_back(location);
            } else {
                LOG_DEBUG("Skipping invalid location for fusion: %s", location.toString().c_str());
            }
        }
        
        // 再次检查有效数据数量
        if (validLocations.size() < minRequiredSources) {
            LOG_WARNING("Not enough valid location sources for fusion after filtering: %zu < %zu", 
                       validLocations.size(), minRequiredSources);
            return nullptr;
        }
        
        // 执行具体的融合逻辑
        auto fusedLocation = doFuse(validLocations);
        
        if (fusedLocation) {
            // 设置融合后的位置状态和额外信息
            fusedLocation->status = LocationStatus::NORMAL;
            fusedLocation->sourceType = DataSourceType::FUSED;
            fusedLocation->setExtra("fusionStrategy", fusionStrategyToString(fusionStrategy));
            fusedLocation->setExtra("sourceCount", sizeToString(validLocations.size()));
            
            LOG_DEBUG("Successfully fused %zu location sources", validLocations.size());
        }
        
        return fusedLocation;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in data fusion: %s", e.what());
        return nullptr;
    }
}

// 具体的融合逻辑（虚函数，子类需要实现）
std::shared_ptr<LocationInfo> DataFusion::doFuse(const std::vector<LocationInfo>& locations) {
    return nullptr;
}

// PriorityBasedFusion构造函数
PriorityBasedFusion::PriorityBasedFusion() : DataFusion() {
    // 设置默认优先级
    sourcePriorities[DataSourceType::GNSS] = 100;
    sourcePriorities[DataSourceType::WIFI] = 80;
    sourcePriorities[DataSourceType::BASE_STATION] = 60;
    sourcePriorities[DataSourceType::BLE] = 40;
    sourcePriorities[DataSourceType::INERTIAL] = 20;
}

// 设置数据源优先级
void PriorityBasedFusion::setSourcePriority(DataSourceType sourceType, int priority) {
    sourcePriorities[sourceType] = priority;
    LOG_INFO("Set priority for source type %d to %d", 
             static_cast<int>(sourceType), priority);
}

// 获取数据源优先级
int PriorityBasedFusion::getSourcePriority(DataSourceType sourceType) const {
    auto it = sourcePriorities.find(sourceType);
    if (it != sourcePriorities.end()) {
        return it->second;
    }
    return 0; // 默认优先级为0
}

// 基于优先级的融合实现
std::shared_ptr<LocationInfo> PriorityBasedFusion::doFuse(const std::vector<LocationInfo>& locations) {
    // 按优先级排序位置数据
    std::vector<LocationInfo> sortedLocations = locations;
    
    std::sort(sortedLocations.begin(), sortedLocations.end(),
        [this](const LocationInfo& a, const LocationInfo& b) {
            // 首先按数据源类型优先级排序
            int priorityA = getSourcePriority(a.sourceType);
            int priorityB = getSourcePriority(b.sourceType);
            
            if (priorityA != priorityB) {
                return priorityA > priorityB;
            }
            
            // 优先级相同时，按精度排序（精度值越小越好）
            return a.accuracy < b.accuracy;
        });
    
    // 选择优先级最高的位置作为融合结果
    auto fusedLocation = std::make_shared<LocationInfo>(sortedLocations[0]);
    
    // 添加融合信息
    fusedLocation->setExtra("fusionType", "PRIORITY_BASED");
    fusedLocation->setExtra("selectedSourceType", dataSourceTypeToString(sortedLocations[0].sourceType));
    fusedLocation->setExtra("selectedSourcePriority", intToString(getSourcePriority(sortedLocations[0].sourceType)));
    
    LOG_DEBUG("Priority-based fusion selected location from source type %d with priority %d", 
             static_cast<int>(sortedLocations[0].sourceType), 
             getSourcePriority(sortedLocations[0].sourceType));
    
    return fusedLocation;
}

// 获取融合器名称
std::string PriorityBasedFusion::getName() const {
    return "PriorityBasedFusion";
}

// WeightedAverageFusion构造函数
WeightedAverageFusion::WeightedAverageFusion() : DataFusion() {
    // 设置默认权重策略
    weightStrategy = WeightStrategy::ACCURACY_BASED;
}

// 设置权重策略
void WeightedAverageFusion::setWeightStrategy(WeightStrategy strategy) {
    weightStrategy = strategy;
    LOG_INFO("Set weight strategy to %d", static_cast<int>(strategy));
}

// 获取权重策略
WeightStrategy WeightedAverageFusion::getWeightStrategy() const {
    return weightStrategy;
}

// 设置自定义权重
void WeightedAverageFusion::setCustomWeight(DataSourceType sourceType, double weight) {
    customWeights[sourceType] = std::max(0.0, weight);
    LOG_INFO("Set custom weight for source type %d to %.2f", 
             static_cast<int>(sourceType), weight);
}

// 获取自定义权重
double WeightedAverageFusion::getCustomWeight(DataSourceType sourceType) const {
    auto it = customWeights.find(sourceType);
    if (it != customWeights.end()) {
        return it->second;
    }
    return 1.0; // 默认权重为1.0
}

// 计算数据源权重
std::vector<double> WeightedAverageFusion::calculateWeights(const std::vector<LocationInfo>& locations) {
    std::vector<double> weights;
    weights.reserve(locations.size());
    
    switch (weightStrategy) {
        case WeightStrategy::EQUAL_WEIGHT:
            // 等权重策略
            for (size_t i = 0; i < locations.size(); i++) {
                weights.push_back(1.0 / locations.size());
            }
            break;
            
        case WeightStrategy::ACCURACY_BASED:
            // 基于精度的权重策略（精度越高，权重越大）
            double totalAccuracyInverse = 0.0;
            for (const auto& location : locations) {
                if (location.accuracy > 0) {
                    totalAccuracyInverse += 1.0 / location.accuracy;
                } else {
                    totalAccuracyInverse += 1.0; // 避免除以0
                }
            }
            
            for (const auto& location : locations) {
                if (totalAccuracyInverse > 0) {
                    if (location.accuracy > 0) {
                        weights.push_back((1.0 / location.accuracy) / totalAccuracyInverse);
                    } else {
                        weights.push_back(1.0 / totalAccuracyInverse);
                    }
                } else {
                    weights.push_back(1.0 / locations.size());
                }
            }
            break;
            
        case WeightStrategy::CUSTOM:
            // 自定义权重策略
            double totalCustomWeight = 0.0;
            for (const auto& location : locations) {
                totalCustomWeight += getCustomWeight(location.sourceType);
            }
            
            for (const auto& location : locations) {
                if (totalCustomWeight > 0) {
                    weights.push_back(getCustomWeight(location.sourceType) / totalCustomWeight);
                } else {
                    weights.push_back(1.0 / locations.size());
                }
            }
            break;
            
        default:
            // 默认使用等权重策略
            for (size_t i = 0; i < locations.size(); i++) {
                weights.push_back(1.0 / locations.size());
            }
            break;
    }
    
    return weights;
}

// 基于加权平均的融合实现
std::shared_ptr<LocationInfo> WeightedAverageFusion::doFuse(const std::vector<LocationInfo>& locations) {
    // 计算权重
    std::vector<double> weights = calculateWeights(locations);
    
    // 计算加权平均经纬度
    double weightedLat = 0.0;
    double weightedLon = 0.0;
    double weightedAlt = 0.0;
    double weightedAccuracy = 0.0;
    
    for (size_t i = 0; i < locations.size(); i++) {
        weightedLat += locations[i].latitude * weights[i];
        weightedLon += locations[i].longitude * weights[i];
        weightedAlt += locations[i].altitude * weights[i];
        // 精度的加权平均使用调和平均更合适
        if (locations[i].accuracy > 0) {
            weightedAccuracy += weights[i] / locations[i].accuracy;
        }
    }
    
    // 创建融合后的位置信息
    auto fusedLocation = std::make_shared<LocationInfo>();
    fusedLocation->timestamp = getCurrentTimestampMs();
    fusedLocation->latitude = weightedLat;
    fusedLocation->longitude = weightedLon;
    fusedLocation->altitude = weightedAlt;
    
    // 计算调和平均精度
    if (weightedAccuracy > 0) {
        fusedLocation->accuracy = 1.0 / weightedAccuracy;
    } else {
        // 如果无法计算调和平均，使用算术平均
        double avgAccuracy = 0.0;
        for (const auto& location : locations) {
            avgAccuracy += location.accuracy;
        }
        fusedLocation->accuracy = avgAccuracy / locations.size();
    }
    
    // 添加融合信息
    fusedLocation->setExtra("fusionType", "WEIGHTED_AVERAGE");
    fusedLocation->setExtra("weightStrategy", weightStrategyToString(weightStrategy));
    
    // 记录权重信息
    std::stringstream weightInfo;
    weightInfo << "[";
    for (size_t i = 0; i < locations.size(); i++) {
        if (i > 0) {
            weightInfo << ", ";
        }
        weightInfo << dataSourceTypeToString(locations[i].sourceType) 
                  << ":" << doubleToString(weights[i], 3);
    }
    weightInfo << "]";
    fusedLocation->setExtra("weights", weightInfo.str());
    
    LOG_DEBUG("Weighted average fusion completed with %zu sources", locations.size());
    
    return fusedLocation;
}

// 获取融合器名称
std::string WeightedAverageFusion::getName() const {
    return "WeightedAverageFusion";
}

// AdaptiveFusion构造函数
AdaptiveFusion::AdaptiveFusion() : 
    DataFusion(),
    sceneClassifier(nullptr),
    sceneConfigs()
{
    // 设置默认场景配置
    SceneConfig defaultConfig;
    defaultConfig.sceneType = SceneType::UNKNOWN;
    defaultConfig.fusionStrategy = FusionStrategy::WEIGHTED_AVERAGE;
    defaultConfig.minRequiredSources = 2;
    
    // 添加默认配置
    sceneConfigs[SceneType::UNKNOWN] = defaultConfig;
}

// 设置场景分类器
void AdaptiveFusion::setSceneClassifier(std::shared_ptr<SceneClassifier> classifier) {
    sceneClassifier = classifier;
    LOG_INFO("Scene classifier set");
}

// 添加场景配置
bool AdaptiveFusion::addSceneConfig(const SceneConfig& config) {
    sceneConfigs[config.sceneType] = config;
    LOG_INFO("Added scene config for type %d", static_cast<int>(config.sceneType));
    return true;
}

// 获取场景配置
std::optional<SceneConfig> AdaptiveFusion::getSceneConfig(SceneType sceneType) const {
    auto it = sceneConfigs.find(sceneType);
    if (it != sceneConfigs.end()) {
        return it->second;
    }
    return std::nullopt;
}

// 基于自适应的融合实现
std::shared_ptr<LocationInfo> AdaptiveFusion::doFuse(const std::vector<LocationInfo>& locations) {
    // 确定当前场景
    SceneType currentScene = SceneType::UNKNOWN;
    
    if (sceneClassifier) {
        // 使用场景分类器确定场景
        try {
            currentScene = sceneClassifier->classifyScene(locations);
        } catch (const std::exception& e) {
            LOG_ERROR("Error in scene classification: %s", e.what());
        }
    }
    
    // 获取当前场景的配置
    SceneConfig config = sceneConfigs[SceneType::UNKNOWN]; // 默认配置
    
    auto configIt = sceneConfigs.find(currentScene);
    if (configIt != sceneConfigs.end()) {
        config = configIt->second;
    }
    
    LOG_DEBUG("Adaptive fusion using config for scene type %d", static_cast<int>(currentScene));
    
    // 根据场景配置选择融合策略
    std::shared_ptr<LocationInfo> fusedLocation;
    
    switch (config.fusionStrategy) {
        case FusionStrategy::PRIORITY_BASED:
        {
            PriorityBasedFusion priorityFusion;
            // 应用场景特定的优先级设置
            for (const auto& [sourceType, priority] : config.sourcePriorities) {
                priorityFusion.setSourcePriority(sourceType, priority);
            }
            fusedLocation = priorityFusion.doFuse(locations);
            break;
        }
        case FusionStrategy::WEIGHTED_AVERAGE:
        {
            WeightedAverageFusion weightedFusion;
            weightedFusion.setWeightStrategy(WeightStrategy::CUSTOM);
            // 应用场景特定的权重设置
            for (const auto& [sourceType, weight] : config.sourceWeights) {
                weightedFusion.setCustomWeight(sourceType, weight);
            }
            fusedLocation = weightedFusion.doFuse(locations);
            break;
        }
        case FusionStrategy::ADAPTIVE:
        default:
            // 默认使用加权平均融合
            WeightedAverageFusion defaultFusion;
            fusedLocation = defaultFusion.doFuse(locations);
            break;
    }
    
    // 添加场景信息
    if (fusedLocation) {
        fusedLocation->setExtra("sceneType", sceneTypeToString(currentScene));
        fusedLocation->setExtra("sceneSpecific", "true");
    }
    
    return fusedLocation;
}

// 获取融合器名称
std::string AdaptiveFusion::getName() const {
    return "AdaptiveFusion";
}

// FootprintCoherenceFusion构造函数
FootprintCoherenceFusion::FootprintCoherenceFusion() : 
    DataFusion(),
    coherenceThreshold(0.7), // 默认一致性阈值为0.7
    maxFootprintRadius(50.0) // 默认最大足迹半径为50米
{
}

// 设置一致性阈值
void FootprintCoherenceFusion::setCoherenceThreshold(double threshold) {
    coherenceThreshold = std::max(0.0, std::min(1.0, threshold));
    LOG_INFO("Set coherence threshold to %.2f", threshold);
}

// 获取一致性阈值
double FootprintCoherenceFusion::getCoherenceThreshold() const {
    return coherenceThreshold;
}

// 设置最大足迹半径
void FootprintCoherenceFusion::setMaxFootprintRadius(double radius) {
    maxFootprintRadius = std::max(0.0, radius);
    LOG_INFO("Set max footprint radius to %.2f meters", radius);
}

// 获取最大足迹半径
double FootprintCoherenceFusion::getMaxFootprintRadius() const {
    return maxFootprintRadius;
}

// 计算位置足迹（置信区域）
LocationFootprint FootprintCoherenceFusion::calculateFootprint(const LocationInfo& location) {
    LocationFootprint footprint;
    footprint.center = location;
    
    // 根据精度计算足迹半径
    footprint.radius = std::min(location.accuracy * 2.0, maxFootprintRadius);
    
    return footprint;
}

// 计算两个足迹的重叠度
double FootprintCoherenceFusion::calculateFootprintOverlap(const LocationFootprint& fp1, const LocationFootprint& fp2) {
    // 计算两个中心点之间的距离
    double distance = calculateDistance(
        fp1.center.latitude, fp1.center.longitude,
        fp2.center.latitude, fp2.center.longitude);
    
    // 如果两个足迹完全不重叠
    if (distance >= fp1.radius + fp2.radius) {
        return 0.0;
    }
    
    // 如果一个足迹完全包含另一个
    if (distance + std::min(fp1.radius, fp2.radius) <= std::max(fp1.radius, fp2.radius)) {
        double smallerArea = M_PI * std::pow(std::min(fp1.radius, fp2.radius), 2);
        double largerArea = M_PI * std::pow(std::max(fp1.radius, fp2.radius), 2);
        return smallerArea / largerArea;
    }
    
    // 计算部分重叠的面积比
    double d = distance;
    double r1 = fp1.radius;
    double r2 = fp2.radius;
    
    double a = (r1 * r1 - r2 * r2 + d * d) / (2 * d);
    double h = std::sqrt(r1 * r1 - a * a);
    
    double area1 = r1 * r1 * std::acos(a / r1) - a * h;
    double area2 = r2 * r2 * std::acos((d - a) / r2) - (d - a) * h;
    double totalOverlapArea = area1 + area2;
    
    // 计算两个足迹的总面积
    double totalArea = M_PI * (r1 * r1 + r2 * r2) - totalOverlapArea;
    
    // 返回重叠面积与总面积的比例
    return totalOverlapArea / totalArea;
}

// 基于足迹一致性的融合实现
std::shared_ptr<LocationInfo> FootprintCoherenceFusion::doFuse(const std::vector<LocationInfo>& locations) {
    // 计算每个位置的足迹
    std::vector<LocationFootprint> footprints;
    for (const auto& location : locations) {
        footprints.push_back(calculateFootprint(location));
    }
    
    // 计算足迹一致性矩阵
    std::vector<std::vector<double>> coherenceMatrix(locations.size(), std::vector<double>(locations.size(), 0.0));
    
    for (size_t i = 0; i < locations.size(); i++) {
        for (size_t j = i; j < locations.size(); j++) {
            if (i == j) {
                coherenceMatrix[i][j] = 1.0;
            } else {
                double coherence = calculateFootprintOverlap(footprints[i], footprints[j]);
                coherenceMatrix[i][j] = coherence;
                coherenceMatrix[j][i] = coherence;
            }
        }
    }
    
    // 找出一致性最高的子集
    std::vector<size_t> selectedIndices;
    double maxCoherenceScore = 0.0;
    
    // 使用贪心算法选择一致性最高的子集
    for (size_t i = 0; i < locations.size(); i++) {
        std::vector<size_t> currentSet = {i};
        double currentScore = 0.0;
        
        // 计算当前子集的一致性分数
        for (size_t j = 0; j < locations.size(); j++) {
            if (i != j && coherenceMatrix[i][j] >= coherenceThreshold) {
                currentSet.push_back(j);
            }
        }
        
        // 计算一致性分数（平均一致性）
        if (currentSet.size() > 1) {
            double totalCoherence = 0.0;
            int count = 0;
            
            for (size_t a = 0; a < currentSet.size(); a++) {
                for (size_t b = a + 1; b < currentSet.size(); b++) {
                    totalCoherence += coherenceMatrix[currentSet[a]][currentSet[b]];
                    count++;
                }
            }
            
            if (count > 0) {
                currentScore = totalCoherence / count;
            }
        }
        
        // 更新最佳子集
        if (currentSet.size() >= minRequiredSources && currentScore > maxCoherenceScore) {
            selectedIndices = currentSet;
            maxCoherenceScore = currentScore;
        }
    }
    
    // 如果没有找到足够的一致性子集，使用所有位置
    if (selectedIndices.size() < minRequiredSources) {
        for (size_t i = 0; i < locations.size(); i++) {
            selectedIndices.push_back(i);
        }
    }
    
    // 从选中的索引创建新的位置列表
    std::vector<LocationInfo> selectedLocations;
    for (size_t index : selectedIndices) {
        selectedLocations.push_back(locations[index]);
    }
    
    // 使用加权平均融合选中的位置
    WeightedAverageFusion fusion;
    fusion.setWeightStrategy(WeightStrategy::ACCURACY_BASED);
    auto fusedLocation = fusion.doFuse(selectedLocations);
    
    // 添加足迹一致性信息
    if (fusedLocation) {
        fusedLocation->setExtra("fusionType", "FOOTPRINT_COHERENCE");
        fusedLocation->setExtra("coherenceScore", doubleToString(maxCoherenceScore, 3));
        fusedLocation->setExtra("selectedSourceCount", sizeToString(selectedLocations.size()));
        fusedLocation->setExtra("totalSourceCount", sizeToString(locations.size()));
    }
    
    LOG_DEBUG("Footprint coherence fusion completed, selected %zu of %zu sources with coherence score %.3f", 
             selectedLocations.size(), locations.size(), maxCoherenceScore);
    
    return fusedLocation;
}

// 获取融合器名称
std::string FootprintCoherenceFusion::getName() const {
    return "FootprintCoherenceFusion";
}