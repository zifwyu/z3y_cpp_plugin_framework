/**
 * @file i_event_bus.h
 * @brief 定义 z3y::IEventBus 接口和 z3y::Event 基类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 此文件定义了框架的“发布/订阅” (Pub/Sub) 系统。
 * IEventBus 是一个单例服务，允许插件间进行匿名的、类型安全的通信。
 *
 * @design
 * 1. 类型安全：使用 `std::type_index` (来自 `typeid`) 作为事件ID。
 * 2. 数据安全：使用 `const TEvent&` 传递数据。
 * 3. 自动生命周期：使用 `std::weak_ptr` 管理订阅者，
 * 自动清理已析构的对象，防止“僵尸回调”崩溃。
 * 4. 双模式：支持 `Global` (广播) 和 `Sender-Specific` (实例信号)。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_EVENT_BUS_H_
#define Z3Y_FRAMEWORK_I_EVENT_BUS_H_

#include "class_id.h"
#include "i_component.h"
#include <functional>
#include <typeindex>
#include <memory>

namespace z3y
{

    /**
     * @struct Event
     * @brief 所有“信号”或“事件”的空基类。
     *
     * 开发者应定义自己的 struct 并公有继承它。
     * 它包含虚析构函数，这意味着派生类不能使用聚合初始化 {}。
     */
    struct Event
    {
        /**
         * @brief 虚析构函数。
         */
        virtual ~Event() = default;
    };

    /**
     * @class IEventBus
     * @brief [框架核心] 事件总线 (信号/槽) 接口。
     *
     * 这是一个单例服务，通过 GetService<IEventBus>(clsid::kEventBus) 获取。
     */
    class IEventBus : public IComponent
    {
    public:
        /**
         * @brief 虚析构函数。
         */
        virtual ~IEventBus() = default;

        // --- 1. 全局广播 (Global Broadcast) ---

        /**
         * @brief [模板] 订阅一个全局事件 (广播)。
         *
         * 订阅是自动管理的。当订阅者(subscriber)的 shared_ptr
         * 计数归零被析构时，订阅会自动失效并被清理。
         *
         * @tparam TEvent 要订阅的事件结构体 (必须继承 z3y::Event)。
         * @tparam TSubscriber 订阅者类 (必须继承 std::enable_shared_from_this)。
         * @param[in] subscriber 订阅者实例 (std::shared_ptr)。
         * @param[in] callback 回调函数 (lambda)，签名应为: void(const TEvent&)。
         */
        template<typename TEvent, typename TSubscriber, typename TCallback>
        void SubscribeGlobal(std::shared_ptr<TSubscriber> subscriber, TCallback&& callback)
        {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");
            static_assert(std::is_base_of_v<std::enable_shared_from_this<TSubscriber>, TSubscriber>,
                "Subscriber must inherit from std::enable_shared_from_this");

            std::type_index event_type = std::type_index(typeid(TEvent));

            // 类型擦除：将 void(const TEvent&) 包装为 void(const Event&)
            std::function<void(const Event&)> wrapper =
                [cb = std::forward<TCallback>(callback)](const Event& e)
                {
                    cb(static_cast<const TEvent&>(e));
                };

            // 类型擦除：将 shared_ptr<TSubscriber> 转换为 weak_ptr<void>
            std::weak_ptr<void> weak_id = subscriber;

            SubscribeGlobalImpl(event_type, std::move(weak_id), std::move(wrapper));
        }

        /**
         * @brief [模板] 发布一个全局事件 (广播)。
         * @tparam TEvent 事件结构体的类型。
         * @param[in] event 填充了数据的事件对象。
         */
        template<typename TEvent>
        void FireGlobal(const TEvent& event)
        {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");
            FireGlobalImpl(std::type_index(typeid(TEvent)), event);
        }

        // --- 2. 实例到实例 (Sender-Specific) ---

        /**
         * @brief [模板] 订阅一个特定发送者的事件 ("我只想听 button_A 的")。
         *
         * @param[in] sender 唯一的发送者实例 (例如 button_A 的 'this' 指针)。
         * @param[in] subscriber 订阅者实例 (std::shared_ptr)，用于自动生命周期管理。
         * @param[in] callback 回调函数 void(const TEvent&)。
         */
        template<typename TEvent, typename TSubscriber, typename TCallback>
        void SubscribeToSender(void* sender,
            std::shared_ptr<TSubscriber> subscriber,
            TCallback&& callback)
        {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");
            static_assert(std::is_base_of_v<std::enable_shared_from_this<TSubscriber>, TSubscriber>,
                "Subscriber must inherit from std::enable_shared_from_this");

            std::type_index event_type = std::type_index(typeid(TEvent));
            std::function<void(const Event&)> wrapper =
                [cb = std::forward<TCallback>(callback)](const Event& e)
                {
                    cb(static_cast<const TEvent&>(e));
                };
            std::weak_ptr<void> weak_id = subscriber;

            SubscribeToSenderImpl(sender, event_type, std::move(weak_id), std::move(wrapper));
        }

        /**
         * @brief [模板] 向订阅了此发送者的订阅者发布事件。
         *
         * @param[in] sender 唯一的发送者实例 (必须是 'this')。
         * @param[in] event 要发布的事件。
         */
        template<typename TEvent>
        void FireToSender(void* sender, const TEvent& event)
        {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");
            FireToSenderImpl(sender, std::type_index(typeid(TEvent)), event);
        }

        // --- 3. 手动生命周期管理 ---

        /**
         * @brief [可选] 立即取消此订阅者(subscriber)的所有订阅。
         */
        virtual void Unsubscribe(std::shared_ptr<void> subscriber) = 0;

        /**
         * @brief [必须] 发送者在析构时必须调用此函数！
         *
         * @design
         * 这将清理 m_sender_subscribers_ 中以此 sender 为键的整个条目，
         * 防止内存泄漏。
         *
         * @param[in] sender 正在被析构的发送者 ('this' 指针)。
         */
        virtual void UnregisterSender(void* sender) = 0;

    protected:
        /** @internal 纯虚函数，由 PluginManager 实现 */
        virtual void SubscribeGlobalImpl(std::type_index type,
            std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb) = 0;

        /** @internal 纯虚函数，由 PluginManager 实现 */
        virtual void FireGlobalImpl(std::type_index type, const Event& e) = 0;

        /** @internal 纯虚函数，由 PluginManager 实现 */
        virtual void SubscribeToSenderImpl(void* sender,
            std::type_index type,
            std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb) = 0;

        /** @internal 纯虚函数，由 PluginManager 实现 */
        virtual void FireToSenderImpl(void* sender,
            std::type_index type,
            const Event& e) = 0;
    };

    /**
     * @brief IEventBus 的全局唯一 ClassID。
     */
    namespace clsid
    {
        /**
         * @brief 框架核心事件总线服务的 ClassID。
         */
        constexpr ClassID kEventBus =
            ConstexprHash("z3y-core-event-bus-uuid-D54E82F1-A376-4E9A-8178-05B1E8A73719");
    }

} // namespace z3y

#endif // Z3Y_FRAMEWORK_I_EVENT_BUS_H_