/**
 * @file z3y_framework.h
 * @brief z3y 插件框架 - 宿主 (Host) API
 * @details
 * 宿主程序 (插件的使用者)
 * 只需要包含这一个头文件
 * 就可以使用框架的所有核心功能。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_H_
#define Z3Y_FRAMEWORK_H_

 // 1. 核心的 PluginManager 实例
#include "z3y_plugin_manager/plugin_manager.h" 

// 2. 核心类型
#include "framework/i_component.h"    // 提供 PluginPtr
#include "framework/class_id.h"       // 提供 ClassID

// 3. 核心工具
#include "framework/plugin_cast.h"    // 提供 PluginCast

// 4. 事件系统
#include "framework/i_event_bus.h"      // 提供 IEventBus
#include "framework/connection_type.h"// IEventBus 依赖
#include "framework_events.h"         // 框架标准事件

#endif // Z3Y_FRAMEWORK_H_