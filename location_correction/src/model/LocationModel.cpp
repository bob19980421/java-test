// LocationModel.cpp - 位置数据模型实现

#include "LocationModel.h"
#include <sstream>
#include <iomanip>

// 构造函数实现
LocationInfo::LocationInfo() : 
    latitude(0.0),
    longitude(0.0),
    altitude(0.0),
    accuracy(0.0),
    speed(0.0),
    direction(0.0),
    timestamp(0),
    dataSource(DataSourceType::UNKNOWN),
    status(LocationStatus::INVALID),
    satelliteCount(0),
    signalStrength(0),
    locationType(""),
    provider(""),
    extras() {
}

LocationInfo::LocationInfo(double lat, double lng, double acc, DataSourceType source) : 
    latitude(lat),
    longitude(lng),
    altitude(0.0),
    accuracy(acc),
    speed(0.0),
    direction(0.0),
    timestamp(getCurrentTimestampMs()),
    dataSource(source),
    status(LocationStatus::VALID),
    satelliteCount(0),
    signalStrength(0),
    locationType(""),
    provider(""),
    extras() {
}

// 复制构造函数
LocationInfo::LocationInfo(const LocationInfo& other) : 
    latitude(other.latitude),
    longitude(other.longitude),
    altitude(other.altitude),
    accuracy(other.accuracy),
    speed(other.speed),
    direction(other.direction),
    timestamp(other.timestamp),
    dataSource(other.dataSource),
    status(other.status),
    satelliteCount(other.satelliteCount),
    signalStrength(other.signalStrength),
    locationType(other.locationType),
    provider(other.provider),
    extras(other.extras) {
}

// 赋值运算符
LocationInfo& LocationInfo::operator=(const LocationInfo& other) {
    if (this != &other) {
        latitude = other.latitude;
        longitude = other.longitude;
        altitude = other.altitude;
        accuracy = other.accuracy;
        speed = other.speed;
        direction = other.direction;
        timestamp = other.timestamp;
        dataSource = other.dataSource;
        status = other.status;
        satelliteCount = other.satelliteCount;
        signalStrength = other.signalStrength;
        locationType = other.locationType;
        provider = other.provider;
        extras = other.extras;
    }
    return *this;
}

// 检查位置数据是否有效
bool LocationInfo::isValid() const {
    return status == LocationStatus::VALID && 
           latitude >= -90.0 && latitude <= 90.0 && 
           longitude >= -180.0 && longitude <= 180.0 && 
           accuracy >= 0.0 && 
           timestamp > 0;
}

// 获取当前时间戳（毫秒）
long long LocationInfo::getCurrentTimestampMs() {
    return std::chrono::duration_cast<std::chrono::milliseconds>(
        std::chrono::system_clock::now().time_since_epoch()).count();
}

// 转换为字符串表示
std::string LocationInfo::toString() const {
    std::stringstream ss;
    ss << "LocationInfo{" 
       << "lat=" << std::fixed << std::setprecision(6) << latitude << ", "
       << "lng=" << std::fixed << std::setprecision(6) << longitude << ", "
       << "alt=" << altitude << ", "
       << "acc=" << accuracy << ", "
       << "spd=" << speed << ", "
       << "dir=" << direction << ", "
       << "ts=" << timestamp << ", "
       << "src=" << static_cast<int>(dataSource) << ", "
       << "status=" << static_cast<int>(status) << ", "
       << "satellites=" << satelliteCount << ", "
       << "signal=" << signalStrength
       << "}";
    return ss.str();
}

// 设置额外信息
void LocationInfo::setExtra(const std::string& key, const std::string& value) {
    extras[key] = value;
}

// 获取额外信息
std::string LocationInfo::getExtra(const std::string& key, const std::string& defaultValue) const {
    auto it = extras.find(key);
    if (it != extras.end()) {
        return it->second;
    }
    return defaultValue;
}

// 检查是否存在额外信息
bool LocationInfo::hasExtra(const std::string& key) const {
    return extras.find(key) != extras.end();
}

// CorrectedLocation构造函数
CorrectedLocation::CorrectedLocation(const LocationInfo& original) : 
    originalLocation(original),
    correctedLatitude(original.latitude),
    correctedLongitude(original.longitude),
    correctedAltitude(original.altitude),
    correctionAccuracy(original.accuracy),
    correctionMethod("none"),
    confidenceScore(1.0),
    isAnomaly(false),
    anomalyType(""),
    correctionTime(LocationInfo::getCurrentTimestampMs()),
    correctionDistance(0.0),
    isFused(false),
    sourceCount(1),
    correctionDetails() {
}

// 复制构造函数
CorrectedLocation::CorrectedLocation(const CorrectedLocation& other) : 
    originalLocation(other.originalLocation),
    correctedLatitude(other.correctedLatitude),
    correctedLongitude(other.correctedLongitude),
    correctedAltitude(other.correctedAltitude),
    correctionAccuracy(other.correctionAccuracy),
    correctionMethod(other.correctionMethod),
    confidenceScore(other.confidenceScore),
    isAnomaly(other.isAnomaly),
    anomalyType(other.anomalyType),
    correctionTime(other.correctionTime),
    correctionDistance(other.correctionDistance),
    isFused(other.isFused),
    sourceCount(other.sourceCount),
    correctionDetails(other.correctionDetails) {
}

// 赋值运算符
CorrectedLocation& CorrectedLocation::operator=(const CorrectedLocation& other) {
    if (this != &other) {
        originalLocation = other.originalLocation;
        correctedLatitude = other.correctedLatitude;
        correctedLongitude = other.correctedLongitude;
        correctedAltitude = other.correctedAltitude;
        correctionAccuracy = other.correctionAccuracy;
        correctionMethod = other.correctionMethod;
        confidenceScore = other.confidenceScore;
        isAnomaly = other.isAnomaly;
        anomalyType = other.anomalyType;
        correctionTime = other.correctionTime;
        correctionDistance = other.correctionDistance;
        isFused = other.isFused;
        sourceCount = other.sourceCount;
        correctionDetails = other.correctionDetails;
    }
    return *this;
}

// 获取纠偏后的位置信息
LocationInfo CorrectedLocation::getCorrectedLocationInfo() const {
    LocationInfo corrected = originalLocation;
    corrected.latitude = correctedLatitude;
    corrected.longitude = correctedLongitude;
    corrected.altitude = correctedAltitude;
    corrected.accuracy = correctionAccuracy;
    corrected.timestamp = correctionTime;
    return corrected;
}

// 转换为字符串表示
std::string CorrectedLocation::toString() const {
    std::stringstream ss;
    ss << "CorrectedLocation{" 
       << "correctedLat=" << std::fixed << std::setprecision(6) << correctedLatitude << ", "
       << "correctedLng=" << std::fixed << std::setprecision(6) << correctedLongitude << ", "
       << "method='" << correctionMethod << "', "
       << "confidence=" << confidenceScore << ", "
       << "isAnomaly=" << (isAnomaly ? "true" : "false") << ", "
       << "correctionDist=" << correctionDistance << ", "
       << "sourceCount=" << sourceCount
       << "}";
    return ss.str();
}

// 设置纠偏详情
void CorrectedLocation::setCorrectionDetail(const std::string& key, const std::string& value) {
    correctionDetails[key] = value;
}

// 获取纠偏详情
std::string CorrectedLocation::getCorrectionDetail(const std::string& key, const std::string& defaultValue) const {
    auto it = correctionDetails.find(key);
    if (it != correctionDetails.end()) {
        return it->second;
    }
    return defaultValue;
}

// 计算纠偏距离
void CorrectedLocation::calculateCorrectionDistance() {
    // 计算原始位置与纠偏后位置之间的距离
    const double R = 6371000.0; // 地球半径（米）
    double lat1 = originalLocation.latitude * M_PI / 180.0;
    double lon1 = originalLocation.longitude * M_PI / 180.0;
    double lat2 = correctedLatitude * M_PI / 180.0;
    double lon2 = correctedLongitude * M_PI / 180.0;
    
    double dLat = lat2 - lat1;
    double dLon = lon2 - lon1;
    
    double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
               std::cos(lat1) * std::cos(lat2) *
               std::sin(dLon / 2) * std::sin(dLon / 2);
    
    double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
    correctionDistance = R * c;
}