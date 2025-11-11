/**
 * @file i_logger.h
 * @brief 定义一个示例性的单例服务接口 ILogger。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 这是一个纯粹的业务接口，它将被实现为一个“单例服务”。
 */

#ifndef Z3Y_SRC_INTERFACES_EXAMPLE_I_LOGGER_H_
#define Z3Y_SRC_INTERFACES_EXAMPLE_I_LOGGER_H_

#include "framework/i_component.h"
#include <string>

namespace z3y
{
    /**
     * @class ILogger
     * @brief 一个简单的单例日志服务接口。
     *
     * 继承自 z3y::IComponent 以便能被框架管理。
     */
    class ILogger : public IComponent
    {
    public:
        /**
         * @brief 虚析构函数。
         */
        virtual ~ILogger() = default;

        /**
         * @brief 一个示例性的业务方法，用于写入日志。
         * @param[in] message 要记录的消息。
         */
        virtual void Log(const std::string& message) = 0;
    };

} // namespace z3y

#endif // Z3Y_SRC_INTERFACES_EXAMPLE_I_LOGGER_H_