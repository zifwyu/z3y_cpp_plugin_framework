/**
 * @file plugin_registration.h
 * @brief 定义 z3y::RegisterComponent 和 z3y::RegisterService 模板辅助函数。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]：
 * 1.
 * RegisterComponent/RegisterService
 * [修改]
 * 调用 GetInterfaceDetails()
 * 并传递 InterfaceDetails
 * 列表。
 * 2. 遵从 Google 命名约定
 * 3. [修改] [!!]
 * 增加 "bool is_default"
 * 参数
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_REGISTRATION_H_
#define Z3Y_FRAMEWORK_PLUGIN_REGISTRATION_H_

#include "framework/i_plugin_registry.h"
#include "framework/plugin_impl.h"  // 
 // [修改] 
 // 依赖 PluginImpl 
 // 以获取 ImplClass::kClsid 
 // 和 GetInterfaceDetails
#include <memory>                   // 依赖 std::make_shared
#include <string>                   // 依赖 std::string
#include <vector>

namespace z3y {
    /**
     * @brief [框架便利工具] 自动注册一个“普通组件”(瞬态)。
     *
     * @tparam ImplClass 要注册的具体实现类。
     * @param[in] registry 宿主传入的 IPluginRegistry 指针。
     * @param[in] alias 一个可选的、人类可读的字符串别名。
     * @param[in] is_default [!!
     * 新增 !!]
     * 是否注册为默认实现。
     */
    template <typename ImplClass>
    void RegisterComponent(IPluginRegistry* registry,
        const std::string& alias = "",
        bool is_default = false) { // [!! 
        // 新增 !!]
// 1. 自动生成工厂 lambda
        FactoryFunction factory = []() -> PluginPtr<IComponent> {
            return std::make_shared<ImplClass>();
            };

        // 2. [修改]：
        // 一次性调用，
        // 并传递 InterfaceDetails 
        // 列表
        registry->RegisterComponent(ImplClass::kClsid, std::move(factory),
            false,  // is_singleton = false
            alias,
            ImplClass::GetInterfaceDetails(),  // [修改]
            is_default // [!! 
                       // 新增 !!]
        );
    }

    /**
     * @brief [框架便利工具] 自动注册一个“单例服务”。
     *
     * @tparam ImplClass 要注册的具体实现类。
     * @param[in] registry 宿主传入的 IPluginRegistry 指针。
     * @param[in] alias 一个可选的、人类可读的字符串别名。
     * @param[in] is_default [!!
     * 新增 !!]
     * 是否注册为默认实现。
     */
    template <typename ImplClass>
    void RegisterService(IPluginRegistry* registry, const std::string& alias = "",
        bool is_default = false) { // [!! 
        // 新增 !!]
// 1. 自动生成工厂 lambda
        FactoryFunction factory = []() -> PluginPtr<IComponent> {
            return std::make_shared<ImplClass>();
            };

        // 2. [修改]：
        // 一次性调用，
        // 并传递 InterfaceDetails 
        // 列表
        registry->RegisterComponent(ImplClass::kClsid, std::move(factory),
            true,  // is_singleton = true
            alias,
            ImplClass::GetInterfaceDetails(),  // [修改]
            is_default // [!! 
                       // 新增 !!]
        );
    }

}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_PLUGIN_REGISTRATION_H_