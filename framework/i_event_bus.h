/**
 * @file i_event_bus.h
 * @brief 定义 z3y::IEventBus 接口和 z3y::Event 基类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [重构 v5.1 - Bug 修复]
 * 1. `IEventBus`
 * 现在继承 `public virtual IComponent`。
 * 2.
 * 这是为了配合 `PluginImpl`
 * 解决“钻石继承”
 * 歧义 (C2594)。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_EVENT_BUS_H_
#define Z3Y_FRAMEWORK_I_EVENT_BUS_H_

#include "class_id.h"         //
#include "i_component.h"      //
#include "connection_type.h"  //
#include <functional>
#include <typeindex>
#include <memory>
#include <utility>      // 用于 std::forward

namespace z3y
{

    /**
     * @struct Event
     * @brief 所有“信号”或“事件”的空基类。
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
     * @design
     * [Fix]
     * 必须使用 `public virtual IComponent`
     * 继承，
     * 以防止 `PluginManager`
     * (它实现了 `IEventBus`)
     * 中
     * 出现 `IComponent`
     * 的歧义。
     */
    class IEventBus : public virtual IComponent // <-- [THE FIX] 
        //      添加 virtual
    {
    public:
        /**
         * @brief 虚析构函数。
         */
        virtual ~IEventBus() = default;

        // --- 1. 全局广播 (Global Broadcast) ---

        /**
         * @brief [模板] 订阅一个全局事件 (广播)。
         */
        template<typename TEvent, typename TSubscriber, typename TCallback>
        void SubscribeGlobal(std::shared_ptr<TSubscriber> subscriber,
            TCallback&& callback,
            ConnectionType type = ConnectionType::kDirect)
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

            SubscribeGlobalImpl(event_type, std::move(weak_id), std::move(wrapper), type);
        }

        /**
         * @brief [模板] 发布一个全局事件 (广播)。
         */
        template<typename TEvent, typename... Args>
        void FireGlobal(Args&&... args)
        {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");

            PluginPtr<TEvent> event_ptr =
                std::make_shared<TEvent>(std::forward<Args>(args)...);

            PluginPtr<Event> base_event = event_ptr;
            FireGlobalImpl(std::type_index(typeid(TEvent)), base_event);
        }

        // --- 2. 实例到实例 (Sender-Specific) ---

        /**
         * @brief [模板] 订阅一个特定发送者的事件。
         */
        template<typename TEvent, typename TSender, typename TSubscriber, typename TCallback>
        void SubscribeToSender(std::shared_ptr<TSender> sender,
            std::shared_ptr<TSubscriber> subscriber,
            TCallback&& callback,
            ConnectionType type = ConnectionType::kDirect)
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

            std::weak_ptr<void> weak_sub_id = subscriber;
            std::weak_ptr<void> weak_sender_id = sender;
            void* sender_key = sender.get();

            SubscribeToSenderImpl(sender_key,
                event_type,
                std::move(weak_sub_id),
                std::move(weak_sender_id),
                std::move(wrapper),
                type);
        }

        /**
         * @brief [模板] 向订阅了此发送者的订阅者发布事件。
         */
        template<typename TEvent, typename TSender, typename... Args>
        void FireToSender(std::shared_ptr<TSender> sender, Args&&... args)
        {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");

            PluginPtr<TEvent> event_ptr =
                std::make_shared<TEvent>(std::forward<Args>(args)...);

            PluginPtr<Event> base_event = event_ptr;
            void* sender_key = sender.get();

            FireToSenderImpl(sender_key, std::type_index(typeid(TEvent)), base_event);
        }

        // --- 3. 手动生命周期管理 ---

        /**
         * @brief [可选] 立即取消此订阅者(subscriber)的所有订阅。
         */
        virtual void Unsubscribe(std::shared_ptr<void> subscriber) = 0;

    protected:
        /** @internal */
        virtual void SubscribeGlobalImpl(std::type_index type,
            std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) = 0;

        /** @internal */
        virtual void FireGlobalImpl(std::type_index type, PluginPtr<Event> e_ptr) = 0;

        /** @internal */
        virtual void SubscribeToSenderImpl(void* sender_key,
            std::type_index type,
            std::weak_ptr<void> sub_id,
            std::weak_ptr<void> sender_id,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) = 0;

        /** @internal */
        virtual void FireToSenderImpl(void* sender_key,
            std::type_index type,
            PluginPtr<Event> e_ptr) = 0;
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