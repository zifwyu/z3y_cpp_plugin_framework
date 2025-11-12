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
 *
 * [v2.1 修复]:
 * 1.
 * 所有 ...Impl
 * 纯虚函数的签名
 * 已从 ClassID
 * 切换到 EventID
 * 别名，
 * 以明确意图。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_EVENT_BUS_H_
#define Z3Y_FRAMEWORK_I_EVENT_BUS_H_

#include "class_id.h"         //
#include "i_component.h"      //
#include "connection_type.h"  //
#include <functional>
#include <typeindex>          //
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
         * @brief 为 IEventBus 接口定义一个唯一的接口 ID (IID)。
         */
        static constexpr ClassID kIID =
            ConstexprHash("z3y-core-IEventBus-IID-A0000002");

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

            /*
             * [修改]
             * 关键改动：
             * 从 std::type_index(typeid(TEvent))
             * 切换到 TEvent::kEventID。
             * TEvent 结构体现在必须定义一个
             * 'static constexpr ClassID kEventID'。
             */
            ClassID event_id = TEvent::kEventID;

            std::function<void(const Event&)> wrapper =
                [cb = std::forward<TCallback>(callback)](const Event& e)
                {
                    cb(static_cast<const TEvent&>(e));
                };

            std::weak_ptr<void> weak_id = subscriber;

            // [修改] 调用 ...Impl
            SubscribeGlobalImpl(event_id, std::move(weak_id), std::move(wrapper), type);
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

            /*
             * [修改]
             * 关键改动：
             * 使用 TEvent::kEventID 作为事件标识符。
             */
            FireGlobalImpl(TEvent::kEventID, base_event);
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

            // [修改] 使用 kEventID
            ClassID event_id = TEvent::kEventID;

            std::function<void(const Event&)> wrapper =
                [cb = std::forward<TCallback>(callback)](const Event& e)
                {
                    cb(static_cast<const TEvent&>(e));
                };

            std::weak_ptr<void> weak_sub_id = subscriber;
            std::weak_ptr<void> weak_sender_id = sender;
            void* sender_key = sender.get();

            // [修改] 调用 ...Impl
            SubscribeToSenderImpl(sender_key,
                event_id,
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

            // [修改] 使用 kEventID
            FireToSenderImpl(sender_key, TEvent::kEventID, base_event);
        }

        // --- 3. 手动生命周期管理 ---

        /**
         * @brief [可选] 立即取消此订阅者(subscriber)的所有订阅。
         */
        virtual void Unsubscribe(std::shared_ptr<void> subscriber) = 0;

    protected:
        /**
         * @internal
         * [修改] 参数从 ClassID 更改为 EventID
         */
        virtual void SubscribeGlobalImpl(EventID event_id,
            std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) = 0;

        /**
         * @internal
         * [修改] 参数从 ClassID 更改为 EventID
         */
        virtual void FireGlobalImpl(EventID event_id, PluginPtr<Event> e_ptr) = 0;

        /**
         * @internal
         * [修改] 参数从 ClassID 更改为 EventID
         */
        virtual void SubscribeToSenderImpl(void* sender_key,
            EventID event_id,
            std::weak_ptr<void> sub_id,
            std::weak_ptr<void> sender_id,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) = 0;

        /**
         * @internal
         * [修改] 参数从 ClassID 更改为 EventID
         */
        virtual void FireToSenderImpl(void* sender_key,
            EventID event_id,
            PluginPtr<Event> e_ptr) = 0;
    };

    /**
     * @brief IEventBus 的全局唯一 ClassID。
     */
    namespace clsid
    {
        /**
         * @brief 框架核心事件总线服务的 ClassID。
         *
         * 注意：这是 *实现* (PluginManager) 的 ClassID，
         * 而不是 IEventBus 接口的 IID。
         */
        constexpr ClassID kEventBus =
            ConstexprHash("z3y-core-event-bus-uuid-D54E82F1-A376-4E9A-8178-05B1E8A73719");
    }

} // namespace z3y

#endif // Z3Y_FRAMEWORK_I_EVENT_BUS_H_