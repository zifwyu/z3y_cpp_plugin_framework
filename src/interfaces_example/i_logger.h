/**
 * @file i_logger.h
 * @brief 定义一个示例性的单例服务接口 ILogger。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 这是一个纯粹的业务接口，它将被实现为一个“单例服务”。
 *
 * [重构 v5.1 - Bug 修复]
 * 1. `ILogger`
 * 现在继承 `public virtual IComponent`。
 * 2.
 * 这是为了配合 `PluginImpl`
 * 解决“钻石继承”
 * 歧义 (C2594)。
 */

#ifndef Z3Y_SRC_INTERFACES_EXAMPLE_I_LOGGER_H_
#define Z3Y_SRC_INTERFACES_EXAMPLE_I_LOGGER_H_

#include "framework/i_component.h" //
#include <string>

namespace z3y
{
    /**
     * @class ILogger
     * @brief 一个简单的单例日志服务接口。
     *
     * @design
     * [Fix]
     * 必须使用 `public virtual IComponent`
     * 继承，
     * 以防止 `LoggerService`
     * 等实现类
     * 出现 `IComponent`
     * 的歧义。
     */
    class ILogger : public virtual IComponent // <-- [THE FIX] 
        //      添加 virtual
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