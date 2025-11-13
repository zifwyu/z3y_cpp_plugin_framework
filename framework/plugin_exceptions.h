/**
 * @file plugin_exceptions.h
 * @brief [新]
 * 定义 z3y::InstanceError
 * 枚举和 z3y::PluginException
 * 异常类。
 * @author (您的名字)
 * @date 2025-11-13
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_EXCEPTIONS_H_
#define Z3Y_FRAMEWORK_PLUGIN_EXCEPTIONS_H_

#include <stdexcept>
#include <string>
#include <map>

namespace z3y {

    /**
     * @enum InstanceError
     * @brief
     * GetService
     * 和 CreateInstance
     * 的详细错误码。
     */
    enum class InstanceError : uint32_t {
        /**
         * @brief
         * 成功
         * (
         * 内部使用，
         * 不会抛出
         * )。
         */
        kSuccess = 0,

        /**
         * @brief
         * 错误：
         * 提供的别名 (Alias)
         * 未在注册表中找到。
         */
        kErrorAliasNotFound = 1,

        /**
         * @brief
         * 错误：
         * 提供的 ClassId (CLSID)
         * 未在注册表中找到。
         */
        kErrorClsidNotFound = 2,

        /**
         * @brief
         * 错误：
         * GetService
         * 失败，
         * 因为该 CLSID
         * 注册为普通组件
         * (
         * 应使用 CreateInstance
         * )。
         */
        kErrorNotAService = 3,

        /**
         * @brief
         * 错误：
         * CreateInstance
         * 失败，
         * 因为该 CLSID
         * 注册为单例服务
         * (
         * 应使用 GetService
         * )。
         */
        kErrorNotAComponent = 4,

        /**
         * @brief
         * 错误：
         * 插件工厂函数 (Factory)
         * 执行失败
         * (
         * 例如 new
         * 失败返回
         * nullptr)
         * 。
         */
        kErrorFactoryFailed = 5,

        /**
         * @brief
         * 错误：
         * 类型转换 (PluginCast)
         * 失败。
         * 组件未实现所请求的接口 (IID
         * 不匹配
         * )。
         */
        kErrorInterfaceNotImpl = 6,

        /**
         * @brief
         * 错误：
         * 版本不兼容。
         * 插件的接口主版本 (Major)
         * 与宿主 (Host)
         * 请求的主版本不匹配。
         */
        kErrorVersionMajorMismatch = 7,

        /**
         * @brief
         * 错误：
         * 版本不兼容。
         * 插件的接口次版本 (Minor)
         * 低于宿主 (Host)
         * 请求的次版本。
         */
        kErrorVersionMinorTooLow = 8,

        /**
         * @brief
         * 错误：
         * 发生内部错误
         * (
         * 例如指针为空
         * )。
         */
        kErrorInternal = 9
    };

    /**
     * @brief
     * 将 InstanceError
     * 转换为可读字符串的辅助函数
     */
    inline std::string ResultToString(InstanceError error) {
        // 
        // 
        // 
        static const std::map<InstanceError, std::string> error_map = {
            {InstanceError::kSuccess, "kSuccess"},
            {InstanceError::kErrorAliasNotFound, "kErrorAliasNotFound (Alias not found)"},
            {InstanceError::kErrorClsidNotFound, "kErrorClsidNotFound (CLSID not found)"},
            {InstanceError::kErrorNotAService, "kErrorNotAService (Is a component, not a service)"},
            {InstanceError::kErrorNotAComponent, "kErrorNotAComponent (Is a service, not a component)"},
            {InstanceError::kErrorFactoryFailed, "kErrorFactoryFailed (Plugin factory failed)"},
            {InstanceError::kErrorInterfaceNotImpl, "kErrorInterfaceNotImpl (IID not implemented)"},
            {InstanceError::kErrorVersionMajorMismatch, "kErrorVersionMajorMismatch (Major version mismatch)"},
            {InstanceError::kErrorVersionMinorTooLow, "kErrorVersionMinorTooLow (Plugin version is too old)"},
            {InstanceError::kErrorInternal, "kErrorInternal"}
        };

        auto it = error_map.find(error);
        if (it != error_map.end()) {
            return it->second;
        }
        return "Unknown ErrorCode";
    }

    /**
     * @class PluginException
     * @brief
     * 框架在 GetService/CreateInstance
     * 失败时抛出的标准异常。
     */
    class PluginException : public std::exception {
    public:
        /**
         * @brief
         * 构造函数
         * @param[in] error
         * 详细的错误码
         * @param[in] message
         * 可选的上下文消息
         */
        PluginException(InstanceError error, const std::string& message = "")
            : error_(error), message_(message) {

            // 
            // 
            // 
            // 
            full_message_ = "[z3y::PluginException] ";
            if (!message_.empty()) {
                full_message_ += message_ + " (Reason: ";
            }
            full_message_ += ResultToString(error_);
            if (!message_.empty()) {
                full_message_ += ")";
            }
        }

        /**
         * @brief
         * 获取 std::exception
         * 标准错误消息
         */
        const char* what() const noexcept override {
            return full_message_.c_str();
        }

        /**
         * @brief
         * 获取详细的
         * InstanceError
         * 错误码
         */
        InstanceError GetError() const noexcept {
            return error_;
        }

    private:
        InstanceError error_;
        std::string message_;
        std::string full_message_;
    };

} // namespace z3y

#endif // Z3Y_FRAMEWORK_PLUGIN_EXCEPTIONS_H_