/**
 * @file z3y_plugin_sdk.h
 * @brief z3y 插件框架 - 插件开发工具包 (SDK)
 * @details
 * 插件开发者只需要包含这一个头文件，
 * 即可定义接口、实现类、使用辅助宏和自动注册。
 * [修改] [!!]
 * 最终归一化，
 * 成为插件开发者的
 * * * * * 头文件。
 */

#pragma once

#ifndef Z3Y_PLUGIN_SDK_H_
#define Z3Y_PLUGIN_SDK_H_

 // 1. 核心类型 (IComponent 
 // / ClassId / PluginImpl)
#include "framework/i_component.h"
#include "framework/class_id.h"
#include "framework/plugin_impl.h"

// 2. 宏辅助和接口定义 (
// Z3Y_DEFINE_INTERFACE 
// )
#include "framework/interface_helpers.h"
#include "framework/component_helpers.h"
#include "framework/event_helpers.h"

// 3. 注册 (
// IPluginRegistry / Registration Helpers)
#include "framework/i_plugin_registry.h"
#include "framework/plugin_registration.h"
#include "framework/auto_registration.h" // [!! 
                                         // 核心 !!] 
                                         // 
                                         // 

// 4. 可选的框架服务 (
// IEventBus / PluginCast
// )
#include "framework/plugin_cast.h"
#include "framework/i_event_bus.h"
#include "framework/connection_type.h"
#include "framework/plugin_exceptions.h" 

// 5. [!! 新增 !!] 全局服务定位器 (易用性优化)
// (允许插件A 轻松调用插件B 提供的服务)
#include "framework/z3y_service_locator.h"

#endif // Z3Y_PLUGIN_SDK_H_