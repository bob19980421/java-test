// Utils.cpp - 通用工具实现

#include "Utils.h"
#include <cmath>
#include <algorithm>
#include <sstream>
#include <iomanip>
#include <random>
#include <chrono>
#include <thread>

// 计算两个经纬度之间的距离（米）
double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
    const double R = 6371000.0; // 地球半径（米）
    
    // 将角度转换为弧度
    double radLat1 = lat1 * M_PI / 180.0;
    double radLon1 = lon1 * M_PI / 180.0;
    double radLat2 = lat2 * M_PI / 180.0;
    double radLon2 = lon2 * M_PI / 180.0;
    
    // 计算经纬度差值
    double dLat = radLat2 - radLat1;
    double dLon = radLon2 - radLon1;
    
    // 应用Haversine公式
    double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::cos(radLat1) * std::cos(radLat2) *
               std::sin(dLon / 2) * std::sin(dLon / 2);
    
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    double distance = R * c; // 距离（米）
    
    return distance;
}

// 计算两个位置信息之间的距离
double calculateDistance(const LocationInfo& loc1, const LocationInfo& loc2) {
    return calculateDistance(loc1.latitude, loc1.longitude, loc2.latitude, loc2.longitude);
}

// 计算方位角（从loc1到loc2的角度，0-360度）
double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
    // 将角度转换为弧度
    double radLat1 = lat1 * M_PI / 180.0;
    double radLon1 = lon1 * M_PI / 180.0;
    double radLat2 = lat2 * M_PI / 180.0;
    double radLon2 = lon2 * M_PI / 180.0;
    
    // 计算差值
    double dLon = radLon2 - radLon1;
    
    // 计算方位角
    double y = std::sin(dLon) * std::cos(radLat2);
    double x = std::cos(radLat1) * std::sin(radLat2) -
               std::sin(radLat1) * std::cos(radLat2) * std::cos(dLon);
    double bearing = std::atan2(y, x) * 180.0 / M_PI;
    
    // 确保方位角在0-360度范围内
    if (bearing < 0) {
        bearing += 360.0;
    }
    
    return bearing;
}

// 计算方位角（从loc1到loc2的角度）
double calculateBearing(const LocationInfo& loc1, const LocationInfo& loc2) {
    return calculateBearing(loc1.latitude, loc1.longitude, loc2.latitude, loc2.longitude);
}

// WGS84坐标转换为GCJ02坐标（火星坐标系）
LocationInfo wgs84ToGcj02(const LocationInfo& wgs84) {
    LocationInfo gcj02 = wgs84; // 复制原始位置信息
    
    // 这里简化实现，实际应用中应该使用完整的坐标转换算法
    // 注意：真实的GCJ02转换涉及到复杂的加密算法
    double lat = wgs84.latitude;
    double lon = wgs84.longitude;
    
    // 判断是否在国内，不在国内则不转换
    if (!isInsideChina(lat, lon)) {
        return gcj02;
    }
    
    // 简化的转换算法（仅作示例）
    double a = 6378245.0; // 地球椭球体长半轴
    double ee = 0.00669342162296594323; // 地球椭球体偏心率平方
    
    double dLat = transformLat(lon - 105.0, lat - 35.0);
    double dLon = transformLon(lon - 105.0, lat - 35.0);
    
    double radLat = lat / 180.0 * M_PI;
    double magic = std::sin(radLat);
    magic = 1 - ee * magic * magic;
    double sqrtMagic = std::sqrt(magic);
    
    dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * M_PI);
    dLon = (dLon * 180.0) / (a / sqrtMagic * std::cos(radLat) * M_PI);
    
    gcj02.latitude = lat + dLat;
    gcj02.longitude = lon + dLon;
    
    return gcj02;
}

// GCJ02坐标转换为WGS84坐标
LocationInfo gcj02ToWgs84(const LocationInfo& gcj02) {
    LocationInfo wgs84 = gcj02; // 复制原始位置信息
    
    // 这里简化实现，实际应用中应该使用完整的坐标转换算法
    double lat = gcj02.latitude;
    double lon = gcj02.longitude;
    
    // 判断是否在国内，不在国内则不转换
    if (!isInsideChina(lat, lon)) {
        return wgs84;
    }
    
    // 简化的转换算法（仅作示例）
    double a = 6378245.0; // 地球椭球体长半轴
    double ee = 0.00669342162296594323; // 地球椭球体偏心率平方
    
    double dLat = transformLat(lon - 105.0, lat - 35.0);
    double dLon = transformLon(lon - 105.0, lat - 35.0);
    
    double radLat = lat / 180.0 * M_PI;
    double magic = std::sin(radLat);
    magic = 1 - ee * magic * magic;
    double sqrtMagic = std::sqrt(magic);
    
    dLat = (dLat * 180.0) / ((a * (1 - ee)) / (magic * sqrtMagic) * M_PI);
    dLon = (dLon * 180.0) / (a / sqrtMagic * std::cos(radLat) * M_PI);
    
    // 反向转换
    wgs84.latitude = lat - dLat;
    wgs84.longitude = lon - dLon;
    
    return wgs84;
}

// 计算纬度转换量
double transformLat(double x, double y) {
    double ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * std::sqrt(std::abs(x));
    ret += (20.0 * std::sin(6.0 * x * M_PI) + 20.0 * std::sin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * std::sin(y * M_PI) + 40.0 * std::sin(y / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (160.0 * std::sin(y / 12.0 * M_PI) + 320 * std::sin(y * M_PI / 30.0)) * 2.0 / 3.0;
    return ret;
}

// 计算经度转换量
double transformLon(double x, double y) {
    double ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * std::sqrt(std::abs(x));
    ret += (20.0 * std::sin(6.0 * x * M_PI) + 20.0 * std::sin(2.0 * x * M_PI)) * 2.0 / 3.0;
    ret += (20.0 * std::sin(x * M_PI) + 40.0 * std::sin(x / 3.0 * M_PI)) * 2.0 / 3.0;
    ret += (150.0 * std::sin(x / 12.0 * M_PI) + 300.0 * std::sin(x / 30.0 * M_PI)) * 2.0 / 3.0;
    return ret;
}

// 判断坐标是否在中国境内
bool isInsideChina(double lat, double lon) {
    // 简化判断，实际应用中应该使用更精确的边界判断
    return lat >= 0.8293 && lat <= 55.8271 && lon >= 73.4976 && lon <= 135.0841;
}

// 深拷贝LocationInfo对象
std::shared_ptr<LocationInfo> deepCopyLocationInfo(const LocationInfo& source) {
    auto copy = std::make_shared<LocationInfo>();
    
    // 复制所有基本属性
    copy->latitude = source.latitude;
    copy->longitude = source.longitude;
    copy->altitude = source.altitude;
    copy->accuracy = source.accuracy;
    copy->speed = source.speed;
    copy->direction = source.direction;
    copy->timestamp = source.timestamp;
    copy->dataSource = source.dataSource;
    copy->status = source.status;
    copy->satelliteCount = source.satelliteCount;
    copy->signalStrength = source.signalStrength;
    copy->locationType = source.locationType;
    copy->provider = source.provider;
    
    // 深拷贝extras映射
    copy->extras = source.extras;
    
    return copy;
}

// 深拷贝CorrectedLocation对象
std::shared_ptr<CorrectedLocation> deepCopyCorrectedLocation(const CorrectedLocation& source) {
    auto copy = std::make_shared<CorrectedLocation>();
    
    // 复制原始位置信息
    copy->originalLocation = *deepCopyLocationInfo(source.originalLocation);
    
    // 复制其他属性
    copy->correctedLatitude = source.correctedLatitude;
    copy->correctedLongitude = source.correctedLongitude;
    copy->correctedAltitude = source.correctedAltitude;
    copy->correctionAccuracy = source.correctionAccuracy;
    copy->correctionMethod = source.correctionMethod;
    copy->confidenceScore = source.confidenceScore;
    copy->isAnomaly = source.isAnomaly;
    copy->anomalyType = source.anomalyType;
    copy->correctionTime = source.correctionTime;
    copy->correctionDistance = source.correctionDistance;
    copy->isFused = source.isFused;
    copy->sourceCount = source.sourceCount;
    
    // 深拷贝correctionDetails映射
    copy->correctionDetails = source.correctionDetails;
    
    return copy;
}

// 生成函数防抖器
DebouncedFunction createDebounce(std::function<void()> func, long long delayMs) {
    // 创建共享的标志和计时器
    auto isScheduled = std::make_shared<bool>(false);
    auto lastCallTime = std::make_shared<long long>(0);
    auto timerMutex = std::make_shared<std::mutex>();
    
    return [=](bool immediate = false) {
        long long currentTime = getCurrentTimestampMs();
        
        // 如果是立即执行，则取消之前的定时器并立即执行
        if (immediate) {
            std::lock_guard<std::mutex> lock(*timerMutex);
            *isScheduled = false;
            func();
            return;
        }
        
        // 检查是否需要更新定时器
        {{
            std::lock_guard<std::mutex> lock(*timerMutex);
            *lastCallTime = currentTime;
            
            if (!*isScheduled) {
                *isScheduled = true;
                
                // 创建新线程来处理延迟执行
                std::thread([=]() {
                    std::this_thread::sleep_for(std::chrono::milliseconds(delayMs));
                    
                    long long callTime;
                    bool shouldExecute;
                    {{
                        std::lock_guard<std::mutex> lock(*timerMutex);
                        callTime = *lastCallTime;
                        shouldExecute = *isScheduled && (currentTime == callTime);
                        *isScheduled = false;
                    }}
                    
                    if (shouldExecute) {
                        func();
                    }
                }).detach();
            }
        }}
    };
}

// 生成函数节流器
ThrottledFunction createThrottle(std::function<void()> func, long long intervalMs) {
    // 创建共享的最后执行时间
    auto lastExecutionTime = std::make_shared<long long>(0);
    auto mutex = std::make_shared<std::mutex>();
    
    return [=](bool leading = true, bool trailing = false) {
        long long currentTime = getCurrentTimestampMs();
        
        // 如果是首次调用且leading为true，则立即执行
        if (leading && *lastExecutionTime == 0) {
            std::lock_guard<std::mutex> lock(*mutex);
            if (*lastExecutionTime == 0) { // 双重检查
                func();
                *lastExecutionTime = currentTime;
            }
            return;
        }
        
        // 检查是否可以执行
        long long elapsed = currentTime - *lastExecutionTime;
        
        if (elapsed >= intervalMs) {
            std::lock_guard<std::mutex> lock(*mutex);
            elapsed = currentTime - *lastExecutionTime;
            if (elapsed >= intervalMs) { // 双重检查
                func();
                *lastExecutionTime = currentTime;
            }
        } else if (trailing) {
            // 对于trailing edge，我们创建一个延迟执行的任务
            // 这里简化处理，实际应用中可能需要更复杂的逻辑
        }
    };
}

// 将字符串转换为double
double parseDouble(const std::string& str, double defaultValue) {
    try {
        size_t pos;
        double result = std::stod(str, &pos);
        // 确保整个字符串都被解析
        if (pos == str.length()) {
            return result;
        }
    } catch (...) {
        // 忽略解析错误
    }
    return defaultValue;
}

// 将double转换为字符串，控制精度
std::string doubleToString(double value, int precision) {
    std::stringstream ss;
    ss << std::fixed << std::setprecision(precision) << value;
    return ss.str();
}

// 生成UUID
std::string generateUUID() {
    // 创建随机数生成器
    static std::random_device rd;
    static std::mt19937 gen(rd());
    static std::uniform_int_distribution<uint64_t> dist;
    
    // 生成随机数
    uint64_t uuid1 = dist(gen);
    uint64_t uuid2 = dist(gen);
    
    // 格式化UUID
    std::stringstream ss;
    ss << std::hex << std::setfill('0')
       << std::setw(8) << (uuid1 >> 32) << "-"
       << std::setw(4) << ((uuid1 >> 16) & 0xffff) << "-"
       << std::setw(4) << (uuid1 & 0xffff) << "-"
       << std::setw(4) << (uuid2 >> 48) << "-"
       << std::setw(12) << (uuid2 & 0xffffffffffffULL);
    
    return ss.str();
}

// 获取当前时间戳（毫秒）
long long getCurrentTimestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// 获取当前时间字符串
std::string getCurrentTimeString(const std::string& format) {
    auto now = std::chrono::system_clock::now();
    auto now_c = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    
#ifdef _WIN32
    std::tm localTime;
    localtime_s(&localTime, &now_c);
    ss << std::put_time(&localTime, format.c_str());
#else
    std::tm localTime;
    localtime_r(&now_c, &localTime);
    ss << std::put_time(&localTime, format.c_str());
#endif
    
    return ss.str();
}

// 休眠指定毫秒
void sleepMs(long long milliseconds) {
    std::this_thread::sleep_for(std::chrono::milliseconds(milliseconds));
}

// 计算向量的平均值
double calculateAverage(const std::vector<double>& values) {
    if (values.empty()) {
        return 0.0;
    }
    
    double sum = 0.0;
    for (double value : values) {
        sum += value;
    }
    
    return sum / values.size();
}

// 计算向量的标准差
double calculateStandardDeviation(const std::vector<double>& values) {
    if (values.size() < 2) {
        return 0.0;
    }
    
    double avg = calculateAverage(values);
    double sumSquaredDiff = 0.0;
    
    for (double value : values) {
        double diff = value - avg;
        sumSquaredDiff += diff * diff;
    }
    
    return std::sqrt(sumSquaredDiff / (values.size() - 1));
}

// 计算加权平均值
double calculateWeightedAverage(const std::vector<double>& values, const std::vector<double>& weights) {
    if (values.empty() || weights.empty() || values.size() != weights.size()) {
        return 0.0;
    }
    
    double sumValues = 0.0;
    double sumWeights = 0.0;
    
    for (size_t i = 0; i < values.size(); ++i) {
        sumValues += values[i] * weights[i];
        sumWeights += weights[i];
    }
    
    if (sumWeights == 0.0) {
        return 0.0;
    }
    
    return sumValues / sumWeights;
}