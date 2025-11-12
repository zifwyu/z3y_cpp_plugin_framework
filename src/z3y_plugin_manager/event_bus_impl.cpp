/**
 * @file event_bus_impl.cpp
 * @brief z3y::PluginManager 类的 IEventBus 接口实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [重构 v2 - Fix 1] (死锁修复):
 * 1. 锁从 std::mutex 切换到 std::recursive_mutex。
 *
 * [重构 v2 - Fix 4] (性能优化):
 * 1. Subscribe...Impl() 填充反向查找表。
 * 2. Unsubscribe() 使用反向查找表实现 O(log N) 移除。
 *
 * [重构 v3 - Fix 5] (泄漏修复):
 * 1. CleanupExpiredSubscriptions 现在会填充 gc_queue_。
 * 2. EventLoop() 会异步消耗此队列。
 *
 * [重构 v4 - Fix 6] (Bug 修复):
 * 1. 修复了 EventLoop (Fix 5) 中引入的 GC 饥饿 Bug。
 * 2. 使用 wait_for() (超时等待) 替代 wait()。
 * 3. 这确保了 EventLoop 即使在没有 EventTask
 * (系统空闲) 的情况下，
 * 也会定期苏醒 (例如每 50ms)，
 * 从而有机会处理 gc_queue_ 中的任务。
 *
 * [v2.1 修复]:
 * 1.
 * 所有 ...Impl
 * 函数的实现都已更新，
 * 使用 EventID
 * 别名替换 ClassID。
 * 2.
 * Unsubscribe()
 * 中的反向查找表遍历已更新为使用 EventID。
 */

#include "plugin_manager.h"
#include <vector>
#include <algorithm> // 用于 std::remove_if
#include <set>
#include <utility>
#include <chrono>    // [Fix 6] 依赖 std::chrono

namespace z3y
{

    /**
     * @brief [辅助函数] 清理已失效的(expired)订阅者 (weak_ptr)。
     *
     * [Fix 5] (重构):
     * 现在将发现的失效 weak_ptr
     * 推入 gc_queue_，
     * 以便 EventLoop 稍后清理反向查找表。
     *
     * @param[in,out] subs 要清理的订阅列表 (vector)。
     * @param[in] check_sender_also 是否也检查 sender_id_。
     * @param[in,out] gc_queue [Fix 5]
     * 用于暂存失效 weak_ptr 的 GC 队列。
     */
    void PluginManager::CleanupExpiredSubscriptions(
        std::vector<PluginManager::Subscription>& subs,
        bool check_sender_also,
        std::queue<std::weak_ptr<void>>& gc_queue
    )
    {
        subs.erase(
            std::remove_if(subs.begin(), subs.end(),
                [&gc_queue, check_sender_also](const PluginManager::Subscription& s)
                {
                    // 检查订阅者是否已失效
                    bool expired = s.subscriber_id_.expired();

                    // 如果订阅者未失效，
                    // 并且我们需要检查发送者...
                    if (!expired && check_sender_also)
                    {
                        // 检查发送者是否已失效
                        // (注意: owner_before 
                        //  检查确保 sender_id_ 
                        //  非空)
                        expired = !s.sender_id_.owner_before(std::weak_ptr<void>())
                            && s.sender_id_.expired();
                    }

                    if (expired)
                    {
                        // [Fix 5] 发现了失效订阅！
                        // 1. 将其放入 GC 队列，以便稍后
                        //    从反向查找表 (map) 中安全移除。
                        gc_queue.push(s.subscriber_id_);

                        // 2. 返回 true，以便 std::remove_if
                        //    将其从正向列表 (vector) 中移除。
                        return true;
                    }
                    return false;
                }),
            subs.end()
        );
    }

    // --- 1. 事件循环 (Event Loop) ---

    /**
     * @brief 事件循环工作线程的主函数。
     *
     * [Fix 6] (Bug 修复):
     * 1. 修复了 "Fix 5" 中引入的 GC 饥饿 Bug。
     * 2. 原始的 wait()
     * 逻辑会导致在系统空闲时 (没有 EventTask)，
     * GC 队列 (gc_queue_) 永远不会被处理。
     * 3. 现在的实现使用 wait_for() (带超时)。
     * 4. 这确保了 EventLoop 即使在没有 EventTask
     * 的情况下，
     * 也会定期 (例如每 50 毫秒) 苏醒一次，
     * 从而有机会处理 GC 队列中的任务。
     */
    void PluginManager::EventLoop()
    {
        // [Fix 6] 定义一个循环超时时间
        const auto kLoopTimeout = std::chrono::milliseconds(50);

        while (true)
        {
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
                if (!running_ && event_queue_.empty())
                {
                    // 析构函数已发出停止信号，
                    // 并且队列已清空，
                    // 安全退出线程。
                    return;
                }

                // 如果是“谓词” (有任务) 
                // 或“超时”后发现有任务
                if (!event_queue_.empty())
                {
                    task_to_run = std::move(event_queue_.front());
                    event_queue_.pop();
                }

            } // 释放 queue_mutex_ 锁

            // 1a. (如果有) 执行异步事件
            if (task_to_run)
            {
                try
                {
                    task_to_run(); // 在工作线程上执行回调
                }
                catch (const std::exception& e)
                {
                    //
                    // FireGlobal<...>
                    //
                    this->FireGlobal<event::AsyncExceptionEvent>(std::string(e.what()));
                }
                catch (...)
                {
                    this->FireGlobal<event::AsyncExceptionEvent>("Unknown exception in async event loop.");
                }
            }

            // --- 2. [Fix 5/6] 垃圾回收 (GC) 阶段 ---
            //
            // 无论是否有 EventTask，
            // 此阶段 *每次循环* // (即至少每 50ms) 都会运行一次。

            // 2a. 尝试从 GC 队列中获取一个失效指针
            {
                // gc_queue_ 
                // 由 event_mutex_ 保护
                std::lock_guard<std::recursive_mutex> lock(event_mutex_);
                if (!gc_queue_.empty())
                {
                    expired_sub_to_gc = gc_queue_.front();
                    gc_queue_.pop();
                }
            } // 释放 event_mutex_

            // 2b. (如果获取到) 执行清理
            if (!expired_sub_to_gc.owner_before(std::weak_ptr<void>()))
            {
                // 重新获取锁并安全地
                // 清理两个反向查找表
                std::lock_guard<std::recursive_mutex> lock(event_mutex_);
                global_sub_lookup_.erase(expired_sub_to_gc);
                sender_sub_lookup_.erase(expired_sub_to_gc);
            }
        } // end while(true)
    }


    // --- 2. 全局事件 (Global Events) ---

    /**
     * @brief [IEventBus 内部实现] 订阅一个全局事件。
     *
     * [修改]
     * 参数 'type'
     * 已重命名为 'event_id'
     * (类型已从 ClassID
     * 更改为 EventID)。
     */
    void PluginManager::SubscribeGlobalImpl(
        EventID event_id, // <-- [修改]
        std::weak_ptr<void> sub, // 订阅者
        std::function<void(const Event&)> cb,
        ConnectionType connection_type)
    {
        std::lock_guard<std::recursive_mutex> lock(event_mutex_);

        // 1. 添加到主订阅列表
        // [修改] 使用 event_id 作为键
        global_subscribers_[event_id].push_back(
            { std::move(sub), std::weak_ptr<void>(), std::move(cb), connection_type }
        );

        // 2. [Fix 4] 添加到反向查找表
        // [修改] 插入 event_id
        global_sub_lookup_[global_subscribers_[event_id].back().subscriber_id_].insert(event_id);
    }

    /**
     * @brief [IEventBus 内部实现] 发布一个全局事件。
     *
     * [Fix 5] (重构):
     * CleanupExpiredSubscriptions
     * 现在会填充 gc_queue_。
     * [Fix 6] (修正):
     * 我们不再需要在 `did_gc_queue`
     * 时手动 notify()。
     * EventLoop 的 wait_for()
     * 超时机制会自动处理 GC 队列。
     *
     * [修改]
     * 参数 'type'
     * 已重命名为 'event_id'
     * (类型已从 ClassID
     * 更改为 EventID)。
     */
    void PluginManager::FireGlobalImpl(EventID event_id, PluginPtr<Event> e_ptr) // <-- [修改]
    {
        std::vector<std::function<void(const Event&)>> direct_calls;
        std::vector<std::function<void(const Event&)>> queued_calls;
        // [Fix 6] 移除 bool did_gc_queue = false;

        {
            std::lock_guard<std::recursive_mutex> lock(event_mutex_);
            // [修改] 使用 event_id 查找
            auto it = global_subscribers_.find(event_id);
            if (it == global_subscribers_.end())
            {
                return;
            }

            // [Fix 5] (核心) 
            // 将 gc_queue_ 传给清理函数。
            CleanupExpiredSubscriptions(it->second, false, gc_queue_);
            // [Fix 6] 
            // (不再需要检查 gc_queue_ 
            //  的大小或设置 did_gc_queue)

            // 3. 将回调分类
            for (const auto& sub : it->second)
            {
                if (sub.connection_type_ == ConnectionType::kDirect)
                {
                    direct_calls.push_back(sub.callback_);
                }
                else
                {
                    queued_calls.push_back(sub.callback_);
                }
            }
        } // 释放 event_mutex_ 锁

        // 4. [同步] 立即在发布者线程上执行
        for (const auto& cb : direct_calls)
        {
            cb(*e_ptr);
        }

        // 5. [异步] 将 kQueued 回调推入队列
        if (!queued_calls.empty())
        {
            EventTask task = [e_ptr, queued_calls]()
                {
                    for (const auto& cb : queued_calls)
                    {
                        cb(*e_ptr);
                    }
                };

            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                event_queue_.push(std::move(task));
            }
            queue_cv_.notify_one(); // 唤醒事件循环 (处理 EventTask)
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
     * 参数 'type'
     * 已重命名为 'event_id'
     * (类型已从 ClassID
     * 更改为 EventID)。
     */
    void PluginManager::SubscribeToSenderImpl(
        void* sender_key,
        EventID event_id, // <-- [修改]
        std::weak_ptr<void> sub_id, // 订阅者
        std::weak_ptr<void> sender_id, // 发送者 (用于清理)
        std::function<void(const Event&)> cb,
        ConnectionType connection_type)
    {
        std::lock_guard<std::recursive_mutex> lock(event_mutex_);

        // 1. 添加到主订阅列表
        // [修改] 使用 event_id 作为内部 map 的键
        sender_subscribers_[sender_key][event_id].push_back(
            { std::move(sub_id), std::move(sender_id), std::move(cb), connection_type }
        );

        // 2. [Fix 4] 添加到反向查找表
        // [修改] 插入 event_id
        sender_sub_lookup_[sender_subscribers_[sender_key][event_id].back().subscriber_id_]
            .insert({ sender_key, event_id });
    }

    /**
     * @brief [IEventBus 内部实现] 发布一个特定发送者的事件。
     *
     * [Fix 6] (修正):
     * (同 FireGlobalImpl)
     * 移除 did_gc_queue 逻辑。
     *
     * [修改]
     * 参数 'type'
     * 已重命名为 'event_id'
     * (类型已从 ClassID
     * 更改为 EventID)。
     */
    void PluginManager::FireToSenderImpl(void* sender_key,
        EventID event_id, // <-- [修改]
        PluginPtr<Event> e_ptr)
    {
        std::vector<std::function<void(const Event&)>> direct_calls;
        std::vector<std::function<void(const Event&)>> queued_calls;
        // [Fix 6] 移除 bool did_gc_queue = false;

        {
            std::lock_guard<std::recursive_mutex> lock(event_mutex_);
            auto sender_it = sender_subscribers_.find(sender_key);
            if (sender_it == sender_subscribers_.end())
            {
                return;
            }

            // [修改] 使用 event_id 查找
            auto event_it = sender_it->second.find(event_id);
            if (event_it == sender_it->second.end())
            {
                return;
            }

            // [Fix 5] (核心)
            // 将 gc_queue_ 传入清理函数
            CleanupExpiredSubscriptions(event_it->second, true, gc_queue_);
            // [Fix 6] (不再需要 did_gc_queue)

            // 分类回调
            for (const auto& sub : event_it->second)
            {
                if (sub.connection_type_ == ConnectionType::kDirect)
                {
                    direct_calls.push_back(sub.callback_);
                }
                else
                {
                    queued_calls.push_back(sub.callback_);
                }
            }
        } // 释放 event_mutex_ 锁

        // 1. [同步] 立即在发布者线程上执行
        for (const auto& cb : direct_calls)
        {
            cb(*e_ptr);
        }

        // 2. [异步] 将任务推入队列
        if (!queued_calls.empty())
        {
            EventTask task = [e_ptr, queued_calls]()
                {
                    for (const auto& cb : queued_calls)
                    {
                        cb(*e_ptr);
                    }
                };

            {
                std::lock_guard<std::mutex> lock(queue_mutex_);
                event_queue_.push(std::move(task));
            }
            queue_cv_.notify_one(); // 唤醒 (处理 EventTask)
        }
        // [Fix 6] 移除 (else if (did_gc_queue))
    }

    // --- 4. 手动生命周期管理 ---

    /**
     * @brief [IEventBus 接口实现] [可选] 立即取消订阅者所有的订阅。
     *
     * [Fix 4] (性能优化):
     * 使用反向查找表来精确定位并删除订阅。
     *
     * [修改]
     * 内部循环变量 'type'
     * 已重命名为 'event_id'
     * (类型已从 ClassID
     * 更改为 EventID)。
     */
    void PluginManager::Unsubscribe(std::shared_ptr<void> subscriber)
    {
        std::lock_guard<std::recursive_mutex> lock(event_mutex_);

        std::weak_ptr<void> weak_id = subscriber;

        auto is_same_subscriber = [&weak_id](const Subscription& s)
            {
                return !s.subscriber_id_.owner_before(weak_id)
                    && !weak_id.owner_before(s.subscriber_id_);
            };


        // --- 3. [Fix 4] 处理全局订阅 ---
        auto global_it = global_sub_lookup_.find(weak_id);
        if (global_it != global_sub_lookup_.end())
        {
            // [修改]
            // 'type' 
            // -> 'event_id'
            // (类型已变为 EventID)
            for (const EventID& event_id : global_it->second)
            {
                auto event_list_it = global_subscribers_.find(event_id);
                if (event_list_it != global_subscribers_.end())
                {
                    auto& subs = event_list_it->second;
                    subs.erase(std::remove_if(subs.begin(), subs.end(), is_same_subscriber),
                        subs.end());
                }
            }
            // 从反向查找表中移除
            global_sub_lookup_.erase(global_it);
        }

        // --- 4. [Fix 4] 处理实例订阅 ---
        auto sender_it = sender_sub_lookup_.find(weak_id);
        if (sender_it != sender_sub_lookup_.end())
        {
            // [修改]
            // pair 类型现在是
            // (void*, EventID)
            for (const auto& pair : sender_it->second)
            {
                void* sender_key = pair.first;
                // [修改]
                // 'type' 
                // -> 'event_id'
                // (类型已变为 EventID)
                const EventID& event_id = pair.second;

                auto sender_map_it = sender_subscribers_.find(sender_key);
                if (sender_map_it != sender_subscribers_.end())
                {
                    auto event_list_it = sender_map_it->second.find(event_id);
                    if (event_list_it != sender_map_it->second.end())
                    {
                        auto& subs = event_list_it->second;
                        subs.erase(std::remove_if(subs.begin(), subs.end(), is_same_subscriber),
                            subs.end());
                    }
                }
            }
            // 从反向查找表中移除
            sender_sub_lookup_.erase(sender_it);
        }
    }

} // namespace z3y