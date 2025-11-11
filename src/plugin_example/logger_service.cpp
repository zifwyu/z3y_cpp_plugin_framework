/**
 * @file logger_service.cpp
 * @brief z3y::ILogger 接口的实现类 LoggerService 的源文件。
 * @author 孙鹏宇
 * @date 2025-11-10
 */

#include "logger_service.h" // 包含它对应的头文件
#include <iostream>         // 用于 std::cout
#include <string>           // 用于 std::string
#include <mutex>            // 用于 std::lock_guard

namespace z3y
{

    /**
     * @brief 构造函数。
     *
     * 在 PluginManager 第一次调用 GetService() 时被调用一次。
     */
    LoggerService::LoggerService()
    {
        std::cout << "[plugin_example]: LoggerService instance created (构造函数)." << std::endl;
    }

    /**
     * @brief 析构函数。
     */
    LoggerService::~LoggerService()
    {
        std::cout << "[plugin_example]: LoggerService instance destroyed (析构函数)." << std::endl;
    }

    /**
     * @brief ILogger::Log 接口的实现。
     *
     * 这是单例服务的“业务逻辑”。
     * 我们使用 std::lock_guard 来确保来自不同线程的日志消息
     * 不会互相交错。
     */
    void LoggerService::Log(const std::string& message)
    {
        // 使用 lock_guard 自动管理互斥锁的锁定和解锁
        std::lock_guard<std::mutex> lock(mutex_);

        // 模拟写入日志
        std::cout << "[LoggerService]: " << message << std::endl;
    }

} // namespace z3y