// Logger.cpp - 日志工具实现

#include "Logger.h"
#include "Logger.tpp"
#include <iostream>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <chrono>
#include <thread>

// 静态成员初始化
std::shared_ptr<Logger> Logger::instance = nullptr;
std::mutex Logger::instanceMutex;

// Logger构造函数
Logger::Logger() : config(LoggerConfig()), 
                   fileStream(nullptr), 
                   running(true), 
                   logThread(nullptr), 
                   logQueue() {
    // 默认配置
    config.logLevel = LogLevel::INFO;
    config.enableConsoleOutput = true;
    config.enableFileOutput = false;
    config.logFile = "application.log";
    config.maxLogFileSize = 10 * 1024 * 1024; // 10MB
    config.maxBackupFiles = 5;
    config.consoleLogFormat = "[%TIME%] [%LEVEL%] %MESSAGE%";
    config.fileLogFormat = "[%TIME%] [%LEVEL%] [%THREAD%] %MESSAGE%";
    config.dateFormat = "%Y-%m-%d %H:%M:%S.%MS";
    
    // 启动日志线程
    startLogThread();
}

// Logger析构函数
Logger::~Logger() {
    // 停止日志线程
    stopLogThread();
    
    // 关闭文件流
    if (fileStream != nullptr && fileStream->is_open()) {
        fileStream->close();
        delete fileStream;
        fileStream = nullptr;
    }
}

// 获取Logger单例实例
std::shared_ptr<Logger> Logger::getInstance() {
    std::lock_guard<std::mutex> lock(instanceMutex);
    if (instance == nullptr) {
        instance = std::shared_ptr<Logger>(new Logger());
    }
    return instance;
}

// 设置日志配置
void Logger::setConfig(const LoggerConfig& newConfig) {
    std::lock_guard<std::mutex> lock(configMutex);
    config = newConfig;
    
    // 如果启用了文件输出，初始化文件流
    if (config.enableFileOutput && fileStream == nullptr) {
        initializeFileStream();
    } else if (!config.enableFileOutput && fileStream != nullptr) {
        // 如果禁用了文件输出，关闭文件流
        fileStream->close();
        delete fileStream;
        fileStream = nullptr;
    }
}

// 获取当前日志配置
LoggerConfig Logger::getConfig() const {
    std::lock_guard<std::mutex> lock(configMutex);
    return config;
}

// 初始化文件流
bool Logger::initializeFileStream() {
    std::lock_guard<std::mutex> lock(fileMutex);
    
    try {
        // 检查文件大小，如果超过最大大小，进行日志轮转
        checkLogFileSize();
        
        // 打开文件流（追加模式）
        fileStream = new std::ofstream(config.logFile, std::ios::out | std::ios::app);
        
        if (!fileStream->is_open()) {
            std::cerr << "Failed to open log file: " << config.logFile << std::endl;
            delete fileStream;
            fileStream = nullptr;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception when initializing log file: " << e.what() << std::endl;
        if (fileStream != nullptr) {
            delete fileStream;
            fileStream = nullptr;
        }
        return false;
    }
}

// 检查日志文件大小
void Logger::checkLogFileSize() {
    std::ifstream file(config.logFile, std::ios::ate | std::ios::binary);
    if (file.is_open()) {
        std::streamsize size = file.tellg();
        file.close();
        
        // 如果文件大小超过最大限制，进行日志轮转
        if (size > config.maxLogFileSize) {
            rotateLogFiles();
        }
    }
}

// 轮转日志文件
void Logger::rotateLogFiles() {
    // 删除最旧的备份文件
    for (int i = config.maxBackupFiles; i > 1; --i) {
        std::string oldFile = config.logFile + "." + std::to_string(i-1);
        std::string newFile = config.logFile + "." + std::to_string(i);
        
        // 如果文件存在，则重命名
        if (std::ifstream(oldFile)) {
            std::rename(oldFile.c_str(), newFile.c_str());
        }
    }
    
    // 将当前日志文件重命名为.log.1
    if (std::ifstream(config.logFile)) {
        std::string backupFile = config.logFile + ".1";
        std::rename(config.logFile.c_str(), backupFile.c_str());
    }
}

// 启动日志线程
void Logger::startLogThread() {
    running = true;
    logThread = new std::thread(&Logger::processLogQueue, this);
}

// 停止日志线程
void Logger::stopLogThread() {
    running = false;
    logCV.notify_one();
    
    if (logThread != nullptr && logThread->joinable()) {
        logThread->join();
        delete logThread;
        logThread = nullptr;
    }
}

// 处理日志队列
void Logger::processLogQueue() {
    while (running) {
        std::unique_lock<std::mutex> lock(queueMutex);
        
        // 等待直到队列非空或需要停止
        logCV.wait(lock, [this] { return !logQueue.empty() || !running; });
        
        // 处理队列中的所有日志消息
        while (!logQueue.empty()) {
            LogMessage msg = logQueue.front();
            logQueue.pop();
            lock.unlock();
            
            // 输出日志
            outputLog(msg);
            
            lock.lock();
        }
    }
}

// 输出日志
void Logger::outputLog(const LogMessage& msg) {
    // 获取当前配置（避免在日志输出过程中配置被修改）
    LoggerConfig currentConfig = getConfig();
    
    // 格式化日志消息
    std::string formattedMessage = formatLogMessage(msg, currentConfig);
    
    // 控制台输出
    if (currentConfig.enableConsoleOutput) {
        std::lock_guard<std::mutex> consoleLock(consoleMutex);
        std::cout << formattedMessage << std::endl;
    }
    
    // 文件输出
    if (currentConfig.enableFileOutput && fileStream != nullptr) {
        std::lock_guard<std::mutex> fileLock(fileMutex);
        if (fileStream->is_open()) {
            *fileStream << formattedMessage << std::endl;
            fileStream->flush();
        }
    }
}

// 格式化日志消息
std::string Logger::formatLogMessage(const LogMessage& msg, const LoggerConfig& config) {
    std::string format = config.enableFileOutput ? config.fileLogFormat : config.consoleLogFormat;
    std::string result = format;
    
    // 替换时间戳
    size_t timePos = result.find("%TIME%");
    if (timePos != std::string::npos) {
        result.replace(timePos, 6, getFormattedTime(msg.timestamp, config.dateFormat));
    }
    
    // 替换日志级别
    size_t levelPos = result.find("%LEVEL%");
    if (levelPos != std::string::npos) {
        result.replace(levelPos, 6, getLogLevelString(msg.level));
    }
    
    // 替换线程ID
    size_t threadPos = result.find("%THREAD%");
    if (threadPos != std::string::npos) {
        result.replace(threadPos, 8, std::to_string(msg.threadId));
    }
    
    // 替换消息内容
    size_t messagePos = result.find("%MESSAGE%");
    if (messagePos != std::string::npos) {
        result.replace(messagePos, 9, msg.message);
    }
    
    return result;
}

// 获取格式化的时间
std::string Logger::getFormattedTime(long long timestampMs, const std::string& format) {
    // 将毫秒时间戳转换为time_t
    std::time_t timestamp = timestampMs / 1000;
    int milliseconds = timestampMs % 1000;
    
    // 格式化时间
    std::tm localTime;
#ifdef _WIN32
    localtime_s(&localTime, &timestamp);
#else
    localtime_r(&timestamp, &localTime);
#endif
    
    std::stringstream ss;
    
    // 处理格式化字符串
    for (size_t i = 0; i < format.size(); ++i) {
        if (format[i] == '%' && i + 1 < format.size()) {
            i++;
            switch (format[i]) {
                case 'Y': ss << std::setw(4) << std::setfill('0') << (localTime.tm_year + 1900); break;
                case 'm': ss << std::setw(2) << std::setfill('0') << (localTime.tm_mon + 1); break;
                case 'd': ss << std::setw(2) << std::setfill('0') << localTime.tm_mday; break;
                case 'H': ss << std::setw(2) << std::setfill('0') << localTime.tm_hour; break;
                case 'M': ss << std::setw(2) << std::setfill('0') << localTime.tm_min; break;
                case 'S': ss << std::setw(2) << std::setfill('0') << localTime.tm_sec; break;
                case 'f': case 'MS': ss << std::setw(3) << std::setfill('0') << milliseconds; break;
                default: ss << format[i]; break;
            }
        } else {
            ss << format[i];
        }
    }
    
    return ss.str();
}

// 获取日志级别字符串
std::string Logger::getLogLevelString(LogLevel level) {
    switch (level) {
        case LogLevel::DEBUG: return "DEBUG";
        case LogLevel::INFO: return "INFO";
        case LogLevel::WARNING: return "WARNING";
        case LogLevel::ERROR: return "ERROR";
        case LogLevel::FATAL: return "FATAL";
        default: return "UNKNOWN";
    }
}

// 从字符串获取日志级别
LogLevel Logger::getLogLevelFromString(const std::string& levelStr) {
    std::string upperStr = levelStr;
    // 转换为大写
    for (char& c : upperStr) {
        c = std::toupper(c);
    }
    
    if (upperStr == "DEBUG") return LogLevel::DEBUG;
    if (upperStr == "INFO") return LogLevel::INFO;
    if (upperStr == "WARNING" || upperStr == "WARN") return LogLevel::WARNING;
    if (upperStr == "ERROR") return LogLevel::ERROR;
    if (upperStr == "FATAL" || upperStr == "CRITICAL") return LogLevel::FATAL;
    
    // 默认返回INFO级别
    return LogLevel::INFO;
}

// 检查日志级别是否需要输出
bool Logger::shouldLog(LogLevel level) const {
    std::lock_guard<std::mutex> lock(configMutex);
    return level >= config.logLevel;
}

// 刷新日志
void Logger::flush() {
    std::lock_guard<std::mutex> fileLock(fileMutex);
    if (fileStream != nullptr && fileStream->is_open()) {
        fileStream->flush();
    }
}

// 清空日志文件（仅适用于文件日志）
bool Logger::clearLogFile() {
    std::lock_guard<std::mutex> fileLock(fileMutex);
    
    try {
        // 关闭当前文件流
        if (fileStream != nullptr && fileStream->is_open()) {
            fileStream->close();
        }
        
        // 重新创建文件（覆盖模式）
        fileStream = new std::ofstream(config.logFile, std::ios::out | std::ios::trunc);
        
        if (!fileStream->is_open()) {
            delete fileStream;
            fileStream = nullptr;
            return false;
        }
        
        return true;
    } catch (const std::exception& e) {
        std::cerr << "Exception when clearing log file: " << e.what() << std::endl;
        if (fileStream != nullptr) {
            delete fileStream;
            fileStream = nullptr;
        }
        return false;
    }
}