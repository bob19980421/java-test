// DataProcessor.cpp - 数据预处理实现

#include "DataProcessor.h"
#include "Logger.h"
#include "Utils.h"
#include <algorithm>
#include <numeric>
#include <cmath>

// DataProcessor构造函数
DataProcessor::DataProcessor() : 
    enabled(true),
    priority(0),
    params() {
}

// DataProcessor析构函数
DataProcessor::~DataProcessor() {
}

// 设置处理器是否启用
void DataProcessor::setEnabled(bool enable) {
    enabled = enable;
}

// 检查处理器是否启用
bool DataProcessor::isEnabled() const {
    return enabled;
}

// 设置处理器优先级
void DataProcessor::setPriority(int pri) {
    priority = pri;
}

// 获取处理器优先级
int DataProcessor::getPriority() const {
    return priority;
}

// 设置处理器参数
void DataProcessor::setParameter(const std::string& key, const std::string& value) {
    params[key] = value;
}

// 获取处理器参数
std::string DataProcessor::getParameter(const std::string& key, const std::string& defaultValue) const {
    auto it = params.find(key);
    if (it != params.end()) {
        return it->second;
    }
    return defaultValue;
}

// BaseDataProcessor构造函数
BaseDataProcessor::BaseDataProcessor() : DataProcessor() {
}

// 处理单个位置数据
std::shared_ptr<LocationInfo> BaseDataProcessor::process(const LocationInfo& location) {
    if (!isEnabled()) {
        return std::make_shared<LocationInfo>(location);
    }
    
    try {
        // 创建位置数据的副本
        auto processedLocation = std::make_shared<LocationInfo>(location);
        
        // 执行具体的处理逻辑
        bool success = doProcess(*processedLocation);
        
        if (!success) {
            LOG_WARNING("Processing failed for location: %s", location.toString().c_str());
            // 返回原始位置的副本
            return std::make_shared<LocationInfo>(location);
        }
        
        return processedLocation;
    } catch (const std::exception& e) {
        LOG_ERROR("Exception in data processing: %s", e.what());
        // 发生异常时返回原始位置的副本
        return std::make_shared<LocationInfo>(location);
    }
}

// 批量处理位置数据
std::vector<std::shared_ptr<LocationInfo>> BaseDataProcessor::batchProcess(const std::vector<LocationInfo>& locations) {
    std::vector<std::shared_ptr<LocationInfo>> processedLocations;
    processedLocations.reserve(locations.size());
    
    for (const auto& location : locations) {
        processedLocations.push_back(process(location));
    }
    
    return processedLocations;
}

// 具体的处理逻辑（虚函数，子类需要实现）
bool BaseDataProcessor::doProcess(LocationInfo& location) {
    // 基类实现为空，子类需要重写此方法
    return true;
}

// AccuracyFilterProcessor构造函数
AccuracyFilterProcessor::AccuracyFilterProcessor() : BaseDataProcessor() {
    // 设置默认参数
    minAccuracy = 0.0;
    maxAccuracy = 100.0; // 默认最大精度为100米
}

// 设置精度范围
void AccuracyFilterProcessor::setAccuracyRange(double minAcc, double maxAcc) {
    minAccuracy = std::max(0.0, minAcc);
    maxAccuracy = std::max(minAccuracy, maxAcc);
}

// 执行精度过滤
bool AccuracyFilterProcessor::doProcess(LocationInfo& location) {
    // 检查精度是否在有效范围内
    if (location.accuracy < minAccuracy || location.accuracy > maxAccuracy) {
        // 如果精度不在范围内，标记为低精度
        location.status = LocationStatus::LOW_ACCURACY;
        LOG_DEBUG("Location accuracy out of range: %f (min: %f, max: %f)", 
                 location.accuracy, minAccuracy, maxAccuracy);
    }
    
    // 即使精度不在范围内，我们也返回true，因为我们只是标记位置，而不是拒绝它
    return true;
}

// 获取处理器名称
std::string AccuracyFilterProcessor::getName() const {
    return "AccuracyFilterProcessor";
}

// TimeFilterProcessor构造函数
TimeFilterProcessor::TimeFilterProcessor() : BaseDataProcessor() {
    // 设置默认参数
    maxTimeDiff = 300000; // 默认最大时间差为5分钟（毫秒）
}

// 设置最大时间差
void TimeFilterProcessor::setMaxTimeDiff(long long diffMs) {
    maxTimeDiff = std::max(0LL, diffMs);
}

// 执行时间过滤
bool TimeFilterProcessor::doProcess(LocationInfo& location) {
    // 获取当前时间戳
    long long currentTime = getCurrentTimestampMs();
    
    // 计算时间差
    long long timeDiff = currentTime - location.timestamp;
    
    // 检查时间差是否在有效范围内
    if (timeDiff > maxTimeDiff) {
        // 如果时间差太大，标记为无效
        location.status = LocationStatus::INVALID;
        LOG_DEBUG("Location time difference too large: %lld ms (max: %lld ms)", 
                 timeDiff, maxTimeDiff);
    }
    
    return true;
}

// 获取处理器名称
std::string TimeFilterProcessor::getName() const {
    return "TimeFilterProcessor";
}

// OutlierDetectionProcessor构造函数
OutlierDetectionProcessor::OutlierDetectionProcessor() : BaseDataProcessor() {
    // 设置默认参数
    thresholdFactor = 2.0; // 默认使用2倍标准差作为阈值
    maxHistorySize = 50; // 最大历史数据量
    minSampleSize = 5; // 最小样本数量
}

// 设置阈值因子
void OutlierDetectionProcessor::setThresholdFactor(double factor) {
    thresholdFactor = std::max(1.0, factor);
}

// 设置最大历史数据量
void OutlierDetectionProcessor::setMaxHistorySize(size_t size) {
    std::lock_guard<std::mutex> lock(historyMutex);
    maxHistorySize = std::max(minSampleSize, size);
    
    // 如果历史数据超过最大容量，删除最早的数据
    trimHistory();
}

// 裁剪历史数据
void OutlierDetectionProcessor::trimHistory() {
    while (history.size() > maxHistorySize) {
        history.pop_front();
    }
}

// 执行异常点检测
bool OutlierDetectionProcessor::doProcess(LocationInfo& location) {
    // 只有有效的位置才进行异常检测
    if (!location.isValid()) {
        return true;
    }
    
    std::lock_guard<std::mutex> lock(historyMutex);
    
    // 如果历史数据不足，直接添加并返回
    if (history.size() < minSampleSize) {
        history.push_back(location);
        return true;
    }
    
    // 计算历史数据的统计信息
    double avgLat = 0.0, avgLon = 0.0, stdDev = 0.0;
    calculateStatistics(avgLat, avgLon, stdDev);
    
    // 计算当前位置与平均值的距离
    double distance = calculateDistance(location.latitude, location.longitude, avgLat, avgLon);
    
    // 判断是否为异常点
    if (distance > thresholdFactor * stdDev) {
        // 标记为异常点
        location.status = LocationStatus::ANOMALY;
        location.setExtra("isOutlier", "true");
        location.setExtra("outlierDistance", doubleToString(distance, 2));
        location.setExtra("threshold", doubleToString(thresholdFactor * stdDev, 2));
        
        LOG_DEBUG("Detected outlier: distance=%f > threshold=%f", 
                 distance, thresholdFactor * stdDev);
    } else {
        // 添加到历史数据
        history.push_back(location);
        trimHistory();
    }
    
    return true;
}

// 计算历史数据的统计信息
void OutlierDetectionProcessor::calculateStatistics(double& avgLat, double& avgLon, double& stdDev) {
    // 计算经纬度平均值
    double sumLat = 0.0, sumLon = 0.0;
    for (const auto& loc : history) {
        sumLat += loc.latitude;
        sumLon += loc.longitude;
    }
    
    avgLat = sumLat / history.size();
    avgLon = sumLon / history.size();
    
    // 计算距离的标准差
    std::vector<double> distances;
    for (const auto& loc : history) {
        double dist = calculateDistance(loc.latitude, loc.longitude, avgLat, avgLon);
        distances.push_back(dist);
    }
    
    stdDev = calculateStandardDeviation(distances);
    
    // 如果标准差为0（所有点相同），设置一个默认值
    if (stdDev < 0.0001) {
        stdDev = 1.0; // 默认1米的标准差
    }
}

// 获取处理器名称
std::string OutlierDetectionProcessor::getName() const {
    return "OutlierDetectionProcessor";
}

// 清空历史数据
void OutlierDetectionProcessor::clearHistory() {
    std::lock_guard<std::mutex> lock(historyMutex);
    history.clear();
}

// CoordinateConverterProcessor构造函数
CoordinateConverterProcessor::CoordinateConverterProcessor() : BaseDataProcessor() {
    // 设置默认参数
    sourceSystem = CoordinateSystem::WGS84;
    targetSystem = CoordinateSystem::GCJ02;
}

// 设置坐标转换参数
void CoordinateConverterProcessor::setConversionParams(CoordinateSystem src, CoordinateSystem dest) {
    sourceSystem = src;
    targetSystem = dest;
}

// 执行坐标转换
bool CoordinateConverterProcessor::doProcess(LocationInfo& location) {
    // 如果源系统和目标系统相同，无需转换
    if (sourceSystem == targetSystem) {
        return true;
    }
    
    // 执行坐标转换
    try {
        if (sourceSystem == CoordinateSystem::WGS84 && targetSystem == CoordinateSystem::GCJ02) {
            // WGS84转换为GCJ02
            LocationInfo gcj02 = wgs84ToGcj02(location);
            location.latitude = gcj02.latitude;
            location.longitude = gcj02.longitude;
            location.setExtra("coordinateSystem", "GCJ02");
        } else if (sourceSystem == CoordinateSystem::GCJ02 && targetSystem == CoordinateSystem::WGS84) {
            // GCJ02转换为WGS84
            LocationInfo wgs84 = gcj02ToWgs84(location);
            location.latitude = wgs84.latitude;
            location.longitude = wgs84.longitude;
            location.setExtra("coordinateSystem", "WGS84");
        }
        
        LOG_DEBUG("Converted coordinates from %d to %d", 
                 static_cast<int>(sourceSystem), static_cast<int>(targetSystem));
        
        return true;
    } catch (const std::exception& e) {
        LOG_ERROR("Error converting coordinates: %s", e.what());
        return false;
    }
}

// 获取处理器名称
std::string CoordinateConverterProcessor::getName() const {
    return "CoordinateConverterProcessor";
}

// ProcessorChain构造函数
ProcessorChain::ProcessorChain() : processors(), mutex() {
}

// 添加处理器到链中
bool ProcessorChain::addProcessor(std::shared_ptr<DataProcessor> processor) {
    if (!processor) {
        return false;
    }
    
    std::lock_guard<std::mutex> lock(mutex);
    processors.push_back(processor);
    
    // 根据优先级排序处理器
    sortProcessorsByPriority();
    
    LOG_INFO("Added processor to chain: %s", processor->getName().c_str());
    return true;
}

// 从链中移除处理器
bool ProcessorChain::removeProcessor(const std::string& processorName) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = std::find_if(processors.begin(), processors.end(),
        [&processorName](const std::shared_ptr<DataProcessor>& p) {
            return p->getName() == processorName;
        });
    
    if (it != processors.end()) {
        LOG_INFO("Removed processor from chain: %s", processorName.c_str());
        processors.erase(it);
        return true;
    }
    
    LOG_WARNING("Processor not found in chain: %s", processorName.c_str());
    return false;
}

// 根据优先级排序处理器
void ProcessorChain::sortProcessorsByPriority() {
    std::sort(processors.begin(), processors.end(),
        [](const std::shared_ptr<DataProcessor>& p1, const std::shared_ptr<DataProcessor>& p2) {
            return p1->getPriority() < p2->getPriority();
        });
}

// 处理单个位置数据（按链顺序）
std::shared_ptr<LocationInfo> ProcessorChain::process(const LocationInfo& location) {
    std::lock_guard<std::mutex> lock(mutex);
    
    // 如果没有处理器，直接返回原始位置的副本
    if (processors.empty()) {
        return std::make_shared<LocationInfo>(location);
    }
    
    // 按顺序应用每个处理器
    auto currentLocation = std::make_shared<LocationInfo>(location);
    
    for (const auto& processor : processors) {
        if (processor->isEnabled()) {
            auto processed = processor->process(*currentLocation);
            if (processed) {
                currentLocation = processed;
            }
            
            // 如果位置变为无效，可以选择提前终止处理链
            if (!currentLocation->isValid() && 
                getParameter("stopOnInvalid", "false") == "true") {
                break;
            }
        }
    }
    
    return currentLocation;
}

// 批量处理位置数据
std::vector<std::shared_ptr<LocationInfo>> ProcessorChain::batchProcess(const std::vector<LocationInfo>& locations) {
    std::vector<std::shared_ptr<LocationInfo>> processedLocations;
    processedLocations.reserve(locations.size());
    
    for (const auto& location : locations) {
        processedLocations.push_back(process(location));
    }
    
    return processedLocations;
}

// 获取链中所有处理器的名称
std::vector<std::string> ProcessorChain::getProcessorNames() const {
    std::lock_guard<std::mutex> lock(mutex);
    
    std::vector<std::string> names;
    for (const auto& processor : processors) {
        names.push_back(processor->getName());
    }
    
    return names;
}

// 清空处理链
void ProcessorChain::clear() {
    std::lock_guard<std::mutex> lock(mutex);
    processors.clear();
    LOG_INFO("Processor chain cleared");
}

// 启用链中所有处理器
void ProcessorChain::enableAllProcessors() {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (const auto& processor : processors) {
        processor->setEnabled(true);
    }
    
    LOG_INFO("All processors in chain enabled");
}

// 禁用链中所有处理器
void ProcessorChain::disableAllProcessors() {
    std::lock_guard<std::mutex> lock(mutex);
    
    for (const auto& processor : processors) {
        processor->setEnabled(false);
    }
    
    LOG_INFO("All processors in chain disabled");
}

// 根据名称获取处理器
std::shared_ptr<DataProcessor> ProcessorChain::getProcessorByName(const std::string& name) {
    std::lock_guard<std::mutex> lock(mutex);
    
    auto it = std::find_if(processors.begin(), processors.end(),
        [&name](const std::shared_ptr<DataProcessor>& p) {
            return p->getName() == name;
        });
    
    if (it != processors.end()) {
        return *it;
    }
    
    return nullptr;
}