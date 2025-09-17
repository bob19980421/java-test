// Logger.h - 日志工具类

#ifndef LOGGER_H
#define LOGGER_H

#include <string>
#include <fstream>
#include <mutex>
#include <iostream>
#include <sstream>

// 日志级别枚举
enum class LogLevel {
    DEBUG,   // 调试信息
    INFO,    // 一般信息
    WARNING, // 警告信息
    ERROR,   // 错误信息
    FATAL    // 致命错误
};

// 日志配置结构体
struct LoggerConfig {
    LogLevel level;              // 日志级别
    std::string logFilePath;     // 日志文件路径
    bool enableConsoleOutput;    // 是否启用控制台输出
    bool enableFileOutput;       // 是否启用文件输出
    size_t maxFileSize;          // 最大文件大小（字节）

    // 构造函数
    LoggerConfig() : 
        level(LogLevel::INFO),
        logFilePath("location_correction.log"),
        enableConsoleOutput(true),
        enableFileOutput(false),
        maxFileSize(10 * 1024 * 1024) // 默认10MB
    {}
};

// 日志工具类
class Logger {
private:
    static Logger* instance;          // 单例实例
    static std::mutex instanceMutex;  // 单例互斥锁
    LoggerConfig config;              // 日志配置
    std::ofstream logFile;            // 日志文件流
    std::mutex logMutex;              // 日志互斥锁

    // 私有构造函数
    Logger();
    // 析构函数
    ~Logger();

    // 检查并切换日志文件
    void checkAndRotateLogFile();

    // 将日志级别转换为字符串
    std::string levelToString(LogLevel level);

public:
    // 获取单例实例
    static Logger* getInstance();

    // 设置日志配置
    void setConfig(const LoggerConfig& newConfig);

    // 获取当前日志配置
    LoggerConfig getConfig() const;

    // 记录日志
    void log(LogLevel level, const std::string& tag, const std::string& message);

    // 记录调试日志
    void debug(const std::string& tag, const std::string& message);

    // 记录信息日志
    void info(const std::string& tag, const std::string& message);

    // 记录警告日志
    void warning(const std::string& tag, const std::string& message);

    // 记录错误日志
    void error(const std::string& tag, const std::string& message);

    // 记录致命错误日志
    void fatal(const std::string& tag, const std::string& message);

    // 格式化日志（支持可变参数）
    template<typename... Args>
    void logFormat(LogLevel level, const std::string& tag, const char* format, Args... args);
};

// 方便使用的日志宏定义
#define LOG_DEBUG(tag, msg) Logger::getInstance()->debug(tag, msg)
#define LOG_INFO(tag, msg) Logger::getInstance()->info(tag, msg)
#define LOG_WARNING(tag, msg) Logger::getInstance()->warning(tag, msg)
#define LOG_ERROR(tag, msg) Logger::getInstance()->error(tag, msg)
#define LOG_FATAL(tag, msg) Logger::getInstance()->fatal(tag, msg)

// 模板方法实现
#include "Logger.tpp"

#endif // LOGGER_H