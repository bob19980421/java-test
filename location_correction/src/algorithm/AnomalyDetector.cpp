// AnomalyDetector.cpp - 异常点检测算法实现

#include "AnomalyDetector.h"
#include "Logger.h"
#include "Utils.h"
#include <algorithm>
#include <cmath>
#include <numeric>
#include <queue>

// AnomalyDetector构造函数
AnomalyDetector::AnomalyDetector() : 
    enabled(true),
    threshold(0.0),
    minSampleSize(5)
{
}

// AnomalyDetector析构函数
AnomalyDetector::~AnomalyDetector() {
}

// 设置检测器是否启用
void AnomalyDetector::setEnabled(bool enable) {
    enabled = enable;
}

// 检查检测器是否启用
bool AnomalyDetector::isEnabled() const {
    return enabled;
}

// 设置异常检测阈值
void AnomalyDetector::setThreshold(double thresh) {
    threshold = std::max(0.0, thresh);
}

// 获取异常检测阈值
double AnomalyDetector::getThreshold() const {
    return threshold;
}

// 设置最小样本数量
void AnomalyDetector::setMinSampleSize(size_t size) {
    minSampleSize = std::max(static_cast<size_t>(1), size);
}

// 获取最小样本数量
size_t AnomalyDetector::getMinSampleSize() const {
    return minSampleSize;
}

// 检测单个位置是否异常
AnomalyResult AnomalyDetector::detectAnomaly(const LocationInfo& location, const std::vector<LocationInfo>& context) {
    AnomalyResult result;
    result.isAnomaly = false;
    result.confidence = 0.0;
    
    // 如果检测器未启用或上下文数据不足，直接返回非异常
    if (!isEnabled() || context.size() < minSampleSize) {
        return result;
    }
    
    try {
        // 执行具体的异常检测算法
        result = doDetectAnomaly(location, context);
        
        // 记录检测结果
        if (result.isAnomaly) {
            LOG_DEBUG("Anomaly detected: %s, confidence: %.2f", 
                     location.toString().c_str(), result.confidence);
        } else {
            LOG_DEBUG("Location is normal: %s", location.toString().c_str());
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in anomaly detection: %s", e.what());
    }
    
    return result;
}

// 批量检测位置异常
std::vector<AnomalyResult> AnomalyDetector::batchDetectAnomaly(
    const std::vector<LocationInfo>& locations, 
    const std::vector<LocationInfo>& context) {
    
    std::vector<AnomalyResult> results;
    results.reserve(locations.size());
    
    for (const auto& location : locations) {
        results.push_back(detectAnomaly(location, context));
    }
    
    return results;
}

// 具体的异常检测算法（虚函数，子类需要实现）
AnomalyResult AnomalyDetector::doDetectAnomaly(const LocationInfo& location, const std::vector<LocationInfo>& context) {
    AnomalyResult result;
    result.isAnomaly = false;
    result.confidence = 0.0;
    return result;
}

// TimeDifferenceAnomalyDetector构造函数
TimeDifferenceAnomalyDetector::TimeDifferenceAnomalyDetector() : 
    AnomalyDetector(),
    maxTimeDiff(60000) // 默认最大时间差为60秒（毫秒）
{
    setThreshold(1.0); // 设置默认阈值
}

// 设置最大时间差
void TimeDifferenceAnomalyDetector::setMaxTimeDifference(long long diffMs) {
    maxTimeDiff = std::max(0LL, diffMs);
}

// 获取最大时间差
long long TimeDifferenceAnomalyDetector::getMaxTimeDifference() const {
    return maxTimeDiff;
}

// 执行时间差异常检测
AnomalyResult TimeDifferenceAnomalyDetector::doDetectAnomaly(const LocationInfo& location, const std::vector<LocationInfo>& context) {
    AnomalyResult result;
    result.isAnomaly = false;
    result.confidence = 0.0;
    
    // 计算当前时间与位置时间戳的差值
    long long currentTime = getCurrentTimestampMs();
    long long timeDiff = currentTime - location.timestamp;
    
    // 检查时间差是否超过阈值
    if (timeDiff > maxTimeDiff) {
        result.isAnomaly = true;
        
        // 计算置信度（时间差越大，置信度越高）
        double normalizedDiff = static_cast<double>(timeDiff) / maxTimeDiff;
        result.confidence = std::min(1.0, normalizedDiff);
        
        // 添加异常信息
        result.anomalyInfo["type"] = "TIME_DIFFERENCE";
        result.anomalyInfo["timeDiff"] = longLongToString(timeDiff);
        result.anomalyInfo["maxAllowed"] = longLongToString(maxTimeDiff);
    }
    
    return result;
}

// 获取检测器名称
std::string TimeDifferenceAnomalyDetector::getName() const {
    return "TimeDifferenceAnomalyDetector";
}

// DistanceDeviationAnomalyDetector构造函数
DistanceDeviationAnomalyDetector::DistanceDeviationAnomalyDetector() : 
    AnomalyDetector(),
    maxSpeed(30.0), // 默认最大速度为30m/s（约108km/h）
    windowSize(10) // 默认窗口大小为10个点
{
    setThreshold(1.0); // 设置默认阈值
}

// 设置最大允许速度
void DistanceDeviationAnomalyDetector::setMaxSpeed(double speedMps) {
    maxSpeed = std::max(0.0, speedMps);
}

// 获取最大允许速度
double DistanceDeviationAnomalyDetector::getMaxSpeed() const {
    return maxSpeed;
}

// 设置窗口大小
void DistanceDeviationAnomalyDetector::setWindowSize(size_t size) {
    windowSize = std::max(static_cast<size_t>(2), size);
    setMinSampleSize(windowSize);
}

// 获取窗口大小
size_t DistanceDeviationAnomalyDetector::getWindowSize() const {
    return windowSize;
}

// 执行距离偏移异常检测
AnomalyResult DistanceDeviationAnomalyDetector::doDetectAnomaly(const LocationInfo& location, const std::vector<LocationInfo>& context) {
    AnomalyResult result;
    result.isAnomaly = false;
    result.confidence = 0.0;
    
    // 检查上下文数据是否足够
    if (context.size() < minSampleSize) {
        return result;
    }
    
    try {
        // 找出与当前位置时间最接近的前一个位置
        LocationInfo previousLocation;
        bool foundPrevious = false;
        long long minTimeDiff = LLONG_MAX;
        
        for (const auto& ctxLoc : context) {
            if (ctxLoc.timestamp < location.timestamp) {
                long long timeDiff = location.timestamp - ctxLoc.timestamp;
                if (timeDiff < minTimeDiff) {
                    minTimeDiff = timeDiff;
                    previousLocation = ctxLoc;
                    foundPrevious = true;
                }
            }
        }
        
        // 如果找到了前一个位置
        if (foundPrevious && minTimeDiff > 0) {
            // 计算两个位置之间的距离
            double distance = calculateDistance(
                previousLocation.latitude, previousLocation.longitude,
                location.latitude, location.longitude);
            
            // 计算平均速度（m/s）
            double speed = distance / (minTimeDiff / 1000.0);
            
            // 检查速度是否超过阈值
            if (speed > maxSpeed) {
                result.isAnomaly = true;
                
                // 计算置信度（速度超过越多，置信度越高）
                double speedRatio = speed / maxSpeed;
                result.confidence = std::min(1.0, speedRatio - 1.0);
                
                // 添加异常信息
                result.anomalyInfo["type"] = "SPEED_EXCEEDANCE";
                result.anomalyInfo["calculatedSpeed"] = doubleToString(speed, 2);
                result.anomalyInfo["maxAllowedSpeed"] = doubleToString(maxSpeed, 2);
                result.anomalyInfo["distance"] = doubleToString(distance, 2);
                result.anomalyInfo["timeDiff"] = longLongToString(minTimeDiff);
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error in distance deviation detection: %s", e.what());
    }
    
    return result;
}

// 获取检测器名称
std::string DistanceDeviationAnomalyDetector::getName() const {
    return "DistanceDeviationAnomalyDetector";
}

// StatisticalAnomalyDetector构造函数
StatisticalAnomalyDetector::StatisticalAnomalyDetector() : 
    AnomalyDetector(),
    zScoreThreshold(2.0), // 默认Z-score阈值为2.0
    historySize(50) // 默认历史数据大小为50
{
    setThreshold(zScoreThreshold);
}

// 设置Z-score阈值
void StatisticalAnomalyDetector::setZScoreThreshold(double threshold) {
    zScoreThreshold = std::max(0.0, threshold);
    setThreshold(zScoreThreshold);
}

// 获取Z-score阈值
double StatisticalAnomalyDetector::getZScoreThreshold() const {
    return zScoreThreshold;
}

// 设置历史数据大小
void StatisticalAnomalyDetector::setHistorySize(size_t size) {
    std::lock_guard<std::mutex> lock(historyMutex);
    historySize = std::max(static_cast<size_t>(minSampleSize), size);
    trimHistory();
}

// 获取历史数据大小
size_t StatisticalAnomalyDetector::getHistorySize() const {
    std::lock_guard<std::mutex> lock(historyMutex);
    return historySize;
}

// 裁剪历史数据
void StatisticalAnomalyDetector::trimHistory() {
    while (history.size() > historySize) {
        history.pop_front();
    }
}

// 添加位置到历史数据
void StatisticalAnomalyDetector::addLocationToHistory(const LocationInfo& location) {
    std::lock_guard<std::mutex> lock(historyMutex);
    history.push_back(location);
    trimHistory();
}

// 清空历史数据
void StatisticalAnomalyDetector::clearHistory() {
    std::lock_guard<std::mutex> lock(historyMutex);
    history.clear();
}

// 执行统计异常检测
AnomalyResult StatisticalAnomalyDetector::doDetectAnomaly(const LocationInfo& location, const std::vector<LocationInfo>& context) {
    AnomalyResult result;
    result.isAnomaly = false;
    result.confidence = 0.0;
    
    std::lock_guard<std::mutex> lock(historyMutex);
    
    // 合并传入的上下文和历史数据
    std::vector<LocationInfo> combinedContext = context;
    combinedContext.insert(combinedContext.end(), history.begin(), history.end());
    
    // 检查数据是否足够
    if (combinedContext.size() < minSampleSize) {
        // 数据不足，将当前位置添加到历史，但不进行异常检测
        history.push_back(location);
        trimHistory();
        return result;
    }
    
    try {
        // 计算统计特征
        std::vector<double> latitudes, longitudes, accuracies;
        for (const auto& loc : combinedContext) {
            latitudes.push_back(loc.latitude);
            longitudes.push_back(loc.longitude);
            accuracies.push_back(loc.accuracy);
        }
        
        // 计算平均值和标准差
        double meanLat = calculateMean(latitudes);
        double meanLon = calculateMean(longitudes);
        double meanAccuracy = calculateMean(accuracies);
        
        double stdDevLat = calculateStandardDeviation(latitudes, meanLat);
        double stdDevLon = calculateStandardDeviation(longitudes, meanLon);
        double stdDevAccuracy = calculateStandardDeviation(accuracies, meanAccuracy);
        
        // 计算Z-scores
        double zScoreLat = 0.0, zScoreLon = 0.0, zScoreAccuracy = 0.0;
        
        if (stdDevLat > 0.0) {
            zScoreLat = std::abs(location.latitude - meanLat) / stdDevLat;
        }
        
        if (stdDevLon > 0.0) {
            zScoreLon = std::abs(location.longitude - meanLon) / stdDevLon;
        }
        
        if (stdDevAccuracy > 0.0) {
            zScoreAccuracy = std::abs(location.accuracy - meanAccuracy) / stdDevAccuracy;
        }
        
        // 判断是否为异常点
        bool isLatAnomaly = zScoreLat > zScoreThreshold;
        bool isLonAnomaly = zScoreLon > zScoreThreshold;
        bool isAccuracyAnomaly = zScoreAccuracy > zScoreThreshold * 2; // 精度异常使用更高的阈值
        
        result.isAnomaly = isLatAnomaly || isLonAnomaly || isAccuracyAnomaly;
        
        // 计算置信度（取最大的Z-score标准化后的值）
        double maxZScore = std::max({zScoreLat, zScoreLon, zScoreAccuracy / 2});
        result.confidence = std::min(1.0, (maxZScore - zScoreThreshold) / zScoreThreshold);
        
        // 添加异常信息
        if (result.isAnomaly) {
            result.anomalyInfo["type"] = "STATISTICAL";
            result.anomalyInfo["zScoreLat"] = doubleToString(zScoreLat, 2);
            result.anomalyInfo["zScoreLon"] = doubleToString(zScoreLon, 2);
            result.anomalyInfo["zScoreAccuracy"] = doubleToString(zScoreAccuracy, 2);
            result.anomalyInfo["threshold"] = doubleToString(zScoreThreshold, 2);
        }
        
        // 如果不是异常点，添加到历史数据
        if (!result.isAnomaly) {
            history.push_back(location);
            trimHistory();
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error in statistical anomaly detection: %s", e.what());
    }
    
    return result;
}

// 获取检测器名称
std::string StatisticalAnomalyDetector::getName() const {
    return "StatisticalAnomalyDetector";
}

// PatternMatchingAnomalyDetector构造函数
PatternMatchingAnomalyDetector::PatternMatchingAnomalyDetector() : 
    AnomalyDetector(),
    patterns(),
    patternThreshold(0.7) // 默认模式匹配阈值为0.7
{
    setThreshold(patternThreshold);
}

// 添加异常模式
void PatternMatchingAnomalyDetector::addAnomalyPattern(const LocationPattern& pattern) {
    std::lock_guard<std::mutex> lock(patternMutex);
    patterns.push_back(pattern);
    LOG_INFO("Added anomaly pattern with name: %s", pattern.name.c_str());
}

// 移除异常模式
bool PatternMatchingAnomalyDetector::removeAnomalyPattern(const std::string& patternName) {
    std::lock_guard<std::mutex> lock(patternMutex);
    
    auto it = std::find_if(patterns.begin(), patterns.end(),
        [&patternName](const LocationPattern& p) {
            return p.name == patternName;
        });
    
    if (it != patterns.end()) {
        patterns.erase(it);
        LOG_INFO("Removed anomaly pattern: %s", patternName.c_str());
        return true;
    }
    
    LOG_WARNING("Pattern not found: %s", patternName.c_str());
    return false;
}

// 清空所有异常模式
void PatternMatchingAnomalyDetector::clearAllPatterns() {
    std::lock_guard<std::mutex> lock(patternMutex);
    patterns.clear();
    LOG_INFO("All anomaly patterns cleared");
}

// 设置模式匹配阈值
void PatternMatchingAnomalyDetector::setPatternThreshold(double threshold) {
    patternThreshold = std::max(0.0, std::min(1.0, threshold));
    setThreshold(patternThreshold);
}

// 获取模式匹配阈值
double PatternMatchingAnomalyDetector::getPatternThreshold() const {
    return patternThreshold;
}

// 执行模式匹配异常检测
AnomalyResult PatternMatchingAnomalyDetector::doDetectAnomaly(const LocationInfo& location, const std::vector<LocationInfo>& context) {
    AnomalyResult result;
    result.isAnomaly = false;
    result.confidence = 0.0;
    
    std::lock_guard<std::mutex> lock(patternMutex);
    
    // 如果没有模式，直接返回非异常
    if (patterns.empty()) {
        return result;
    }
    
    try {
        // 检查每个模式
        for (const auto& pattern : patterns) {
            double similarity = matchPattern(location, pattern);
            
            // 如果相似度超过阈值，标记为异常
            if (similarity >= patternThreshold) {
                result.isAnomaly = true;
                
                // 置信度等于相似度
                result.confidence = similarity;
                
                // 添加异常信息
                result.anomalyInfo["type"] = "PATTERN_MATCH";
                result.anomalyInfo["patternName"] = pattern.name;
                result.anomalyInfo["similarity"] = doubleToString(similarity, 3);
                
                // 如果是严格模式，一旦匹配就返回
                if (pattern.isStrict) {
                    break;
                }
            }
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error in pattern matching detection: %s", e.what());
    }
    
    return result;
}

// 匹配单个模式
double PatternMatchingAnomalyDetector::matchPattern(const LocationInfo& location, const LocationPattern& pattern) {
    double similarity = 0.0;
    int matchedConditions = 0;
    int totalConditions = 0;
    
    // 匹配数据源类型
    if (pattern.sourceType.has_value()) {
        totalConditions++;
        if (location.sourceType == pattern.sourceType.value()) {
            matchedConditions++;
            similarity += 0.2; // 权重为0.2
        }
    }
    
    // 匹配精度范围
    if (pattern.minAccuracy.has_value() && pattern.maxAccuracy.has_value()) {
        totalConditions++;
        if (location.accuracy >= pattern.minAccuracy.value() && 
            location.accuracy <= pattern.maxAccuracy.value()) {
            matchedConditions++;
            similarity += 0.2; // 权重为0.2
        }
    }
    
    // 匹配状态
    if (pattern.status.has_value()) {
        totalConditions++;
        if (location.status == pattern.status.value()) {
            matchedConditions++;
            similarity += 0.1; // 权重为0.1
        }
    }
    
    // 匹配位置范围（如果提供了区域）
    if (pattern.region.has_value()) {
        totalConditions++;
        if (isPointInRegion(location, pattern.region.value())) {
            matchedConditions++;
            similarity += 0.3; // 权重为0.3
        }
    }
    
    // 匹配额外信息
    if (!pattern.extras.empty()) {
        totalConditions += pattern.extras.size();
        for (const auto& [key, value] : pattern.extras) {
            auto it = location.getExtras().find(key);
            if (it != location.getExtras().end() && it->second == value) {
                matchedConditions++;
                similarity += 0.05; // 每个额外条件权重为0.05
            }
        }
    }
    
    // 归一化相似度到0-1范围
    if (totalConditions > 0) {
        similarity = std::min(1.0, similarity);
    }
    
    return similarity;
}

// 检查点是否在区域内
bool PatternMatchingAnomalyDetector::isPointInRegion(const LocationInfo& point, const LocationRegion& region) {
    // 简单的矩形区域检查
    if (point.latitude >= region.minLat && point.latitude <= region.maxLat &&
        point.longitude >= region.minLon && point.longitude <= region.maxLon) {
        return true;
    }
    return false;
}

// 获取检测器名称
std::string PatternMatchingAnomalyDetector::getName() const {
    return "PatternMatchingAnomalyDetector";
}

// MultiDetectorAnomalyDetector构造函数
MultiDetectorAnomalyDetector::MultiDetectorAnomalyDetector() : 
    AnomalyDetector(),
    detectors(),
    fusionStrategy(FusionStrategy::MAJORITY_VOTE),
    minRequiredDetectors(2) // 默认至少需要2个检测器判定为异常
{
}

// 添加子检测器
bool MultiDetectorAnomalyDetector::addDetector(std::shared_ptr<AnomalyDetector> detector, double weight) {
    if (!detector) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(detectorsMutex);
    
    // 检查检测器是否已存在
    auto it = std::find_if(detectors.begin(), detectors.end(),
        [&detector](const std::pair<std::shared_ptr<AnomalyDetector>, double>& d) {
            return d.first == detector;
        });
    
    if (it != detectors.end()) {
        LOG_WARNING("Detector already added");
        return false;
    }
    
    // 添加检测器和权重
    detectors.push_back({detector, std::max(0.0, weight)});
    LOG_INFO("Added detector to multi-detector: %s", detector->getName().c_str());
    
    return true;
}

// 移除子检测器
bool MultiDetectorAnomalyDetector::removeDetector(const std::string& detectorName) {
    std::lock_guard<std::mutex> lock(detectorsMutex);
    
    auto it = std::find_if(detectors.begin(), detectors.end(),
        [&detectorName](const std::pair<std::shared_ptr<AnomalyDetector>, double>& d) {
            return d.first->getName() == detectorName;
        });
    
    if (it != detectors.end()) {
        LOG_INFO("Removed detector from multi-detector: %s", detectorName.c_str());
        detectors.erase(it);
        return true;
    }
    
    LOG_WARNING("Detector not found in multi-detector: %s", detectorName.c_str());
    return false;
}

// 设置融合策略
void MultiDetectorAnomalyDetector::setFusionStrategy(FusionStrategy strategy) {
    fusionStrategy = strategy;
}

// 获取融合策略
FusionStrategy MultiDetectorAnomalyDetector::getFusionStrategy() const {
    return fusionStrategy;
}

// 设置最小需要的检测器数量
void MultiDetectorAnomalyDetector::setMinRequiredDetectors(size_t count) {
    minRequiredDetectors = std::max(static_cast<size_t>(1), count);
}

// 获取最小需要的检测器数量
size_t MultiDetectorAnomalyDetector::getMinRequiredDetectors() const {
    return minRequiredDetectors;
}

// 执行多检测器异常检测
AnomalyResult MultiDetectorAnomalyDetector::doDetectAnomaly(const LocationInfo& location, const std::vector<LocationInfo>& context) {
    AnomalyResult result;
    result.isAnomaly = false;
    result.confidence = 0.0;
    
    std::lock_guard<std::mutex> lock(detectorsMutex);
    
    // 如果没有子检测器，直接返回非异常
    if (detectors.empty()) {
        return result;
    }
    
    try {
        // 收集所有子检测器的结果
        std::vector<AnomalyResult> detectorResults;
        std::vector<double> weights;
        
        for (const auto& [detector, weight] : detectors) {
            if (detector->isEnabled()) {
                AnomalyResult detectorResult = detector->detectAnomaly(location, context);
                detectorResults.push_back(detectorResult);
                weights.push_back(weight);
            }
        }
        
        // 如果没有启用的检测器，直接返回非异常
        if (detectorResults.empty()) {
            return result;
        }
        
        // 根据融合策略合并结果
        switch (fusionStrategy) {
            case FusionStrategy::MAJORITY_VOTE:
                result = fuseByMajorityVote(detectorResults);
                break;
            case FusionStrategy::WEIGHTED_AVERAGE:
                result = fuseByWeightedAverage(detectorResults, weights);
                break;
            case FusionStrategy::THRESHOLD_BASED:
                result = fuseByThreshold(detectorResults);
                break;
            default:
                result = fuseByMajorityVote(detectorResults);
                break;
        }
        
        // 添加异常信息
        if (result.isAnomaly) {
            result.anomalyInfo["type"] = "MULTI_DETECTOR";
            result.anomalyInfo["fusionStrategy"] = fusionStrategyToString(fusionStrategy);
        }
    } catch (const std::exception& e) {
        LOG_ERROR("Error in multi-detector anomaly detection: %s", e.what());
    }
    
    return result;
}

// 多数投票融合
AnomalyResult MultiDetectorAnomalyDetector::fuseByMajorityVote(const std::vector<AnomalyResult>& results) {
    AnomalyResult fusedResult;
    
    // 统计异常判定的数量
    int anomalyCount = 0;
    double totalConfidence = 0.0;
    
    for (const auto& result : results) {
        if (result.isAnomaly) {
            anomalyCount++;
            totalConfidence += result.confidence;
        }
    }
    
    // 判断是否超过最小需要的检测器数量
    fusedResult.isAnomaly = (anomalyCount >= minRequiredDetectors);
    
    // 计算平均置信度
    if (anomalyCount > 0) {
        fusedResult.confidence = totalConfidence / anomalyCount;
    } else {
        fusedResult.confidence = 0.0;
    }
    
    return fusedResult;
}

// 加权平均融合
AnomalyResult MultiDetectorAnomalyDetector::fuseByWeightedAverage(
    const std::vector<AnomalyResult>& results, 
    const std::vector<double>& weights) {
    
    AnomalyResult fusedResult;
    
    // 计算加权和
    double weightedSum = 0.0;
    double totalWeight = 0.0;
    
    for (size_t i = 0; i < results.size() && i < weights.size(); i++) {
        weightedSum += results[i].confidence * weights[i];
        totalWeight += weights[i];
    }
    
    // 计算加权平均置信度
    if (totalWeight > 0.0) {
        fusedResult.confidence = weightedSum / totalWeight;
    } else {
        fusedResult.confidence = 0.0;
    }
    
    // 根据置信度判断是否为异常
    fusedResult.isAnomaly = (fusedResult.confidence >= threshold);
    
    return fusedResult;
}

// 阈值融合
AnomalyResult MultiDetectorAnomalyDetector::fuseByThreshold(const std::vector<AnomalyResult>& results) {
    AnomalyResult fusedResult;
    
    // 如果有任何一个检测器判定为异常，就认为是异常
    for (const auto& result : results) {
        if (result.isAnomaly && result.confidence >= threshold) {
            fusedResult.isAnomaly = true;
            
            // 取最高的置信度
            if (result.confidence > fusedResult.confidence) {
                fusedResult.confidence = result.confidence;
            }
        }
    }
    
    return fusedResult;
}

// 获取检测器名称
std::string MultiDetectorAnomalyDetector::getName() const {
    return "MultiDetectorAnomalyDetector";
}