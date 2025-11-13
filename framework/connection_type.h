/**
 * @file connection_type.h
 * @brief 定义 z3y::ConnectionType 枚举。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 此文件定义了事件订阅的连接方式，
 * 允许订阅者选择回调函数是在发布者线程上同步执行，
 * 还是在框架的工作线程上异步执行。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_CONNECTION_TYPE_H_
#define Z3Y_FRAMEWORK_CONNECTION_TYPE_H_

namespace z3y
{

    /**
     * @enum ConnectionType
     * @brief 定义事件订阅的连接方式。
     */
    enum class ConnectionType
    {
        /**
         * @brief 直接连接 (同步)。
         * 回调函数在发布者 (FireGlobal/FireToSender)
         * 所在的线程上立即执行。
         * 如果回调函数阻塞，发布者也会被阻塞。
         * 这是默认选项。
         */
        kDirect,

        /**
         * @brief 队列连接 (异步)。
         * 事件被放入一个线程安全的队列，由 PluginManager
         * 拥有的一个专用工作线程稍后执行。
         * FireGlobal/FireToSender 会立即返回，发布者不会被阻塞。
         */
        kQueued
    };

} // namespace z3y

#endif // Z3Y_FRAMEWORK_CONNECTION_TYPE_H_