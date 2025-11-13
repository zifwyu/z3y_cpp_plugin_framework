/**
 * @file framework_events.h
 * @brief 定义 z3y 框架的核心内部事件。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 * [修改]
 * 1.
 * 遵从 Google 命名约定
 * 2.
 * 添加 class_id.h
 * 头文件
 * 3. [修改]
 * 使用 Z3Y_DEFINE_EVENT
 * 宏
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_FRAMEWORK_EVENTS_H_
#define Z3Y_FRAMEWORK_FRAMEWORK_EVENTS_H_

#include "framework/i_event_bus.h"
#include "framework/class_id.h"
#include "framework/event_helpers.h" // [新]
#include <string>

namespace z3y {
    namespace event {
        // --- 1. 插件加载事件 ---

        /**
         * @struct PluginLoadSuccessEvent
         * @brief [事件] 当一个 DLL/SO 插件被成功加载和初始化时触发。
         */
        struct PluginLoadSuccessEvent : public Event {
            /**
             * @brief [修改]
             * 使用 Z3Y_DEFINE_EVENT
             * 宏
             */
            Z3Y_DEFINE_EVENT(PluginLoadSuccessEvent,
                "z3y-event-plugin-load-success-E0000001")

                std::string plugin_path_;  // [修改]

            explicit PluginLoadSuccessEvent(std::string path)
                : plugin_path_(std::move(path)) {
            }  // [修改]
        };

        /**
         * @struct PluginLoadFailureEvent
         * @brief [事件] 当一个 DLL/SO 插件加载或初始化失败时触发。
         */
        struct PluginLoadFailureEvent : public Event {
            /**
             * @brief [修改]
             * 使用 Z3Y_DEFINE_EVENT
             * 宏
             */
            Z3Y_DEFINE_EVENT(PluginLoadFailureEvent,
                "z3y-event-plugin-load-failure-E0000002")

                std::string plugin_path_;  // [修改]
            std::string error_message_;  // [修改]

            PluginLoadFailureEvent(std::string path, std::string error)
                : plugin_path_(std::move(path)),
                error_message_(std::move(error)) {
            }  // [修改]
        };


        // --- 2. 组件注册事件 ---

        /**
         * @struct ComponentRegisterEvent
         * @brief [事件] 当一个组件 (普通或单例)
         * 被注册到 PluginManager 时触发。
         */
        struct ComponentRegisterEvent : public Event {
            /**
             * @brief [修改]
             * 使用 Z3Y_DEFINE_EVENT
             * 宏
             */
            Z3Y_DEFINE_EVENT(ComponentRegisterEvent,
                "z3y-event-component-register-E0000003")

                ClassId clsid_;          // [修改]
            std::string alias_;      // [修改]
            std::string plugin_path_;  // [修改]
            bool is_singleton_;  // [修改]

            /**
             * @brief 构造函数
             */
            ComponentRegisterEvent(ClassId id, const std::string& a,
                const std::string& path, bool singleton)
                : clsid_(id),
                alias_(a),
                plugin_path_(path),
                is_singleton_(singleton) {
            }  // [修改]
        };


        // --- 3. 异步异常事件 ---

        /**
         * @struct AsyncExceptionEvent
         * @brief [事件]
         * 当一个 kQueued
         * (异步)
         * 事件回调在工作线程中抛出异常时触发。
         */
        struct AsyncExceptionEvent : public Event {
            /**
             * @brief [修改]
             * 使用 Z3Y_DEFINE_EVENT
             * 宏
             */
            Z3Y_DEFINE_EVENT(AsyncExceptionEvent,
                "z3y-event-async-exception-E0000004")

                std::string error_message_;  // [修改]

            explicit AsyncExceptionEvent(std::string error)
                : error_message_(std::move(error)) {
            }  // [修改]
        };

    }  // namespace event
}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_FRAMEWORK_EVENTS_H_