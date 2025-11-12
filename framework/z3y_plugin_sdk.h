/**
 * @file z3y_plugin_sdk.h
 * @brief z3y 插件框架 - 插件开发工具包 (SDK)
 * @details
 * ...
 * [修改]
 * 1.
 * 添加了新的辅助宏头文件
 * (interface_helpers.h,
 * component_helpers.h, event_helpers.h)
 */

#pragma once

#ifndef Z3Y_PLUGIN_SDK_H_
#define Z3Y_PLUGIN_SDK_H_

 // 1. 核心类型 (实现插件必须)
#include "framework/i_component.h"
#include "framework/class_id.h"
#include "framework/plugin_impl.h"

// 2. 注册 (实现 plugin_entry 必须)
#include "framework/i_plugin_registry.h"
#include "framework/plugin_registration.h"

// 3. [!! 新增 !!] 
//    辅助宏 (
//    极大提升易用性
//    )
#include "framework/interface_helpers.h"  // 
                                          // 
                                          // 
                                          // 提供 Z3Y_DEFINE_INTERFACE
#include "framework/component_helpers.h"  // 
                                          // 
                                          // 
                                          // 提供 Z3Y_DEFINE_COMPONENT_ID
#include "framework/event_helpers.h"      // 
                                          // 
                                          // 
                                          // 提供 Z3Y_DEFINE_EVENT

// 4. 
//    可选的框架服务 (为复杂插件提供)
#include "framework/plugin_cast.h"
#include "framework/i_event_bus.h"
#include "framework/connection_type.h"

#endif // Z3Y_PLUGIN_SDK_H_