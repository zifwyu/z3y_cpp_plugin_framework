/**
 * @file plugin_manager.h
 * @brief 定义 z3y::PluginManager 类，这是框架的核心。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ... (原有的 v2, v3, v4, v5, v2.1 修复日志) ...
 *
 * [v2.3 修复] (API 重构):
 * 1.
 * LoadPluginsFromDirectory
 * 增加 bool recursive
 * 参数。
 * 2.
 * 增加 LoadPlugin(file_path)
 * 接口，用于加载单个文件。
 * 3.
 * 增加私有的 LoadPluginInternal
 * 辅助函数，
 * 用于统一加载逻辑。
 */

#pragma once

#ifndef Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_
#define Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_

 // 包含所有框架接口
#include "framework/i_plugin_registry.h"
#include "framework/i_event_bus.h"
#include "framework/plugin_impl.h"
#include "framework/plugin_cast.h"
#include "framework/framework_events.h"

// 包含 C++ StdLib
#include <filesystem>
#include <string>
#include <vector>
#include <map>
#include <mutex>
#include <memory>
#include <thread>
#include <queue>
#include <condition_variable>
#include <set>
#include <typeindex> //

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
         * @brief [修改] 扫描指定目录并加载所有插件。
         *
         * @param[in] dir 要扫描的目录。
         * @param[in] recursive [新] true
         * 则递归扫描所有子目录, false
         * 则只扫描当前目录。
         * @param[in] init_func_name
         * 插件入口点函数的名称。
         */
        void LoadPluginsFromDirectory(
            const std::filesystem::path& dir,
            bool recursive = true,
            const std::string& init_func_name = "z3yPluginInit");

        /**
         * @brief [新] 加载一个指定的插件 DLL/SO 文件。
         *
         * @param[in] file_path 插件文件的完整路径。
         * @param[in] init_func_name
         * 插件入口点函数的名称。
         * @return true 加载和初始化成功, false 失败。
         */
        bool LoadPlugin(
            const std::filesystem::path& file_path,
            const std::string& init_func_name = "z3yPluginInit");

        void UnloadAllPlugins();

        /**
         * @brief [模板] 通过 ClassID (uint64_t) 创建一个“普通组件”的新实例。
         */
        template <typename T>
        PluginPtr<T> CreateInstance(const ClassID& clsid)
        {
            FactoryFunction factory;
            {
                std::lock_guard<std::mutex> lock(registry_mutex_);
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
            std::lock_guard<std::mutex> lock(registry_mutex_);

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
        void RegisterComponent(ClassID clsid,
            FactoryFunction factory,
            bool is_singleton,
            const std::string& alias) override;

        // --- IEventBus 接口实现 ---
        void Unsubscribe(std::shared_ptr<void> subscriber) override;

        /** @internal */
        void SubscribeGlobalImpl(EventID event_id,
            std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) override;
        /** @internal */
        void FireGlobalImpl(EventID event_id, PluginPtr<Event> e_ptr) override;

        /** @internal */
        void SubscribeToSenderImpl(void* sender_key,
            EventID event_id,
            std::weak_ptr<void> sub_id,
            std::weak_ptr<void> sender_id,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) override;

        /** @internal */
        void FireToSenderImpl(void* sender_key,
            EventID event_id,
            PluginPtr<Event> e_ptr) override;

    private:
        /**
         * @brief [新] 加载单个插件文件的内部核心逻辑。
         * @return true 成功, false 失败。
         */
        bool LoadPluginInternal(
            const std::filesystem::path& file_path,
            const std::string& init_func_name);

        /**
         * @brief [内部] 通过别名查找 ClassID。
         */
        ClassID GetClsidFromAlias(const std::string& alias);

        /**
         * @brief [内部] 事件循环工作线程的主函数。
         */
        void EventLoop();

        using LibHandle = void*;

        struct FactoryInfo
        {
            FactoryFunction factory;
            bool is_singleton;
        };

        struct Subscription
        {
            std::weak_ptr<void> subscriber_id_;
            std::weak_ptr<void> sender_id_;
            std::function<void(const Event&)> callback_;
            ConnectionType connection_type_;
        };

        /**
         * @brief [辅助函数] 清理已失效的(expired)订阅者 (weak_ptr)。
         */
        static void CleanupExpiredSubscriptions(
            std::vector<Subscription>& subs,
            bool check_sender_also,
            std::queue<std::weak_ptr<void>>& gc_queue
        );

        using EventCallbackList = std::vector<Subscription>;
        using EventMap = std::map<EventID, EventCallbackList>;
        using SenderMap = std::map<void*, EventMap>;
        using EventTask = std::function<void()>;

        using SubscriberLookupMapG = std::map<
            std::weak_ptr<void>,
            std::set<EventID>,
            std::owner_less<std::weak_ptr<void>>
        >;
        using SubscriberLookupMapS = std::map<
            std::weak_ptr<void>,
            std::set<std::pair<void*, EventID>>,
            std::owner_less<std::weak_ptr<void>>
        >;

        // --- 核心成员变量 (组件注册) ---
        std::mutex registry_mutex_;
        std::map<ClassID, FactoryInfo> factories_;
        std::map<ClassID, std::weak_ptr<IComponent>> singletons_;
        std::vector<LibHandle> loaded_libs_;
        std::map<std::string, ClassID> alias_map_;
        std::string current_loading_plugin_path_;

        // --- 事件总线成员 ---
        std::recursive_mutex event_mutex_;
        EventMap global_subscribers_;
        SenderMap sender_subscribers_;
        SubscriberLookupMapG global_sub_lookup_;
        SubscriberLookupMapS sender_sub_lookup_;

        // --- 异步事件总线成员 ---
        std::thread event_loop_thread_;
        std::queue<EventTask> event_queue_;
        std::mutex queue_mutex_; //!< 保护 event_queue_ 和 running_ 标志
        std::condition_variable queue_cv_;
        bool running_;

        std::queue<std::weak_ptr<void>> gc_queue_;
    };

} // namespace z3y

#endif // Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_