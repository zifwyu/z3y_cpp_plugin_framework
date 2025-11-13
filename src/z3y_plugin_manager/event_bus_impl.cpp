/**
 * @file event_bus_impl.cpp
 * @brief z3y::PluginManager 类的 IEventBus 接口实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ... (原有修复日志) ...
 * [修改]
 * 1. 遵从 Google 命名约定 (EventId,
 * struct members without trailing _)。
 */

#include "plugin_manager.h"
#include <algorithm>  // 用于 std::remove_if
#include <chrono>     // [Fix 6] 依赖 std::chrono
#include <set>
#include <utility>
#include <vector>
#include <unordered_map> // [!! 新增 !!] 用于 EventMap/SenderMap 实现

namespace z3y {

    /**
     * @brief [辅助函数] 清理已失效的(expired)订阅者 (weak_ptr)。
     *
     * [Fix 5] (重构):
     * ...
     * [修改] 使用 struct member (s.subscriber_id)
     */
    void PluginManager::CleanupExpiredSubscriptions(
        std::vector<PluginManager::Subscription>& subs, bool check_sender_also,
        std::queue<std::weak_ptr<void>>& gc_queue) {
        subs.erase(
            std::remove_if(
                subs.begin(), subs.end(),
                [&gc_queue, check_sender_also](
                    const PluginManager::Subscription& s) {
                        // 检查订阅者是否已失效
                        bool expired = s.subscriber_id.expired();

                        // 如果订阅者未失效，
                        // 并且我们需要检查发送者...
                        if (!expired && check_sender_also) {
                            // 检查发送者是否已失效
                            // (注意: owner_before
                            //  检查确保 sender_id
                            //  非空)
                            expired = !s.sender_id.owner_before(std::weak_ptr<void>()) &&
                                s.sender_id.expired();
                        }

                        if (expired) {
                            // [Fix 5] 发现了失效订阅！
                            // 1. 将其放入 GC 队列...
                            gc_queue.push(s.subscriber_id);

                            // 2. 返回 true...
                            return true;
                        }
                        return false;
                }),
            subs.end());
    }

    // --- 1. 事件循环 (Event Loop) ---

    /**
     * @brief 事件循环工作线程的主函数。
     *
     * ... (Fix 6 日志) ...
     */
    void PluginManager::EventLoop() {
        // [Fix 6] 定义一个循环超时时间
        const auto kLoopTimeout = std::chrono::milliseconds(50);

        while (true) {
            EventTask task_to_run;
            std::weak_ptr<void> expired_sub_to_gc;

            // --- 1. 异步事件 (EventTask) 阶段 ---
            {
                std::unique_lock<std::mutex> lock(queue_mutex_);

                // [Fix 6] 使用 wait_for 替代 wait
                queue_cv_.wait_for(lock, kLoopTimeout, [this] {
                    // 谓词 (predicate) 不变：
                    // 仅当有任务或停止时才“立即”唤醒
                    return !event_queue_.empty() || !running_;
                    });

                // 检查退出条件
                if (!running_ && event_queue_.empty()) {
                    // 析构函数已发出停止信号，
                    // 并且队列已清空，
                    // 安全退出线程。
                    return;
                }

                // 如果是“谓词” (有任务)
                // 或“超时”后发现有任务
                if (!event_queue_.empty()) {
                    task_to_run = std::move(event_queue_.front());
                    event_queue_.pop();
                }

            }  // 释放 queue_mutex_ 锁

            // 1a. (如果有) 执行异步事件
            if (task_to_run) {
                try {
                    task_to_run();  // 在工作线程上执行回调
                }
                catch (const std::exception& e) {
                    //
                    // FireGlobal<...>
                    //
                    this->FireGlobal<event::AsyncExceptionEvent>(
                        std::string(e.what()));
                }
                catch (...) {
                    this->FireGlobal<event::AsyncExceptionEvent>(
                        "Unknown exception in async event loop.");
                }
            }

            // --- 2. [Fix 5/6] 垃圾回收 (GC) 阶段 ---
            //
            // ... (GC 阶段注释) ...

            // 2a. 尝试从 GC 队列中获取一个失效指针
            {
                // gc_queue_
                // 由 event_mutex_ 保护
                std::lock_guard<std::recursive_mutex> lock(event_mutex_);
                if (!gc_queue_.empty()) {
                    expired_sub_to_gc = gc_queue_.front();
                    gc_queue_.pop();
                }
            }  // 释放 event_mutex_

            // 2b. (如果获取到) 执行清理
            if (!expired_sub_to_gc.owner_before(std::weak_ptr<void>())) {
                // 重新获取锁并安全地
                // 清理两个反向查找表
                std::lock_guard<std::recursive_mutex> lock(event_mutex_);
                // Note: global_sub_lookup_ is std::map
                global_sub_lookup_.erase(expired_sub_to_gc);
                // Note: sender_sub_lookup_ is std::map
                sender_sub_lookup_.erase(expired_sub_to_gc);
            }
        }  // end while(true)
    }


    // --- 2. 全局事件 (Global Events) ---

    /**
     * @brief [IEventBus 内部实现] 订阅一个全局事件。
     *
     * [修改]
     * 参数 'type'
     * 已重命名为 'event_id'
     * (类型已从 ClassId
     * 更改为 EventId)。
     */
    void PluginManager::SubscribeGlobalImpl(
        EventId event_id,                       // <-- [修改]
        std::weak_ptr<void> sub,              // 订阅者
        std::function<void(const Event&)> cb,
        ConnectionType connection_type) {
        std::lock_guard<std::recursive_mutex> lock(event_mutex_);

        // 1. 添加到主订阅列表
        // [修改] 使用 event_id 作为键
        // Note: global_subscribers_ (EventMap) is now unordered_map.
        global_subscribers_[event_id].push_back(
            { std::move(sub), std::weak_ptr<void>(), std::move(cb),
             connection_type });

        // 2. [Fix 4] 添加到反向查找表
        // [修改] 插入 event_id
        // Note: global_sub_lookup_ is std::map
        global_sub_lookup_[global_subscribers_[event_id].back().subscriber_id]
            .insert(event_id);
    }

    /**
     * @brief [IEventBus 内部实现] 发布一个全局事件。
     *
     * ... (Fix 5/6 日志) ...
     *
     * [修改]
     * 参数 'type'
     * 已重命名为 'event_id'
     * (类型已从 ClassId
     * 更改为 EventId)。
     */
    void PluginManager::FireGlobalImpl(EventId event_id,
        PluginPtr<Event> e_ptr)  // <-- [修改]
    {
        std::vector<std::function<void(const Event&)>> direct_calls;
        std::vector<std::function<void(const Event&)>> queued_calls;
        // [Fix 6] 移除 bool did_gc_queue = false;

        {
            std::lock_guard<std::recursive_mutex> lock(event_mutex_);
            // [修改] 使用 event_id 查找
            // Note: global_subscribers_ (EventMap) is now unordered_map.
            auto it = global_subscribers_.find(event_id);
            if (it == global_subscribers_.end()) {
                return;
            }

            // [Fix 5] (核心)
            // 将 gc_queue_ 传给清理函数。
            CleanupExpiredSubscriptions(it->second, false, gc_queue_);
            // [Fix 6]
            // (不再需要检查 gc_queue_
            //  的大小或设置 did_gc_queue)

            // 3. 将回调分类
            for (const auto& sub : it->second) {
                if (sub.connection_type == ConnectionType::kDirect) {
                    direct_calls.push_back(sub.callback);
                }
                else {
                    queued_calls.push_back(sub.callback);
                }
            }
        }  // 释放 event_mutex_ 锁

        // 4. [同步] 立即在发布者线程上执行
        for (const auto& cb : direct_calls) {
            cb(*e_ptr);
        }

        // 5. [异步] 将 kQueued 回调推入队列
        if (!queued_calls.empty()) {
            EventTask task = [e_ptr, queued_calls]() {
                for (const auto& cb : queued_calls) {
                    cb(*e_ptr);
                }
                };

            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                event_queue_.push(std::move(task));
            }
            queue_cv_.notify_one();  // 唤醒事件循环 (处理 EventTask)
        }
        // [Fix 6]
        // 移除了 (else if (did_gc_queue)) 分支，
        // 因为不再需要它。
    }

    // --- 3. 实例事件 (Sender-Specific Events) ---

    /**
     * @brief [IEventBus 内部实现] 订阅一个特定发送者的事件。
     *
     * [修改]
     * ... (同上, 切换到 EventId) ...
     */
    void PluginManager::SubscribeToSenderImpl(
        void* sender_key,
        EventId event_id,  // <-- [修改]
        std::weak_ptr<void> sub_id,              // 订阅者
        std::weak_ptr<void> sender_id,           // 发送者 (用于清理)
        std::function<void(const Event&)> cb,
        ConnectionType connection_type) {
        std::lock_guard<std::recursive_mutex> lock(event_mutex_);

        // 1. 添加到主订阅列表
        // [修改] 使用 event_id 作为内部 map 的键
        // Note: sender_subscribers_ (SenderMap) is now unordered_map.
        sender_subscribers_[sender_key][event_id].push_back(
            { std::move(sub_id), std::move(sender_id), std::move(cb),
             connection_type });

        // 2. [Fix 4] 添加到反向查找表
        // [修改] 插入 event_id
        // Note: sender_sub_lookup_ is std::map
        sender_sub_lookup_
            [sender_subscribers_[sender_key][event_id].back().subscriber_id]
            .insert({ sender_key, event_id });
    }

    /**
     * @brief [IEventBus 内部实现] 发布一个特定发送者的事件。
     *
     * ... (Fix 6, EventId 修改日志) ...
     */
    void PluginManager::FireToSenderImpl(void* sender_key,
        EventId event_id,  // <-- [修改]
        PluginPtr<Event> e_ptr) {
        std::vector<std::function<void(const Event&)>> direct_calls;
        std::vector<std::function<void(const Event&)>> queued_calls;
        // [Fix 6] 移除 bool did_gc_queue = false;

        {
            std::lock_guard<std::recursive_mutex> lock(event_mutex_);
            // Note: sender_subscribers_ (SenderMap) is now unordered_map.
            auto sender_it = sender_subscribers_.find(sender_key);
            if (sender_it == sender_subscribers_.end()) {
                return;
            }

            // [修改] 使用 event_id 查找
            // Note: sender_it->second (EventMap) is now unordered_map.
            auto event_it = sender_it->second.find(event_id);
            if (event_it == sender_it->second.end()) {
                return;
            }

            // [Fix 5] (核心)
            // 将 gc_queue_ 传入清理函数
            CleanupExpiredSubscriptions(event_it->second, true, gc_queue_);
            // [Fix 6] (不再需要 did_gc_queue)

            // 分类回调
            for (const auto& sub : event_it->second) {
                if (sub.connection_type == ConnectionType::kDirect) {
                    direct_calls.push_back(sub.callback);
                }
                else {
                    queued_calls.push_back(sub.callback);
                }
            }
        }  // 释放 event_mutex_ 锁

        // 1. [同步] 立即在发布者线程上执行
        for (const auto& cb : direct_calls) {
            cb(*e_ptr);
        }

        // 2. [异步] 将任务推入队列
        if (!queued_calls.empty()) {
            EventTask task = [e_ptr, queued_calls]() {
                for (const auto& cb : queued_calls) {
                    cb(*e_ptr);
                }
                };

            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                event_queue_.push(std::move(task));
            }
            queue_cv_.notify_one();  // 唤醒 (处理 EventTask)
        }
        // [Fix 6] 移除 (else if (did_gc_queue))
    }

    // --- 4. 手动生命周期管理 ---

    /**
     * @brief [IEventBus 接口实现] [可选] 立即取消订阅者所有的订阅。
     *
     * ... (Fix 4, EventId 修改日志) ...
     */
    void PluginManager::Unsubscribe(std::shared_ptr<void> subscriber) {
        std::lock_guard<std::recursive_mutex> lock(event_mutex_);

        std::weak_ptr<void> weak_id = subscriber;

        auto is_same_subscriber = [&weak_id](const Subscription& s) {
            return !s.subscriber_id.owner_before(weak_id) &&
                !weak_id.owner_before(s.subscriber_id);
            };


        // --- 3. [Fix 4] 处理全局订阅 ---
        // Note: global_sub_lookup_ is std::map
        auto global_it = global_sub_lookup_.find(weak_id);
        if (global_it != global_sub_lookup_.end()) {
            // [修改]
            // 'type'
            // -> 'event_id'
            // (类型已变为 EventId)
            for (const EventId& event_id : global_it->second) {
                // Note: global_subscribers_ (EventMap) is now unordered_map.
                auto event_list_it = global_subscribers_.find(event_id);
                if (event_list_it != global_subscribers_.end()) {
                    auto& subs = event_list_it->second;
                    subs.erase(std::remove_if(subs.begin(), subs.end(),
                        is_same_subscriber),
                        subs.end());
                }
            }
            // 从反向查找表中移除
            global_sub_lookup_.erase(global_it);
        }

        // --- 4. [Fix 4] 处理实例订阅 ---
        // Note: sender_sub_lookup_ is std::map
        auto sender_it = sender_sub_lookup_.find(weak_id);
        if (sender_it != sender_sub_lookup_.end()) {
            // [修改]
            // pair 类型现在是
            // (void*, EventId)
            for (const auto& pair : sender_it->second) {
                void* sender_key = pair.first;
                // [修改]
                // 'type'
                // -> 'event_id'
                // (类型已变为 EventId)
                const EventId& event_id = pair.second;

                // Note: sender_subscribers_ (SenderMap) is now unordered_map.
                auto sender_map_it = sender_subscribers_.find(sender_key);
                if (sender_map_it != sender_subscribers_.end()) {
                    // Note: sender_map_it->second (EventMap) is now unordered_map.
                    auto event_list_it = sender_map_it->second.find(event_id);
                    if (event_list_it != sender_map_it->second.end()) {
                        auto& subs = event_list_it->second;
                        subs.erase(std::remove_if(subs.begin(), subs.end(),
                            is_same_subscriber),
                            subs.end());
                    }
                }
            }
            // 从反向查找表中移除
            sender_sub_lookup_.erase(sender_it);
        }
    }

}  // namespace z3y