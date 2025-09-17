// Utils.h - 通用工具函数

#ifndef UTILS_H
#define UTILS_H

#include <cmath>
#include <string>
#include <vector>
#include <functional>
#include <chrono>
#include <memory>
#include "LocationModel.h"

namespace utils {
    // 地球半径（米）
    constexpr double EARTH_RADIUS = 6371000.0;

    // 角度转弧度
    inline double degToRad(double degrees) {
        return degrees * M_PI / 180.0;
    }

    // 弧度转角度
    inline double radToDeg(double radians) {
        return radians * 180.0 / M_PI;
    }

    // 计算两点之间的距离（米）
    inline double calculateDistance(double lat1, double lon1, double lat2, double lon2) {
        double dLat = degToRad(lat2 - lat1);
        double dLon = degToRad(lon2 - lon1);
        double a = std::sin(dLat / 2) * std::sin(dLat / 2) +
                   std::cos(degToRad(lat1)) * std::cos(degToRad(lat2)) *
                   std::sin(dLon / 2) * std::sin(dLon / 2);
        double c = 2 * std::atan2(std::sqrt(a), std::sqrt(1 - a));
        return EARTH_RADIUS * c;
    }

    // 计算两点之间的方位角（度）
    inline double calculateBearing(double lat1, double lon1, double lat2, double lon2) {
        double dLon = degToRad(lon2 - lon1);
        double y = std::sin(dLon) * std::cos(degToRad(lat2));
        double x = std::cos(degToRad(lat1)) * std::sin(degToRad(lat2)) -
                   std::sin(degToRad(lat1)) * std::cos(degToRad(lat2)) * std::cos(dLon);
        double bearing = radToDeg(std::atan2(y, x));
        return (bearing + 360.0) - std::fmod((bearing + 360.0), 360.0);
    }

    // 根据起点、距离和方位角计算终点坐标
    inline void calculateDestination(double lat1, double lon1, double distance, double bearing, double& outLat, double& outLon) {
        double lat = degToRad(lat1);
        double lon = degToRad(lon1);
        double brng = degToRad(bearing);
        double dR = distance / EARTH_RADIUS;

        double newLat = std::asin(std::sin(lat) * std::cos(dR) +
                                 std::cos(lat) * std::sin(dR) * std::cos(brng));
        double newLon = lon + std::atan2(std::sin(brng) * std::sin(dR) * std::cos(lat),
                                       std::cos(dR) - std::sin(lat) * std::sin(newLat));

        outLat = radToDeg(newLat);
        outLon = radToDeg(newLon);
        // 确保经度在[-180, 180]范围内
        outLon = std::fmod(outLon + 540.0, 360.0) - 180.0;
    }

    // 深拷贝LocationInfo对象
    inline LocationInfo deepCopyLocationInfo(const LocationInfo& src) {
        LocationInfo dest;
        dest.latitude = src.latitude;
        dest.longitude = src.longitude;
        dest.altitude = src.altitude;
        dest.accuracy = src.accuracy;
        dest.timestamp = src.timestamp;
        dest.speed = src.speed;
        dest.direction = src.direction;
        dest.source = src.source;
        dest.sourceId = src.sourceId;
        return dest;
    }

    // 防抖函数
    template<typename Func, typename... Args>
    class Debounce {
    private:
        Func func;
        std::chrono::milliseconds delay;
        bool isActive;

    public:
        Debounce(Func f, std::chrono::milliseconds d) : func(f), delay(d), isActive(false) {}

        void operator()(Args&&... args) {
            // 简单防抖实现，实际项目中可能需要更复杂的线程安全实现
            func(std::forward<Args>(args)...);
        }
    };

    // 节流函数
    template<typename Func, typename... Args>
    class Throttle {
    private:
        Func func;
        std::chrono::milliseconds interval;
        std::chrono::steady_clock::time_point lastCall;

    public:
        Throttle(Func f, std::chrono::milliseconds i) : func(f), interval(i), lastCall(std::chrono::steady_clock::now()) {}

        void operator()(Args&&... args) {
            auto now = std::chrono::steady_clock::now();
            auto duration = std::chrono::duration_cast<std::chrono::milliseconds>(now - lastCall);
            if (duration >= interval) {
                func(std::forward<Args>(args)...);
                lastCall = now;
            }
        }
    };

    // 字符串工具函数
    namespace string {
        // 字符串分割
        std::vector<std::string> split(const std::string& str, const std::string& delimiter);

        // 字符串替换
        std::string replace(const std::string& str, const std::string& from, const std::string& to);

        // 字符串转小写
        std::string toLowerCase(const std::string& str);

        // 字符串转大写
        std::string toUpperCase(const std::string& str);

        // 去除字符串前后空格
        std::string trim(const std::string& str);
    }

    // 时间工具函数
    namespace time {
        // 获取当前时间戳（毫秒）
        long long getCurrentTimestamp();

        // 格式化时间戳为字符串
        std::string formatTimestamp(long long timestamp, const std::string& format = "%Y-%m-%d %H:%M:%S");
    }

    // 数学工具函数
    namespace math {
        // 限制值在指定范围内
        template<typename T>
        T clamp(T value, T min, T max) {
            return std::max(min, std::min(max, value));
        }

        // 线性插值
        double lerp(double a, double b, double t);

        // 计算标准差
        double calculateStandardDeviation(const std::vector<double>& values);
    }
} // namespace utils

#endif // UTILS_H