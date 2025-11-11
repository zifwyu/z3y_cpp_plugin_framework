/**
 * @file logger_service.h
 * @brief z3y::ILogger 接口的实现类 LoggerService 的头文件。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 演示了如何实现一个“单例服务”组件。
 * ClassID (kCLoggerService) 在此头文件中定义。
 */

#ifndef Z3Y_SRC_PLUGIN_EXAMPLE_LOGGER_SERVICE_H_
#define Z3Y_SRC_PLUGIN_EXAMPLE_LOGGER_SERVICE_H_

#include "framework/plugin_impl.h"
#include "interfaces_example/i_logger.h" // 依赖 ILogger 接口
#include <mutex>                         // 包含 mutex 以实现线程安全的日志记录

namespace z3y
{
    /**
     * @brief LoggerService 实现类的 ClassID。
     */
    namespace clsid
    {
        constexpr ClassID kCLoggerService =
            ConstexprHash("z3y-example-cloggerservice-uuid-8C49F3D0-4034-4166-85D4-38E70E83D59D");
    } // namespace clsid

    /**
     * @class LoggerService
     * @brief z3y::ILogger 接口的一个具体实现，将被注册为单例。
     */
    class LoggerService : public z3y::PluginImpl<LoggerService,
        clsid::kCLoggerService,
        ILogger>
    {
    public:
        /**
         * @brief 构造函数。
         */
        LoggerService();

        /**
         * @brief 析构函数。
         */
        virtual ~LoggerService();

        // --- ILogger 接口的实现 ---

        /**
         * @brief ILogger::Log 接口的实现。
         * @param[in] message 要记录的消息。
         */
        void Log(const std::string& message) override;

    private:
        //! 用于保护 std::cout 的互斥锁，确保日志输出不会交叉
        mutable std::mutex mutex_;
    };

} // namespace z3y

#endif // Z3Y_SRC_PLUGIN_EXAMPLE_LOGGER_SERVICE_H_