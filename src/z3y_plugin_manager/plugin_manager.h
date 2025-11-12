/**
 * @file plugin_manager.h
 * @brief 定义 z3y::PluginManager 类，这是框架的核心。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [重构 v2 - Fix 1] (死锁修复):
 * 1. event_mutex_ 已被替换为 std::recursive_mutex，
 * 以防止 kDirect 事件回调重入。
 *
 * [重构 v2 - Fix 4] (性能优化):
 * 1. 增加了 global_sub_lookup_ 和 sender_sub_lookup_
 * 两个反向查找表，用于优化 Unsubscribe()。
 *
 * [重构 v3 - Fix 5] (泄漏修复 + 易用性):
 * 1. 移除了 v2 的 RAII 句柄方案 (易用性差)。
 * 2. 恢复使用 weak_ptr 自动管理生命周期。
 * 3. 增加了 gc_queue_ (异步垃圾回收队列)。
 * 4. CleanupExpiredSubscriptions 会将失效的 weak_ptr
 * 推入 gc_queue_。
 * 5. EventLoop 线程会异步消耗此队列，
 * 并安全地清理反向查找表，
 * 从而在不牺牲性能和易用性的前提下解决内存泄漏。
 *
 * [v2.1 修复]:
 * 1. IEventBus
 * 的 ...Impl
 * override 签名
 * 已更新为使用 EventID。
 *
 * 2.
 * EventMap, SenderMap,
 * 和反向查找表的键
 * 已从 ClassID
 * 替换为 EventID。
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
        //
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

        void LoadPluginsFromDirectory(
            const std::filesystem::path& dir,
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
            //
            // PluginCast<T>
            //
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
                    //
                    // PluginCast<T>
                    //
                    return PluginCast<T>(locked_ptr);
                }
            }

            auto base_obj = it_factory->second.factory();
            if (base_obj)
            {
                singletons_[clsid] = base_obj;
                //
                // PluginCast<T>
                //
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

        /**
         * @internal
         * [修改]
         * 签名从 (ClassID event_id, ...)
         * 更改为 (EventID event_id, ...)
         */
        void SubscribeGlobalImpl(EventID event_id,
            std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) override;
        /**
         * @internal
         * [修改]
         * 签名从 (ClassID event_id, ...)
         * 更改为 (EventID event_id, ...)
         */
        void FireGlobalImpl(EventID event_id, PluginPtr<Event> e_ptr) override;

        /**
         * @internal
         * [修改]
         * 签名从 (..., ClassID event_id, ...)
         * 更改为 (..., EventID event_id, ...)
         */
        void SubscribeToSenderImpl(void* sender_key,
            EventID event_id,
            std::weak_ptr<void> sub_id,
            std::weak_ptr<void> sender_id,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) override;

        /**
         * @internal
         * [修改]
         * 签名从 (..., ClassID event_id, ...)
         * 更改为 (..., EventID event_id, ...)
         */
        void FireToSenderImpl(void* sender_key,
            EventID event_id,
            PluginPtr<Event> e_ptr) override;

    private:
        /**
         * @brief [内部] 通过别名查找 ClassID。
         */
        ClassID GetClsidFromAlias(const std::string& alias);

        /**
         * @brief [内部] 事件循环工作线程的主函数。
         *
         * [Fix 5] (重构):
         * 此函数现在还负责处理 gc_queue_，
         * 异步清理反向查找表。
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
         *
         * [Fix 5] (重构):
         * 此函数现在会将在正向列表中
         * 发现的失效 weak_ptr
         * 放入 gc_queue_ 中，
         * 以便 EventLoop 稍后清理反向查找表。
         *
         * @param[in,out] subs 要清理的订阅列表 (vector)。
         * @param[in] check_sender_also 是否也检查 sender_id_ 的有效性。
         * @param[in,out] gc_queue [Fix 5]
         * 用于暂存失效 weak_ptr 的 GC 队列。
         */
        static void CleanupExpiredSubscriptions(
            std::vector<Subscription>& subs,
            bool check_sender_also,
            std::queue<std::weak_ptr<void>>& gc_queue // [Fix 5] 新增参数
        );

        using EventCallbackList = std::vector<Subscription>;

        /*
         * [修改]
         * EventMap
         * 的键 (Key)
         * 从 ClassID
         * 切换到 EventID。
         */
        using EventMap = std::map<EventID, EventCallbackList>;
        using SenderMap = std::map<void*, EventMap>;
        using EventTask = std::function<void()>;


        // --- [Fix 4] Unsubscribe 性能优化：反向查找表 ---

        /**
         * @brief [Fix 4] 全局事件反向查找表。
         * Key: 订阅者 (weak_ptr)。
         * Value:
         * [修改]
         * 该订阅者订阅的所有全局事件的
         * EventID (event_id)
         * 集合。
         */
        using SubscriberLookupMapG = std::map<
            std::weak_ptr<void>,
            std::set<EventID>, // <-- [修改]
            std::owner_less<std::weak_ptr<void>>
        >;

        /**
         * @brief [Fix 4] 实例事件反向查找表。
         * Key: 订阅者 (weak_ptr)。
         * Value:
         * [修改]
         * 该订阅者订阅的所有实例事件的
         * (sender_key, EventID)
         * 对的集合。
         */
        using SubscriberLookupMapS = std::map<
            std::weak_ptr<void>,
            std::set<std::pair<void*, EventID>>, // <-- [修改]
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

        /**
         * @brief [Fix 1] 保护事件总线订阅表 (使用递归锁)。
         *
         * 保护:
         * 1. global_subscribers_
         * 2. sender_subscribers_
         * 3. global_sub_lookup_ [Fix 4]
         * 4. sender_sub_lookup_ [Fix 4]
         * 5. gc_queue_ [Fix 5]
         */
        std::recursive_mutex event_mutex_;
        EventMap global_subscribers_;
        SenderMap sender_subscribers_;

        // [Fix 4] Unsubscribe 优化查找表
        SubscriberLookupMapG global_sub_lookup_;
        SubscriberLookupMapS sender_sub_lookup_;


        // --- 异步事件总线成员 ---
        std::thread event_loop_thread_;
        std::queue<EventTask> event_queue_;
        std::mutex queue_mutex_; //!< 保护 event_queue_ 和 running_ 标志
        std::condition_variable queue_cv_;
        bool running_;

        /**
         * @brief [Fix 5] 异步垃圾回收队列。
         *
         * CleanupExpiredSubscriptions 会向此队列
         * push 失效的 weak_ptr。
         * EventLoop 线程会从此队列 pop
         * 并清理反向查找表。
         *
         * @design
         * 此队列由 event_mutex_ (递归锁) 保护。
         */
        std::queue<std::weak_ptr<void>> gc_queue_;
    };

} // namespace z3y

#endif // Z3Y_SRC_PLUGIN_MANAGER_PLUGIN_MANAGER_H_