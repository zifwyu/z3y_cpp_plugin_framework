/**
 * @file framework_events.h
 * @brief 定义框架内部用于调试和日志记录的事件。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. ComponentRegisterEvent 现在包含 is_singleton 成员，
 * 以便日志记录器可以区分组件和服务。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_FRAMEWORK_EVENTS_H_
#define Z3Y_FRAMEWORK_FRAMEWORK_EVENTS_H_

#include "i_event_bus.h"
#include "class_id.h"
#include <string>

namespace z3y
{
    namespace event
    {
        /**
         * @struct PluginLoadSuccessEvent
         * @brief 当一个插件DLL被成功加载并初始化时触发。
         */
        struct PluginLoadSuccessEvent : public Event
        {
            std::string plugin_path; //!< 被加载的 DLL 的完整路径

            /**
             * @brief 构造函数
             */
            explicit PluginLoadSuccessEvent(std::string path)
                : plugin_path(std::move(path))
            {
            }
        };

        /**
         * @struct PluginLoadFailureEvent
         * @brief 当一个插件DLL加载失败时触发。
         */
        struct PluginLoadFailureEvent : public Event
        {
            std::string plugin_path;     //!< 尝试加载的 DLL 的路径
            std::string error_message;   //!< 失败原因

            /**
             * @brief 构造函数
             */
            PluginLoadFailureEvent(std::string path, std::string error)
                : plugin_path(std::move(path)), error_message(std::move(error))
            {
            }
        };

        /**
         * @struct ComponentRegisterEvent
         * @brief 当一个组件/服务被成功注册时触发。
         * [已修正]：新增了 is_singleton 成员。
         */
        struct ComponentRegisterEvent : public Event
        {
            ClassID clsid;               //!< 注册的 ClassID (uint64_t)
            std::string alias;           //!< 注册的字符串别名 (如果有)
            std::string plugin_path;     //!< 此组件来自哪个 DLL
            bool is_singleton;           //!< [新增] 是单例服务 (true) 还是普通组件 (false)

            /**
             * @brief 构造函数
             */
            ComponentRegisterEvent(ClassID id, std::string a, std::string path, bool singleton)
                : clsid(id), alias(std::move(a)), plugin_path(std::move(path)), is_singleton(singleton)
            {
            }
        };

    } // namespace event
} // namespace z3y

#endif // Z3Y_FRAMEWORK_FRAMEWORK_EVENTS_H_