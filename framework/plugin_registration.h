/**
 * @file plugin_registration.h
 * @brief 定义 z3y::RegisterComponent 和 z3y::RegisterService 模板辅助函数。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. RegisterComponent 和 RegisterService 现在只调用
 * 一次 IPluginRegistry::RegisterComponent，并传入所有参数。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_REGISTRATION_H_
#define Z3Y_FRAMEWORK_PLUGIN_REGISTRATION_H_

#include "i_plugin_registry.h"
#include "plugin_impl.h" // 依赖 PluginImpl 以获取 ImplClass::kClsid
#include <memory>        // 依赖 std::make_shared
#include <string>        // 依赖 std::string

namespace z3y
{
    /**
     * @brief [框架便利工具] 自动注册一个“普通组件”(瞬态)。
     *
     * @tparam ImplClass 要注册的具体实现类。
     * @param[in] registry 宿主传入的 IPluginRegistry 指针。
     * @param[in] alias 一个可选的、人类可读的字符串别名。
     */
    template <typename ImplClass>
    void RegisterComponent(IPluginRegistry* registry, const std::string& alias = "")
    {
        // 1. 自动生成工厂 lambda
        FactoryFunction factory = []() -> PluginPtr<IComponent>
            {
                return std::make_shared<ImplClass>();
            };

        // 2. [修正]：一次性调用
        registry->RegisterComponent(
            ImplClass::kClsid,
            factory,
            false,              // is_singleton = false
            alias
        );
    }

    /**
     * @brief [框架便利工具] 自动注册一个“单例服务”。
     *
     * @tparam ImplClass 要注册的具体实现类。
     * @param[in] registry 宿主传入的 IPluginRegistry 指针。
     * @param[in] alias 一个可选的、人类可读的字符串别名。
     */
    template <typename ImplClass>
    void RegisterService(IPluginRegistry* registry, const std::string& alias = "")
    {
        // 1. 自动生成工厂 lambda
        FactoryFunction factory = []() -> PluginPtr<IComponent>
            {
                return std::make_shared<ImplClass>();
            };

        // 2. [修正]：一次性调用
        registry->RegisterComponent(
            ImplClass::kClsid,
            factory,
            true,               // is_singleton = true
            alias
        );
    }

} // namespace z3y

#endif // Z3Y_FRAMEWORK_PLUGIN_REGISTRATION_H_