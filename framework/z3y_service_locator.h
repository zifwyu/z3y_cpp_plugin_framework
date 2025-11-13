/**
 * @file z3y_service_locator.h
 * @brief [新] 定义全局辅助函数，用于简化服务定位。
 * @details
 * 此文件由 z3y_framework.h 和 z3y_plugin_sdk.h 包含，
 * 为宿主和插件开发者提供统一、易用的全局函数
 * (如 GetDefaultService)，
 * 从而隐藏 PluginManager 的实例。
 * @author (您的名字)
 * @date 2025-11-13
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_SERVICE_LOCATOR_H_
#define Z3Y_FRAMEWORK_SERVICE_LOCATOR_H_

 // 关键依赖：
 // 1. 需要 PluginManager::GetActiveInstance() 来获取单例
#include "z3y_plugin_manager/plugin_manager.h" 
// 2. 需要 PluginException 和 InstanceError 来处理错误
#include "framework/plugin_exceptions.h"
// 3. 需要 IEventBus 和 clsid::kEventBus 来封装事件函数
#include "framework/i_event_bus.h" 

namespace z3y {

    /**
     * @brief [优化] 全局获取默认的单例服务。
     * @details
     * 自动获取 PluginManager 实例并调用 GetDefaultService<T>()。
     * @tparam T
     * 要获取的接口类型 (例如 ILogger)。
     * @return
     * 指向服务的 PluginPtr<T>。
     * @throws z3y::PluginException
     * 如果管理器未激活或未找到默认服务。
     */
    template <typename T>
    inline PluginPtr<T> GetDefaultService() {
        // 1. 内部自动获取管理器
        auto manager = PluginManager::GetActiveInstance();
        if (!manager) {
            // 抛出异常以匹配 manager 现有的 API 行为
            throw PluginException(InstanceError::kErrorInternal,
                "PluginManager is not active or has been destroyed.");
        }
        // 2. 转发调用
        return manager->GetDefaultService<T>();
    }

    /**
     * @brief [优化] 全局按别名获取单例服务。
     * @tparam T
     * 要获取的接口类型 (例如 ILogger)。
     * @param[in] alias
     * 注册的字符串别名 (例如 "Logger.Default")。
     * @return
     * 指向服务的 PluginPtr<T>。
     * @throws z3y::PluginException
     * 如果管理器未激活或未找到别名。
     */
    template <typename T>
    inline PluginPtr<T> GetService(const std::string& alias) {
        auto manager = PluginManager::GetActiveInstance();
        if (!manager) {
            throw PluginException(InstanceError::kErrorInternal, "PluginManager not active.");
        }
        return manager->GetService<T>(alias);
    }

    /**
     * @brief [!! 修复 !!] 全局按 ClassId 获取单例服务。
     * @tparam T
     * 要获取的接口类型。
     * @param[in] clsid
     * 注册的 ClassId。
     * @return
     * 指向服务的 PluginPtr<T>。
     * @throws z3y::PluginException
     * 如果管理器未激活或未找到 Clsid。
     */
    template <typename T>
    inline PluginPtr<T> GetService(const ClassId& clsid) {
        auto manager = PluginManager::GetActiveInstance();
        if (!manager) {
            throw PluginException(InstanceError::kErrorInternal, "PluginManager not active.");
        }
        return manager->GetService<T>(clsid);
    }

    /**
     * @brief [优化] 全局创建默认的普通组件实例。
     * @tparam T
     * 要创建的接口类型 (例如 ISimple)。
     * @return
     * 指向新实例的 PluginPtr<T>。
     * @throws z3y::PluginException
     * 如果管理器未激活或未找到默认实现。
     */
    template <typename T>
    inline PluginPtr<T> CreateDefaultInstance() {
        auto manager = PluginManager::GetActiveInstance();
        if (!manager) {
            throw PluginException(InstanceError::kErrorInternal, "PluginManager not active.");
        }
        return manager->CreateDefaultInstance<T>();
    }

    /**
     * @brief [优化] 全局按别名创建普通组件实例。
     * @tparam T
     * 要创建的接口类型 (例如 ISimple)。
     * @param[in] alias
     * 注册的字符串别名 (例如 "Simple.B")。
     * @return
     * 指向新实例的 PluginPtr<T>。
     * @throws z3y::PluginException
     * 如果管理器未激活或未找到别名。
     */
    template <typename T>
    inline PluginPtr<T> CreateInstance(const std::string& alias) {
        auto manager = PluginManager::GetActiveInstance();
        if (!manager) {
            throw PluginException(InstanceError::kErrorInternal, "PluginManager not active.");
        }
        return manager->CreateInstance<T>(alias);
    }

    /**
     * @brief [!! 修复 !!] 全局按 ClassId 创建普通组件实例。
     * @tparam T
     * 要创建的接口类型。
     * @param[in] clsid
     * 注册的 ClassId。
     * @return
     * 指向新实例的 PluginPtr<T>。
     * @throws z3y::PluginException
     * 如果管理器未激活或未找到 Clsid。
     */
    template <typename T>
    inline PluginPtr<T> CreateInstance(const ClassId& clsid) {
        auto manager = PluginManager::GetActiveInstance();
        if (!manager) {
            throw PluginException(InstanceError::kErrorInternal, "PluginManager not active.");
        }
        return manager->CreateInstance<T>(clsid);
    }

    // --- 事件总线的便捷封装 ---

    /**
     * @brief [优化] 全局发布一个广播事件。
     * @details
     * 自动获取 EventBus 服务并调用 FireGlobal。
     * 如果 EventBus
     * 尚未注册或管理器已销毁，
     * 此函数将静默失败 (
     * 不抛出异常
     * )。
     */
    template <typename TEvent, typename... Args>
    inline void FireGlobalEvent(Args&&... args) {
        auto manager = PluginManager::GetActiveInstance();
        if (!manager) {
            return; // 不抛异常，静默失败
        }

        try {
            // 内部获取 EventBus 服务
            auto bus = manager->GetService<IEventBus>(clsid::kEventBus);
            if (bus) {
                // 转发调用 (IEventBus 内部已有性能优化)
                bus->FireGlobal<TEvent>(std::forward<Args>(args)...);
            }
        }
        catch (const PluginException&) {
            // EventBus 尚未注册或已销毁，忽略
        }
    }

    /**
     * @brief [优化] 全局订阅一个广播事件。
     * @details
     * (注意: 订阅者 TSubscriber 仍需继承 std::enable_shared_from_this)
     */
    template <typename TEvent, typename TSubscriber, typename TCallback>
    inline void SubscribeGlobalEvent(std::shared_ptr<TSubscriber> subscriber,
        TCallback&& callback,
        ConnectionType type = ConnectionType::kDirect) {

        auto manager = PluginManager::GetActiveInstance();
        if (!manager) {
            return; // 静默失败
        }

        try {
            auto bus = manager->GetService<IEventBus>(clsid::kEventBus);
            if (bus) {
                bus->SubscribeGlobal<TEvent>(subscriber, std::forward<TCallback>(callback), type);
            }
        }
        catch (const PluginException&) {
            // 忽略
        }
    }

    /**
     * @brief [优化] 全局取消订阅者所有的订阅。
     * @details
     * (注意: 订阅者 TSubscriber 仍需继承 std::enable_shared_from_this)
     */
    template <typename TSubscriber>
    inline void Unsubscribe(std::shared_ptr<TSubscriber> subscriber) {
        auto manager = PluginManager::GetActiveInstance();
        if (!manager) {
            return; // 静默失败
        }

        try {
            auto bus = manager->GetService<IEventBus>(clsid::kEventBus);
            if (bus) {
                bus->Unsubscribe(subscriber);
            }
        }
        catch (const PluginException&) {
            // 忽略
        }
    }

} // namespace z3y

#endif // Z3Y_FRAMEWORK_SERVICE_LOCATOR_H_