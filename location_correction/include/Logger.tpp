// Logger.tpp - Logger类模板方法实现

#ifndef LOGGER_TPP
#define LOGGER_TPP

#include "Logger.h"
#include <cstdio>

// 模板方法实现：格式化日志
 template<typename... Args>
 void Logger::logFormat(LogLevel level, const std::string& tag, const char* format, Args... args) {
    if (level < config.level) {
        return;
    }

    // 使用snprintf计算所需的字符串长度
    int length = snprintf(nullptr, 0, format, args...);
    if (length <= 0) {
        return;
    }

    // 分配足够的空间
    std::string message(length + 1, '\0');
    snprintf(&message[0], message.size(), format, args...);

    // 调用普通log方法
    log(level, tag, message);
}

#endif // LOGGER_TPP