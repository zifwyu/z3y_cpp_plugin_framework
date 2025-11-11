/**
 * @file z3y_plugin_sdk.h
 * @brief z3y 插件框架 - 插件开发工具包 (SDK)
 * @details
 * 插件开发者 (插件的实现者)
 * 只需要包含这一个头文件
 * 就可以实现和注册一个插件。
 */

#pragma once

#ifndef Z3Y_PLUGIN_SDK_H_
#define Z3Y_PLUGIN_SDK_H_

 // 1. 核心类型 (实现插件必须)
#include "framework/i_component.h"    //
#include "framework/class_id.h"       //
#include "framework/plugin_impl.h"    // 
                                      //      提供 PluginImpl

// 2. 注册 (实现 plugin_entry 必须)
#include "framework/i_plugin_registry.h"      // 
                                              //      提供 z3yPluginInit 
                                              //      参数
#include "framework/plugin_registration.h"  // 
                                              //      提供 RegisterComponent

// 3. 
//    可选的框架服务 (为复杂插件提供)
#include "framework/plugin_cast.h"    // 
                                      //      (插件可能需要获取其他服务)
#include "framework/i_event_bus.h"      // 
                                      //      (插件可能需要收发事件)
#include "framework/connection_type.h"// 
                                      //      (IEventBus 依赖)

#endif // Z3Y_PLUGIN_SDK_H_