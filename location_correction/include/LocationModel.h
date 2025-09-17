// LocationModel.h - 位置数据模型定义

#ifndef LOCATION_MODEL_H
#define LOCATION_MODEL_H

#include <string>
#include <vector>

// 数据源类型枚举
enum class DataSourceType {
    GNSS,          // 全球导航卫星系统
    WIFI,          // Wi-Fi定位
    BASE_STATION,  // 基站定位
    SENSOR,        // 传感器定位
    OTHER          // 其他定位方式
};

// 位置状态枚举
enum class LocationStatus {
    VALID,         // 有效位置
    INVALID,       // 无效位置
    LOW_ACCURACY,  // 低精度位置
    ANOMALY        // 异常位置
};

// 位置数据结构
struct LocationInfo {
    double latitude;     // 纬度
    double longitude;    // 经度
    double altitude;     // 海拔高度，可选
    double accuracy;     // 精度（米）
    long long timestamp; // 时间戳（毫秒）
    double speed;        // 速度（米/秒），可选
    double direction;    // 方向（度），可选
    DataSourceType source; // 数据源类型
    std::string sourceId; // 数据源ID

    // 构造函数
    LocationInfo() : 
        latitude(0.0), 
        longitude(0.0), 
        altitude(0.0), 
        accuracy(0.0), 
        timestamp(0), 
        speed(0.0), 
        direction(0.0), 
        source(DataSourceType::OTHER) {}

    LocationInfo(double lat, double lon, double acc, long long ts, DataSourceType src) : 
        latitude(lat), 
        longitude(lon), 
        altitude(0.0), 
        accuracy(acc), 
        timestamp(ts), 
        speed(0.0), 
        direction(0.0), 
        source(src) {}
};

// 纠偏结果结构
struct CorrectedLocation {
    LocationInfo original;  // 原始位置
    LocationInfo corrected; // 纠偏后位置
    double confidence;      // 置信度（0-1之间）
    std::string correctionType; // 纠偏类型

    // 构造函数
    CorrectedLocation() : confidence(0.0) {}

    CorrectedLocation(const LocationInfo& orig, const LocationInfo& corr, double conf, const std::string& type) :
        original(orig), 
        corrected(corr), 
        confidence(conf), 
        correctionType(type) {}
};

// 位置变化监听器接口
class LocationChangeListener {
public:
    virtual ~LocationChangeListener() = default;
    virtual void onLocationChanged(const CorrectedLocation& location) = 0;
    virtual void onStatusChanged(LocationStatus status) = 0;
};

#endif // LOCATION_MODEL_H