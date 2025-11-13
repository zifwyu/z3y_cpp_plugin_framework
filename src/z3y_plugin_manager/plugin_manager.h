/**
 * @file plugin_manager.h
 * @brief 定义 z3y::PluginManager 类，这是框架的核心。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 * 12. [修改] [!!]
 * API
 * 已恢复为
 * "
 * 方案 H
 * " (
 * 异常
 * )
 * 版本。
 * 13. [修改] [!!]
 * * * * -
 * GetService/CreateInstance
 * 返回 PluginPtr<T>
 * -
 * 失败时
 * throw z3y::PluginException
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
#include "framework/plugin_exceptions.h" // [!! 
                                         // 新增 !!]

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


        // --- [!! 
        // 最终 API (
        // 方案 H
        // ) !!] ---

        /**
         * @brief [API]
         * 通过字符串别名创建“普通组件”。
         *
         * @return
         * 一个有效的 PluginPtr<T>
         * 。
         * @throws z3y::PluginException
         * 如果创建失败
         * (
         * 包含详细错误码
         * )。
         */
        template <typename T>
        PluginPtr<T> CreateInstance(const std::string& alias);

        /**
         * @brief [API]
         * 通过 ClassId 创建“普通组件”。
         * @throws z3y::PluginException
         */
        template <typename T>
        PluginPtr<T> CreateInstance(const ClassId& clsid);

        /**
         * @brief [API]
         * 通过字符串别名获取“单例服务”。
         * @throws z3y::PluginException
         */
        template <typename T>
        PluginPtr<T> GetService(const std::string& alias);

        /**
         * @brief [API]
         * 通过 ClassId 获取“单例服务”。
         * @throws z3y::PluginException
         */
        template <typename T>
        PluginPtr<T> GetService(const ClassId& clsid);


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

    // --- [!! 
    // 方案 H 
    // API 
    // 实现 !!] ---
    // (
    // 模板实现必须放在头文件中
    // )

    template <typename T>
    PluginPtr<T> PluginManager::CreateInstance(const std::string& alias) {
        // 1. 
        // 查找别名
        ClassId clsid = GetClsidFromAlias(alias);
        if (clsid == 0) {
            // [!! 
            // 抛出 !!]
            throw PluginException(InstanceError::kErrorAliasNotFound,
                "Alias '" + alias + "' not found.");
        }
        // 2. 
        // 委托给 CLSID 
        // 版本
        return CreateInstance<T>(clsid);
    }

    template <typename T>
    PluginPtr<T> PluginManager::CreateInstance(const ClassId& clsid) {
        FactoryFunction factory;
        {
            std::lock_guard<std::mutex> lock(registry_mutex_);
            auto it = components_.find(clsid);

            // 1. 
            // 检查 CLSID 
            // 是否存在
            if (it == components_.end()) {
                // [!! 
                // 抛出 !!]
                throw PluginException(InstanceError::kErrorClsidNotFound);
            }
            // 2. 
            // 检查是否为普通组件
            if (it->second.is_singleton) {
                // [!! 
                // 抛出 !!]
                throw PluginException(InstanceError::kErrorNotAComponent,
                    "CLSID is a service, use GetService() instead.");
            }
            factory = it->second.factory;
        }

        // 3. 
        // 创建实例
        // (
        // 
        // 
        // )
        auto base_obj = factory();
        if (!base_obj) {
            // [!! 
            // 抛出 !!]
            throw PluginException(InstanceError::kErrorFactoryFailed);
        }

        // 4. [!! 
        //    核心 !!] 
        //    执行类型和版本检查
        InstanceError cast_result = InstanceError::kSuccess;
        PluginPtr<T> out_ptr = PluginCast<T>(base_obj, cast_result);

        // 5. 
        // 检查转换结果
        if (cast_result != InstanceError::kSuccess) {
            // [!! 
            // 抛出 !!] (
            // 
            // 
            // )
            throw PluginException(cast_result, "PluginCast failed.");
        }

        // 
        // 
        // 
        return out_ptr;
    }


    template <typename T>
    PluginPtr<T> PluginManager::GetService(const std::string& alias) {
        // 1. 
        // 查找别名
        ClassId clsid = GetClsidFromAlias(alias);
        if (clsid == 0) {
            // [!! 
            // 抛出 !!]
            throw PluginException(InstanceError::kErrorAliasNotFound,
                "Alias '" + alias + "' not found.");
        }
        // 2. 
        // 委托给 CLSID 
        // 版本
        return GetService<T>(clsid);
    }

    template <typename T>
    PluginPtr<T> PluginManager::GetService(const ClassId& clsid) {
        std::lock_guard<std::mutex> lock(registry_mutex_);
        InstanceError cast_result = InstanceError::kSuccess;

        auto it_factory = components_.find(clsid);
        // 1. 
        // 检查 CLSID
        if (it_factory == components_.end()) {
            // [!! 
            // 抛出 !!]
            throw PluginException(InstanceError::kErrorClsidNotFound);
        }
        // 2. 
        // 检查是否为服务
        if (!it_factory->second.is_singleton) {
            // [!! 
            // 抛出 !!]
            throw PluginException(InstanceError::kErrorNotAService,
                "CLSID is a component, use CreateInstance() instead.");
        }

        // 3. 
        // 检查单例缓存
        auto it_inst = singletons_.find(clsid);
        if (it_inst != singletons_.end()) {
            if (auto locked_ptr = it_inst->second.lock()) {
                // 
                // 
                // 
                // (
                // 
                // )
                PluginPtr<T> out_ptr = PluginCast<T>(locked_ptr, cast_result);
                if (cast_result != InstanceError::kSuccess) {
                    // [!! 
                    // 抛出 !!]
                    throw PluginException(cast_result, "PluginCast failed for cached service.");
                }
                return out_ptr;
            }
        }

        // 4. 
        // 缓存中没有，
        // 创建新实例
        auto base_obj = it_factory->second.factory();
        if (!base_obj) {
            // [!! 
            // 抛出 !!]
            throw PluginException(InstanceError::kErrorFactoryFailed);
        }

        // 5. [!! 
        //    核心 !!] 
        //    执行类型和版本检查
        PluginPtr<T> out_ptr = PluginCast<T>(base_obj, cast_result);

        // 6. 
        // 检查转换结果
        if (cast_result != InstanceError::kSuccess) {
            // [!! 
            // 抛出 !!]
            throw PluginException(cast_result, "PluginCast failed for new service.");
        }

        // 7. 
        // 转换成功，
        // 存入缓存并返回
        singletons_[clsid] = base_obj;
        return out_ptr;
    }


}  // namespace z3y

#endif  // Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_