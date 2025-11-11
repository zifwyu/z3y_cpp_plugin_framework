/**
 * @file event_bus_impl.cpp
 * @brief z3y::PluginManager 类的 IEventBus 接口实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 此文件实现了事件总线的所有功能，包括订阅、发布、
 * 自动生命周期清理和实例订阅。
 * 将这部分代码与 plugin_manager.cpp 分离，
 * 是为了保持“按功能内聚”。
 */

#include "plugin_manager.h"
#include <vector>
#include <algorithm> // 用于 std::remove_if

namespace z3y
{
    /**
     * @brief [辅助函数] 清理已失效的(expired)订阅者 (weak_ptr)。
     *
     * 这是一个私有的静态成员函数，因为它需要访问
     * private 的 PluginManager::Subscription 结构体。
     *
     * @param[in,out] subs 一个订阅列表 (std::vector<Subscription>)。
     */
    void PluginManager::CleanupExpiredSubscriptions(
        std::vector<PluginManager::Subscription>& subs)
    {
        // C++17 erase-remove idiom
        subs.erase(
            std::remove_if(subs.begin(), subs.end(),
                [](const PluginManager::Subscription& s)
                {
                    // 检查 weak_ptr 是否已过期（即订阅者已被析构）
                    return s.subscriber_id.expired();
                }),
            subs.end()
        );
    }

    // --- 1. 全局事件 (Global Events) ---

    /**
     * @brief [IEventBus 内部实现] 订阅一个全局事件。
     */
    void PluginManager::SubscribeGlobalImpl(
        std::type_index type,
        std::weak_ptr<void> sub,
        std::function<void(const Event&)> cb)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        global_subscribers_[type].push_back({ std::move(sub), std::move(cb) });
    }

    /**
     * @brief [IEventBus 内部实现] 发布一个全局事件。
     */
    void PluginManager::FireGlobalImpl(std::type_index type, const Event& e)
    {
        // 1. 复制回调列表，以最小化锁的持有时间。
        std::vector<std::function<void(const Event&)>> callbacks_to_run;
        {
            std::lock_guard<std::mutex> lock(mutex_);
            auto it = global_subscribers_.find(type);
            if (it == global_subscribers_.end())
            {
                return; // 此事件没有任何订阅者
            }

            // 2. [自动清理] 清理已析构的订阅者
            CleanupExpiredSubscriptions(it->second);

            // 3. 复制所有存活的回调函数
            for (const auto& sub : it->second)
            {
                callbacks_to_run.push_back(sub.callback);
            }
        } // 锁在这里释放

        // 4. 在锁之外执行所有回调
        for (const auto& cb : callbacks_to_run)
        {
            cb(e); // 调用被包装的 lambda
        }
    }

    // --- 2. 实例事件 (Sender-Specific Events) ---

    /**
     * @brief [IEventBus 内部实现] 订阅一个特定发送者的事件。
     */
    void PluginManager::SubscribeToSenderImpl(
        void* sender,
        std::type_index type,
        std::weak_ptr<void> sub,
        std::function<void(const Event&)> cb)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        sender_subscribers_[sender][type].push_back({ std::move(sub), std::move(cb) });
    }

    /**
     * @brief [IEventBus 内部实现] 发布一个特定发送者的事件。
     */
    void PluginManager::FireToSenderImpl(void* sender, std::type_index type, const Event& e)
    {
        std::vector<std::function<void(const Event&)>> callbacks_to_run;
        {
            std::lock_guard<std::mutex> lock(mutex_);

            // 1. 查找发送者
            auto sender_it = sender_subscribers_.find(sender);
            if (sender_it == sender_subscribers_.end())
            {
                return;
            }

            // 2. 查找事件类型
            auto event_it = sender_it->second.find(type);
            if (event_it == sender_it->second.end())
            {
                return;
            }

            // 3. [自动清理]
            CleanupExpiredSubscriptions(event_it->second);

            // 4. 复制
            for (const auto& sub : event_it->second)
            {
                callbacks_to_run.push_back(sub.callback);
            }
        } // 锁释放

        // 5. 在锁外执行
        for (const auto& cb : callbacks_to_run)
        {
            cb(e);
        }
    }

    // --- 3. 手动生命周期管理 ---

    /**
     * @brief [IEventBus 接口实现] 发送者在析构时必须调用此函数！
     */
    void PluginManager::UnregisterSender(void* sender)
    {
        std::lock_guard<std::mutex> lock(mutex_);
        // 清理发送者映射，防止内存泄漏
        sender_subscribers_.erase(sender);
    }

    /**
     * @brief [IEventBus 接口实现] [可选] 立即取消订阅者所有的订阅。
     */
    void PluginManager::Unsubscribe(std::shared_ptr<void> subscriber)
    {
        std::lock_guard<std::mutex> lock(mutex_);

        std::weak_ptr<void> weak_id = subscriber;

        // 辅助 lambda，用于比较 weak_ptr
        auto is_same_subscriber = [&weak_id](const Subscription& s)
            {
                // 比较 weak_ptr 是否指向同一个控制块
                return !s.subscriber_id.owner_before(weak_id)
                    && !weak_id.owner_before(s.subscriber_id);
            };

        // 清理全局订阅
        for (auto& [type, subs] : global_subscribers_)
        {
            subs.erase(std::remove_if(subs.begin(), subs.end(), is_same_subscriber),
                subs.end());
        }

        // 清理实例订阅
        for (auto& [sender, event_map] : sender_subscribers_)
        {
            for (auto& [type, subs] : event_map)
            {
                subs.erase(std::remove_if(subs.begin(), subs.end(), is_same_subscriber),
                    subs.end());
            }
        }
    }

} // namespace z3y