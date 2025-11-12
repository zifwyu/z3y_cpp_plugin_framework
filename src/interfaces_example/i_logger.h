/**
 * @file i_logger.h
 * @brief 定义 z3y::example::ILogger 接口。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * 1.
 * 使用 Z3Y_DEFINE_INTERFACE
 * 宏 (
 * 版本 1.0)
 * 2.
 * 添加 <string>
 * 3. [修改]
 * 移入 z3y::example
 * 命名空间
 */

#pragma once

#ifndef Z3Y_INTERFACES_EXAMPLE_I_LOGGER_H_
#define Z3Y_INTERFACES_EXAMPLE_I_LOGGER_H_

#include "framework/i_component.h"
#include "framework/interface_helpers.h" // [新]
#include <string>  // [FIX]

namespace z3y {
    namespace example { // [修改]

        /**
         * @class ILogger
         * @brief 一个示例“服务”接口。
         */
        class ILogger : public virtual IComponent {
        public:
            /**
             * @brief [修改]
             * 使用 Z3Y_DEFINE_INTERFACE
             * 宏 (
             * 定义为 1.0
             * 版本)
             */
            Z3Y_DEFINE_INTERFACE(ILogger, "z3y-example-ILogger-IID-B1B542F8", \
                1, 0)

                /**
                 * @brief 记录一条消息。
                 * @param[in] message 要记录的字符串。
                 */
                virtual void Log(const std::string& message) = 0;
        };

    }  // namespace example
}  // namespace z3y

#endif  // Z3Y_INTERFACES_EXAMPLE_I_LOGGER_H_