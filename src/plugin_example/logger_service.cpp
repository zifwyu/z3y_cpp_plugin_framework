/**
 * @file logger_service.cpp
 * @brief LoggerService 类的实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * 1. [修改]
 * 移入 z3y::example
 * 命名空间
 * 2. [FIX]
 * 包含 <iostream>
 * 3. [FIX]
 * 使用 lock_guard(mutex_)
 * 4. [修改] [!!]
 * 增加自动注册宏
 */

#include "logger_service.h"
#include "framework/z3y_plugin_sdk.h"
#include <iostream>
#include <mutex>

// [!! 
// 新增 !!] 
// 
// 
Z3Y_AUTO_REGISTER_SERVICE(z3y::example::LoggerService, "Logger.Default", true /* is_default */);


namespace z3y {
    namespace example { // [修改]

        // --- 构造函数 / 析构函数 ---

        LoggerService::LoggerService() {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cout << "  [LoggerService] Service Created (Constructor)."
                << std::endl;
        }

        LoggerService::~LoggerService() {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cout << "  [LoggerService] Service Destroyed (Destructor)."
                << std::endl;
        }

        // --- ILogger 接口实现 ---

        /**
         * @brief 记录一条消息 (线程安全)。
         * @param[in] message 要记录的字符串。
         */
        void LoggerService::Log(const std::string& message) {
            std::lock_guard<std::mutex> lock(mutex_);
            std::cout << "  [LoggerService] " << message << std::endl;
        }

    }  // namespace example
}  // namespace z3y