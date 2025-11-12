/**
 * @file plugin_manager.h
 * @brief 定义 z3y::PluginManager 类，这是框架的核心。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 * [修改]
 * 9. [修改]
 * 核心注册机制更新为使用
 * InterfaceDetails
 * 10. [FIX] [!!]
 * 简化 PluginImpl
 * 继承 (
 * 移除 kClsid
 * 模板参数)
 * 11. [FIX] [!!]
 * Z3Y_DEFINE_COMPONENT_ID
 * 移回类 *内部*
 */

#pragma once

#ifndef Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_
#define Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_

 // 包含所有框架接口
#include "framework/i_plugin_registry.h"
#include "framework/i_event_bus.h"
#include "framework/i_plugin_query.h"
#include "framework/plugin_impl.h"
#include "framework/plugin_cast.h"
#include "framework/framework_events.h"
#include "framework/connection_type.h"

// 包含 C++ StdLib
#include <condition_variable>
#include <filesystem>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <queue>
#include <set>
#include <string>
#include <thread>
#include <typeindex>
#include <vector>

// [新] 引入辅助宏
#include "framework/component_helpers.h" 

namespace z3y {

    namespace clsid {
        /**
         * @brief [修改]
         * PluginManager
         * 自己的 "实现ID"。
         * (不再需要，
         * 已移入类定义内部)
         */
         // constexpr ClassId kPluginManager =
         //     ConstexprHash("z3y-core-plugin-manager-IMPL-UUID");
    }  // namespace clsid

    /**
     * @class PluginManager
     * @brief [框架核心] 插件管理器。
     * [修改] 继承 IPluginQuery
     * [修改] 更新 PluginImpl 模板参数以包含
     * IPluginQuery
     */
    class PluginManager
        : public IPluginRegistry,
        // public IPluginQuery,  // [FIX] 移除此行
        public PluginImpl<PluginManager, // [修改] 
        // 移除 kClsid 
        // 参数
        IEventBus,
        IPluginQuery>
    {
    public:
        /**
         * @brief [修改]
         * Z3Y_DEFINE_COMPONENT_ID
         * 已移回类 *内部*
         */
        Z3Y_DEFINE_COMPONENT_ID("z3y-core-plugin-manager-IMPL-UUID")

    public:
        /**
         * @brief [工厂函数] 创建 PluginManager 的一个新实例。
         */
        static PluginPtr<PluginManager> Create();

        /**
         * @brief 析构函数。
         * (已修改)
         */
        virtual ~PluginManager();

        // --- 资源管理：禁止拷贝和移动 ---
        PluginManager(const PluginManager&) = delete;
        PluginManager& operator=(const PluginManager&) = delete;
        PluginManager(PluginManager&&) = delete;
        PluginManager& operator=(PluginManager&&) = delete;

        // --- 供宿主程序(Host)调用的公共 API ---

        /**
         * @brief [FIX]
         * Shutdown()
         * 接口已被移除。
         * (已完成)
         */
         // void Shutdown();

         /**
          * @brief [修改] 扫描指定目录并加载所有插件。
          */
        void LoadPluginsFromDirectory(
            const std::filesystem::path& dir, bool recursive = true,
            const std::string& init_func_name = "z3yPluginInit");

        /**
         * @brief [新] 加载一个指定的插件 DLL/SO 文件。
         */
        bool LoadPlugin(const std::filesystem::path& file_path,
            const std::string& init_func_name = "z3yPluginInit");

        /**
         * @brief 卸载所有已加载的插件。
         */
        void UnloadAllPlugins();

        /**
         * @brief [模板] 通过 ClassId (uint64_t) 创建一个“普通组件”的新实例。
         */
        template <typename T>
        PluginPtr<T> CreateInstance(const ClassId& clsid) {
            FactoryFunction factory;
            {
                std::lock_guard<std::mutex> lock(registry_mutex_);
                auto it = components_.find(clsid);
                if (it == components_.end() || it->second.is_singleton) {
                    return nullptr;
                }
                factory = it->second.factory;
            }
            auto base_obj = factory();
            return PluginCast<T>(base_obj);
        }

        /**
         * @brief [模板] 通过 ClassId (uint64_t) 获取一个“单例服务”的唯一实例。
         */
        template <typename T>
        PluginPtr<T> GetService(const ClassId& clsid) {
            std::lock_guard<std::mutex> lock(registry_mutex_);

            auto it_factory = components_.find(clsid);
            if (it_factory == components_.end() ||
                !it_factory->second.is_singleton) {
                return nullptr;
            }

            auto it_inst = singletons_.find(clsid);
            if (it_inst != singletons_.end()) {
                if (auto locked_ptr = it_inst->second.lock()) {
                    return PluginCast<T>(locked_ptr);
                }
            }

            auto base_obj = it_factory->second.factory();
            if (base_obj) {
                singletons_[clsid] = base_obj;
                return PluginCast<T>(base_obj);
            }
            return nullptr;
        }

        /**
         * @brief [模板] 通过字符串别名创建“普通组件”。
         */
        template <typename T>
        PluginPtr<T> CreateInstance(const std::string& alias) {
            return CreateInstance<T>(GetClsidFromAlias(alias));
        }

        /**
         * @brief [模板] 通过字符串别名获取“单例服务”。
         */
        template <typename T>
        PluginPtr<T> GetService(const std::string& alias) {
            return GetService<T>(GetClsidFromAlias(alias));
        }

    protected:
        /**
         * @brief 默认构造函数（受保护）。
         */
        PluginManager();

        // --- IPluginRegistry 接口实现 ---
        /**
         * @brief [修改]
         * 更新签名为
         * vector<InterfaceDetails>
         */
        void RegisterComponent(ClassId clsid, FactoryFunction factory,
            bool is_singleton, const std::string& alias,
            std::vector<InterfaceDetails> implemented_interfaces) override;

        // --- IEventBus 接口实现 ---
        void Unsubscribe(std::shared_ptr<void> subscriber) override;

        /** @internal */
        void SubscribeGlobalImpl(EventId event_id, std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) override;
        /** @internal */
        void FireGlobalImpl(EventId event_id, PluginPtr<Event> e_ptr) override;

        /** @internal */
        void SubscribeToSenderImpl(void* sender_key, EventId event_id,
            std::weak_ptr<void> sub_id,
            std::weak_ptr<void> sender_id,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) override;

        /** @internal */
        void FireToSenderImpl(void* sender_key, EventId event_id,
            PluginPtr<Event> e_ptr) override;

        // --- IPluginQuery 接口实现 ---
        std::vector<ComponentDetails> GetAllComponents() override;
        bool GetComponentDetails(ClassId clsid,
            ComponentDetails& out_details) override;
        bool GetComponentDetailsByAlias(const std::string& alias,
            ComponentDetails& out_details) override; // (已完成)
        std::vector<ComponentDetails> FindComponentsImplementing(
            InterfaceId iid) override;
        std::vector<std::string> GetLoadedPluginFiles() override;
        std::vector<ComponentDetails> GetComponentsFromPlugin(
            const std::string& plugin_path) override;

    private:
        /**
         * @brief [新] 加载单个插件文件的内部核心逻辑。
         */
        bool LoadPluginInternal(const std::filesystem::path& file_path,
            const std::string& init_func_name);

        /**
         * @brief [内部] 通过别名查找 ClassId。
         */
        ClassId GetClsidFromAlias(const std::string& alias);

        /**
         * @brief [内部] 事件循环工作线程的主函数。
         */
        void EventLoop();

        using LibHandle = void*;

        /**
         * @struct ComponentInfo
         * @brief [修改]
         * 存储所有组件元数据
         */
        struct ComponentInfo {
            FactoryFunction factory;
            bool is_singleton;
            std::string alias;
            std::string source_plugin_path;
            /**
             * @brief [修改]
             * 存储 InterfaceDetails
             * 列表
             */
            std::vector<InterfaceDetails> implemented_interfaces;
        };

        /**
         * @struct Subscription
         */
        struct Subscription {
            std::weak_ptr<void> subscriber_id;
            std::weak_ptr<void> sender_id;
            std::function<void(const Event&)> callback;
            ConnectionType connection_type;
        };

        /**
         * @brief [辅助函数] 清理已失效的(expired)订阅者 (weak_ptr)。
         */
        static void CleanupExpiredSubscriptions(
            std::vector<Subscription>& subs, bool check_sender_also,
            std::queue<std::weak_ptr<void>>& gc_queue);

        using EventCallbackList = std::vector<Subscription>;
        using EventMap = std::map<EventId, EventCallbackList>;
        using SenderMap = std::map<void*, EventMap>;
        using EventTask = std::function<void()>;

        using SubscriberLookupMapG =
            std::map<std::weak_ptr<void>, std::set<EventId>,
            std::owner_less<std::weak_ptr<void>>>;
        using SubscriberLookupMapS =
            std::map<std::weak_ptr<void>, std::set<std::pair<void*, EventId>>,
            std::owner_less<std::weak_ptr<void>>>;

        // --- 核心成员变量 (组件注册) ---
        std::mutex registry_mutex_;
        std::map<ClassId, ComponentInfo> components_;  // [修改]
        std::map<ClassId, std::weak_ptr<IComponent>> singletons_;
        std::map<std::string, LibHandle> loaded_libs_;
        std::map<std::string, ClassId> alias_map_;
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
        std::mutex queue_mutex_;
        std::condition_variable queue_cv_;
        bool running_;

        std::queue<std::weak_ptr<void>> gc_queue_;
    };

}  // namespace z3y

#endif  // Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_