/**
 * @file logger_service.h
 * @brief 定义 z3y::example::LoggerService (单例)
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * ...
 * 3. [修改] [!!]
 * Z3Y_DEFINE_COMPONENT_ID
 * 宏已移回类 *内部*。
 * 4. [修改] [!!]
 * 简化 PluginImpl
 * 继承 (
 * 移除 kClsid
 * 模板参数)
 */

#pragma once

#ifndef Z3Y_PLUGIN_EXAMPLE_LOGGER_SERVICE_H_
#define Z3Y_PLUGIN_EXAMPLE_LOGGER_SERVICE_H_

#include "interfaces_example/i_logger.h"  // 依赖 ILogger
#include "framework/z3y_plugin_sdk.h"
#include <mutex>

namespace z3y {
    namespace example { // [修改]

        // --- 1. [修改] ClassId 
        // 定义已移入类内部 ---

        /**
         * @class LoggerService
         * @brief ILogger 接口的单例实现。
         */
        class LoggerService
            : public PluginImpl<LoggerService,
            ILogger> // [修改] 
            // 移除了 kClsid 
            // 模板参数
        {
        public:
            /**
             * @brief [新]
             * 使用 Z3Y_DEFINE_COMPONENT_ID
             * 定义 kClsid
             */
            Z3Y_DEFINE_COMPONENT_ID("z3y-example-CLoggerService-UUID-C50A10B4")

        public:
            LoggerService();
            virtual ~LoggerService();

            // --- ILogger 接口实现 ---
            void Log(const std::string& message) override;

        private:
            std::mutex mutex_;
        };

    }  // namespace example
}  // namespace z3y

#endif  // Z3Y_PLUGIN_EXAMPLE_LOGGER_SERVICE_H_