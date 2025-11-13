/**
 * @file i_event_bus.h
 * @brief 定义 z3y::IEventBus 接口和 z3y::Event 基类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * 1.
 * 遵从 Google 命名约定
 * 2. [FIX]
 * 修正 SubscribeGlobal / SubscribeToSender
 * 3. [修改]
 * 使用 Z3Y_DEFINE_INTERFACE
 * 宏 (
 * 版本 1.0)
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_EVENT_BUS_H_
#define Z3Y_FRAMEWORK_I_EVENT_BUS_H_

#include "framework/class_id.h"
#include "framework/i_component.h"
#include "framework/connection_type.h"
#include "framework/interface_helpers.h" // [新]
#include <functional>
#include <typeindex>
#include <memory>
#include <utility>

namespace z3y {

    /**
     * @struct Event
     * @brief 所有“信号”或“事件”的空基类。
     */
    struct Event {
        /**
         * @brief 虚析构函数。
         */
        virtual ~Event() = default;
    };

    /**
     * @class IEventBus
     * @brief [框架核心] 事件总线 (信号/槽) 接口。
     */
    class IEventBus : public virtual IComponent
    {
    public:
        /**
         * @brief [修改]
         * 使用 Z3Y_DEFINE_INTERFACE
         * 宏 (
         * 定义为 1.0
         * 版本)
         */
        Z3Y_DEFINE_INTERFACE(IEventBus, "z3y-core-IEventBus-IID-A0000002", \
            1, 0)

            /**
             * @brief 虚析构函数。
             */
            virtual ~IEventBus() = default;

        // --- 1. 全局广播 (Global Broadcast) ---

        /**
         * @brief [模板] 订阅一个全局事件 (广播)。
         */
        template <typename TEvent, typename TSubscriber, typename TCallback>
        void SubscribeGlobal(std::shared_ptr<TSubscriber> subscriber,
            TCallback&& callback,
            ConnectionType type = ConnectionType::kDirect) {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");
            static_assert(
                std::is_base_of_v<std::enable_shared_from_this<TSubscriber>,
                TSubscriber>,
                "Subscriber must inherit from std::enable_shared_from_this");

            EventId event_id = TEvent::kEventId;

            std::weak_ptr<TSubscriber> weak_sub = subscriber;

            std::function<void(const Event&)> wrapper =
                [weak_sub, cb = std::forward<TCallback>(callback)](const Event& e) {
                if (auto sub = weak_sub.lock()) {
                    (sub.get()->*cb)(static_cast<const TEvent&>(e));
                }
                };

            std::weak_ptr<void> weak_id = subscriber;

            SubscribeGlobalImpl(event_id, std::move(weak_id), std::move(wrapper),
                type);
        }

        /**
         * @brief [模板] 发布一个全局事件 (广播)。
         * [!! 修改 !!] 增加 IsGlobalSubscribed 检查，实现条件式创建。
         */
        template <typename TEvent, typename... Args>
        void FireGlobal(Args&&... args) {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");

            EventId event_id = TEvent::kEventId;

            // [!! 核心优化 !!] 检查是否有订阅者，如果没有则避免构造事件对象
            if (!IsGlobalSubscribed(event_id)) {
                return;
            }

            PluginPtr<TEvent> event_ptr =
                std::make_shared<TEvent>(std::forward<Args>(args)...);

            PluginPtr<Event> base_event = event_ptr;

            FireGlobalImpl(event_id, base_event);
        }

        // --- 2. 实例到实例 (Sender-Specific) ---

        /**
         * @brief [模板] 订阅一个特定发送者的事件。
         */
        template <typename TEvent, typename TSender, typename TSubscriber,
            typename TCallback>
        void SubscribeToSender(std::shared_ptr<TSender> sender,
            std::shared_ptr<TSubscriber> subscriber,
            TCallback&& callback,
            ConnectionType type = ConnectionType::kDirect) {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");
            static_assert(
                std::is_base_of_v<std::enable_shared_from_this<TSubscriber>,
                TSubscriber>,
                "Subscriber must inherit from std::enable_shared_from_this");

            EventId event_id = TEvent::kEventId;

            std::weak_ptr<TSubscriber> weak_sub = subscriber;

            std::function<void(const Event&)> wrapper =
                [weak_sub, cb = std::forward<TCallback>(callback)](const Event& e) {
                if (auto sub = weak_sub.lock()) {
                    (sub.get()->*cb)(static_cast<const TEvent&>(e));
                }
                };

            std::weak_ptr<void> weak_sub_id = subscriber;
            std::weak_ptr<void> weak_sender_id = sender;
            void* sender_key = sender.get();

            SubscribeToSenderImpl(sender_key, event_id, std::move(weak_sub_id),
                std::move(weak_sender_id), std::move(wrapper),
                type);
        }

        /**
         * @brief [模板] 向订阅了此发送者的订阅者发布事件。
         * [!! 修改 !!] 增加 IsSenderSubscribed 检查，实现条件式创建。
         */
        template <typename TEvent, typename TSender, typename... Args>
        void FireToSender(std::shared_ptr<TSender> sender, Args&&... args) {
            static_assert(std::is_base_of_v<Event, TEvent>,
                "TEvent must derive from z3y::Event");

            EventId event_id = TEvent::kEventId;
            void* sender_key = sender.get();

            // [!! 核心优化 !!] 检查是否有订阅者，如果没有则避免构造事件对象
            if (!IsSenderSubscribed(sender_key, event_id)) {
                return;
            }

            PluginPtr<TEvent> event_ptr =
                std::make_shared<TEvent>(std::forward<Args>(args)...);

            PluginPtr<Event> base_event = event_ptr;

            FireToSenderImpl(sender_key, event_id, base_event);
        }

        // --- 3. 手动生命周期管理 ---

        /**
         * @brief [可选] 立即取消此订阅者(subscriber)的所有订阅。
         */
        virtual void Unsubscribe(std::shared_ptr<void> subscriber) = 0;

    protected:

        /**
         * @internal [!! 新增 !!] 检查是否有全局订阅者。
         */
        virtual bool IsGlobalSubscribed(EventId event_id) = 0;

        /**
         * @internal [!! 新增 !!] 检查是否有特定发送者的订阅者。
         */
        virtual bool IsSenderSubscribed(void* sender_key, EventId event_id) = 0;

        /**
         * @internal
         */
        virtual void SubscribeGlobalImpl(EventId event_id,
            std::weak_ptr<void> sub,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) = 0;

        /**
         * @internal
         */
        virtual void FireGlobalImpl(EventId event_id,
            PluginPtr<Event> e_ptr) = 0;

        /**
         * @internal
         */
        virtual void SubscribeToSenderImpl(void* sender_key,
            EventId event_id,
            std::weak_ptr<void> sub_id,
            std::weak_ptr<void> sender_id,
            std::function<void(const Event&)> cb,
            ConnectionType connection_type) = 0;

        /**
         * @internal
         */
        virtual void FireToSenderImpl(void* sender_key,
            EventId event_id,
            PluginPtr<Event> e_ptr) = 0;
    };

    /**
     * @brief IEventBus 的全局唯一 ClassId。
     */
    namespace clsid {
        /**
         * @brief [修改] 框架核心事件总线服务的 "服务ID"。
         * 宿主程序使用此 ID 来 GetService。
         */
        constexpr ClassId kEventBus =
            ConstexprHash("z3y-core-event-bus-SERVICE-UUID-D54E82F1");
    }  // namespace clsid

}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_I_EVENT_BUS_H_