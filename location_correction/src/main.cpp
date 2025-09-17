#include "LocationService.h"
#include "Logger.h"
#include <iostream>
#include <thread>
#include <chrono>
#include <string>

using namespace location_correction;

// 位置更新监听器
void onLocationUpdated(const LocationInfo& location) {
    std::cout << "\n=== 位置更新 ===" << std::endl;
    std::cout << "时间戳: " << location.timestamp << std::endl;
    std::cout << "纬度: " << location.latitude << std::endl;
    std::cout << "经度: " << location.longitude << std::endl;
    std::cout << "精度: " << location.accuracy << " 米" << std::endl;
    std::cout << "海拔: " << location.altitude << " 米" << std::endl;
    std::cout << "速度: " << location.speed << " km/h" << std::endl;
    std::cout << "方向: " << location.direction << " 度" << std::endl;
    std::cout << "来源: ";
    switch (location.sourceType) {
        case LocationSource::GPS:
            std::cout << "GPS";
            break;
        case LocationSource::WIFI:
            std::cout << "WiFi";
            break;
        case LocationSource::BASE_STATION:
            std::cout << "基站";
            break;
        default:
            std::cout << "未知";
            break;
    }
    std::cout << "\n================\n" << std::endl;
}

// 显示帮助信息
void showHelp() {
    std::cout << "\n位置纠偏系统命令帮助:\n" << std::endl;
    std::cout << "  h 或 help      - 显示此帮助信息" << std::endl;
    std::cout << "  s 或 start     - 启动位置服务" << std::endl;
    std::cout << "  t 或 stop      - 停止位置服务" << std::endl;
    std::cout << "  g 或 get       - 获取当前位置" << std::endl;
    std::cout << "  l 或 list      - 列出最近的位置历史记录" << std::endl;
    std::cout << "  q 或 quit      - 退出程序" << std::endl;
    std::cout << "  mode [normal|high|low|fast|offline] - 设置纠偏模式" << std::endl;
    std::cout << "  log [debug|info|warning|error]      - 设置日志级别" << std::endl;
}

// 设置日志级别
void setLogLevel(const std::string& level) {
    if (level == "debug") {
        Logger::getInstance().setLogLevel(LogLevel::DEBUG);
    } else if (level == "info") {
        Logger::getInstance().setLogLevel(LogLevel::INFO);
    } else if (level == "warning") {
        Logger::getInstance().setLogLevel(LogLevel::WARNING);
    } else if (level == "error") {
        Logger::getInstance().setLogLevel(LogLevel::ERROR);
    } else {
        std::cout << "无效的日志级别。可用值: debug, info, warning, error" << std::endl;
    }
}

// 设置纠偏模式
void setCorrectionMode(std::shared_ptr<LocationService> service, const std::string& mode) {
    if (auto highPerfService = std::dynamic_pointer_cast<HighPerformanceLocationService>(service)) {
        if (auto corrector = std::dynamic_pointer_cast<MultiModeLocationCorrector>(highPerfService->getLocationCorrector())) {
            if (mode == "normal") {
                corrector->setCorrectionMode(CorrectionMode::NORMAL);
            } else if (mode == "high") {
                corrector->setCorrectionMode(CorrectionMode::HIGH_ACCURACY);
            } else if (mode == "low") {
                corrector->setCorrectionMode(CorrectionMode::LOW_POWER);
            } else if (mode == "fast") {
                corrector->setCorrectionMode(CorrectionMode::FAST_UPDATE);
            } else if (mode == "offline") {
                corrector->setCorrectionMode(CorrectionMode::OFFLINE);
            } else {
                std::cout << "无效的纠偏模式。可用值: normal, high, low, fast, offline" << std::endl;
            }
        } else {
            std::cout << "当前位置纠偏器不支持多模式切换" << std::endl;
        }
    } else {
        std::cout << "当前服务不支持模式切换" << std::endl;
    }
}

// 主函数
int main() {
    std::cout << "==============================================" << std::endl;
    std::cout << "           位置纠偏系统 (Location Correction)          " << std::endl;
    std::cout << "==============================================" << std::endl;
    
    // 初始化日志系统
    Logger::getInstance().setLogFile("location_correction.log");
    Logger::getInstance().setLogLevel(LogLevel::INFO);
    Logger::getInstance().info("位置纠偏系统启动");
    
    // 创建位置服务
    std::cout << "正在初始化位置服务..." << std::endl;
    auto serviceFactory = LocationServiceFactory::getInstance();
    auto locationService = serviceFactory.createLocationService(ServiceType::HIGH_PERFORMANCE);
    
    // 配置位置服务
    LocationServiceConfig config;
    config.enableGPS = true;
    config.enableWifi = true;
    config.enableBaseStation = true;
    config.enableHistoryStorage = true;
    config.maxQueueSize = 1000;
    config.cacheSize = 100;
    config.batchProcessingSize = 10;
    
    // 初始化位置服务
    if (!locationService->initialize(config)) {
        std::cerr << "位置服务初始化失败！" << std::endl;
        return 1;
    }
    
    // 设置位置更新监听器
    locationService->setLocationUpdateListener(onLocationUpdated);
    
    std::cout << "位置服务初始化成功！" << std::endl;
    std::cout << "输入 'help' 获取命令帮助。" << std::endl;
    
    // 命令行交互
    std::string command;
    while (true) {
        std::cout << "\n> 请输入命令: ";
        std::getline(std::cin, command);
        
        if (command == "h" || command == "help") {
            showHelp();
        } else if (command == "s" || command == "start") {
            if (locationService->start()) {
                std::cout << "位置服务已启动" << std::endl;
            } else {
                std::cout << "位置服务启动失败" << std::endl;
            }
        } else if (command == "t" || command == "stop") {
            if (locationService->stop()) {
                std::cout << "位置服务已停止" << std::endl;
            } else {
                std::cout << "位置服务停止失败" << std::endl;
            }
        } else if (command == "g" || command == "get") {
            auto location = locationService->getCurrentLocation();
            if (location) {
                std::cout << "\n=== 当前位置 ===" << std::endl;
                std::cout << "纬度: " << location->latitude << std::endl;
                std::cout << "经度: " << location->longitude << std::endl;
                std::cout << "精度: " << location->accuracy << " 米" << std::endl;
                std::cout << "来源: ";
                switch (location->sourceType) {
                    case LocationSource::GPS:
                        std::cout << "GPS";
                        break;
                    case LocationSource::WIFI:
                        std::cout << "WiFi";
                        break;
                    case LocationSource::BASE_STATION:
                        std::cout << "基站";
                        break;
                    default:
                        std::cout << "未知";
                        break;
                }
                std::cout << "\n================\n" << std::endl;
            } else {
                std::cout << "暂无位置数据" << std::endl;
            }
        } else if (command == "l" || command == "list") {
            int count = 5;
            std::cout << "请输入要显示的历史记录数量 (默认 5): ";
            std::string countStr;
            std::getline(std::cin, countStr);
            if (!countStr.empty()) {
                try {
                    count = std::stoi(countStr);
                } catch (...) {
                    std::cout << "无效的数量，使用默认值 5" << std::endl;
                }
            }
            
            auto history = locationService->getLocationHistory(count);
            if (!history.empty()) {
                std::cout << "\n=== 位置历史记录 ===" << std::endl;
                for (size_t i = 0; i < history.size(); ++i) {
                    const auto& loc = history[i];
                    std::cout << "[" << i+1 << "] 纬度: " << loc->latitude 
                              << ", 经度: " << loc->longitude 
                              << ", 精度: " << loc->accuracy << "m" << std::endl;
                }
                std::cout << "====================\n" << std::endl;
            } else {
                std::cout << "暂无历史记录" << std::endl;
            }
        } else if (command.substr(0, 5) == "mode ") {
            std::string mode = command.substr(5);
            setCorrectionMode(locationService, mode);
        } else if (command.substr(0, 4) == "log ") {
            std::string level = command.substr(4);
            setLogLevel(level);
        } else if (command == "q" || command == "quit") {
            std::cout << "正在退出程序..." << std::endl;
            locationService->stop();
            Logger::getInstance().info("位置纠偏系统退出");
            break;
        } else if (!command.empty()) {
            std::cout << "未知命令。输入 'help' 获取帮助。" << std::endl;
        }
    }
    
    std::cout << "程序已退出。" << std::endl;
    return 0;
}