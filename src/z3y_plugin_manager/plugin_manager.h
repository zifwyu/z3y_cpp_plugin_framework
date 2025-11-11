/**
 * @file plugin_manager.h
 * @brief 定义 z3y::PluginManager 类，这是框架的核心。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. IPluginRegistry::RegisterComponent 的签名已更新。
 * 2. 移除了 IPluginRegistry::RegisterAlias。
 */

#pragma once

#ifndef Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_
#define Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_

#include "framework/i_plugin_registry.h"
#include "framework/i_event_bus.h"
#include "framework/plugin_impl.h"
#include "framework/plugin_cast.h"
#include "framework/framework_events.h"

#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>

namespace z3y
{
    /**
     * @class PluginManager
     * @brief [框架核心] 插件管理器。
     */
    class PluginManager : public IPluginRegistry,
        public PluginImpl<PluginManager, clsid::kEventBus, IEventBus>
    {
    public:
        /**
         * @brief [工厂函数] 创建 PluginManager 的一个新实例。
         */
        static PluginPtr<PluginManager> Create();

        /**
         * @brief 析构函数。
         */
        virtual ~PluginManager();

        // --- 资源管理：禁止拷贝和移动 ---
        PluginManager(const PluginManager&) = delete;
        PluginManager& operator=(const PluginManager&) = delete;
        PluginManager(PluginManager&&) = delete;
        PluginManager& operator=(PluginManager&&) = delete;

        // --- 供宿主程序(Host)调用的公共 API ---

        /**
         * @brief 扫描指定目录(及其子目录)并加载所有插件。
         */
        void LoadPluginsFromDirectory(
            const std::filesystem::path& dir,
            const std::string& init_func_name = "z3yPluginInit");

        /**
         * @brief 卸载所有已加载的插件并清空所有注册表。
         */
        void UnloadAllPlugins();

        /**
         * @brief [模板] 通过 ClassID (uint64_t) 创建一个“普通组件”的新实例。
         */
        template <typename T>
        PluginPtr<T> CreateInstance(const ClassID& clsid)
        {
            // ... (实现保持不变) ...
            FactoryFunction factory;
            {
                std::lock_guard<std::mutex> lock(mutex_);
                auto it = factories_.find(clsid);
                if (it == factories_.end() || it->second.is_singleton)
                {
                    return nullptr;
                }
                factory = it->second.factory;
            }
            auto base_obj = factory();
            return PluginCast<T>(base_obj);
        }

        /**
         * @brief [模板] 通过 ClassID (uint64_t) 获取一个“单例服务”的唯一实例。
         */
        template <typename T>
        PluginPtr<T> GetService(const ClassID& clsid)
        {
            // ... (实现保持不变) ...
            std::lock_guard<std::mutex> lock(mutex_);
            auto it_factory = factories_.find(clsid);
            if (it_factory == factories_.end() || !it_factory->second.is_singleton)
            {
                return nullptr;
            }
            auto it_inst = singletons_.find(clsid);
            if (it_inst != singletons_.end())
            {
                if (auto locked_ptr = it_inst->second.lock())
                {
                    return PluginCast<T>(locked_ptr);
                }
            }
            auto base_obj = it_factory->second.factory();
            if (base_obj)
            {
                singletons_[clsid] = base_obj;
                return PluginCast<T>(base_obj);
            }
            return nullptr;
        }

        /**
         * @brief [模板] 通过字符串别名创建“普通组件”。
         */
        template <typename T>
        PluginPtr<T> CreateInstance(const std::string& alias)
        {
            return CreateInstance<T>(GetClsidFromAlias(alias));
        }

        /**
         * @brief [模板] 通过字符串别名获取“单例服务”。
         */
        template <typename T>
        PluginPtr<T> GetService(const std::string& alias)
        {
            return GetService<T>(GetClsidFromAlias(alias));
        }

    protected:
        /**
         * @brief 默认构造函数（受保护）。
         */
        PluginManager();

        // --- IPluginRegistry 接口实现 ---

        /**
         * @brief [IPluginRegistry 接口实现] 注册一个组件类。
         * [已修正]：签名已更新，包含了 alias。
         */
        void RegisterComponent(ClassID clsid,
            FactoryFunction factory,
            bool is_singleton,
            const std::string& alias) override;

        // [已移除] RegisterAlias 接口

        // --- IEventBus 接口实现 (声明保持不变) ---
        void Unsubscribe(std::shared_ptr<void> subscriber) override;
        void UnregisterSender(void* sender) override;
        void SubscribeGlobalImpl(std::type_index type,
            std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb) override;
        void FireGlobalImpl(std::type_index type, const Event& e) override;
        void SubscribeToSenderImpl(void* sender,
            std::type_index type,
            std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb) override;
        void FireToSenderImpl(void* sender,
            std::type_index type,
            const Event& e) override;

    private:
        /**
         * @brief [内部] 通过别名查找 ClassID。
         */
        ClassID GetClsidFromAlias(const std::string& alias);

        using LibHandle = void*;

        struct FactoryInfo
        {
            FactoryFunction factory;
            bool is_singleton;
        };

        struct Subscription
        {
            std::weak_ptr<void> subscriber_id;
            std::function<void(const Event&)> callback;
        };

        /**
         * @brief [辅助函数] 清理已失效的(expired)订阅者 (weak_ptr)。
         */
        static void CleanupExpiredSubscriptions(std::vector<Subscription>& subs);

        std::mutex mutex_;
        std::map<ClassID, FactoryInfo> factories_;
        std::map<ClassID, std::weak_ptr<IComponent>> singletons_;
        std::map<std::type_index, std::vector<Subscription>> global_subscribers_;
        std::map<void*, std::map<std::type_index, std::vector<Subscription>>> sender_subscribers_;
        std::vector<LibHandle> loaded_libs_;

        //! 别名表: 存储 "string" 到 ClassID 的映射
        std::map<std::string, ClassID> alias_map_;

        //! 用于在加载期间传递上下文给 RegisterComponent
        std::string current_loading_plugin_path_;
    };

} // namespace z3y

#endif // Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_