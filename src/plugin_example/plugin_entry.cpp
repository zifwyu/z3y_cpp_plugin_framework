/**
 * @file plugin_entry.cpp
 * @brief plugin_example 插件的入口点文件。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * 1. [修改]
 * 更新注册的类以使用
 * z3y::example
 * 命名空间
 * 2. [修改] [!!]
 * 移除所有组件
 * include
 * 和硬编码注册，
 * 改为执行自动注册列表。
 * 3. [修改] [!!]
 * 文件内容被简化为调用 Z3Y_DEFINE_PLUGIN_ENTRY
 * 宏。
 */

 // 
 // 
 // 
#include "framework/z3y_plugin_sdk.h"

// [!! 
// 核心逻辑 !!] 
// 
// 
Z3Y_DEFINE_PLUGIN_ENTRY