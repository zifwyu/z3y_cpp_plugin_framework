/**
 * @file i_plugin_registry.h
 * @brief 定义 z3y::IPluginRegistry 接口，用于插件注册。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [已修正]：
 * 1. RegisterComponent 现在原子化地接受 clsid, factory, is_singleton 和 alias。
 * 2. 移除了 RegisterAlias 接口，因为它已合并到 RegisterComponent 中。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_PLUGIN_REGISTRY_H_
#define Z3Y_FRAMEWORK_I_PLUGIN_REGISTRY_H_

#include "class_id.h"
#include "i_component.h"
#include <functional>
#include <string>

namespace z3y
{

    /**
     * @typedef FactoryFunction
     * @brief 定义一个“工厂函数”的标准签名。
     */
    using FactoryFunction = std::function<PluginPtr<IComponent>()>;

    /**
     * @class IPluginRegistry
     * @brief [框架核心] 插件注册表接口。
     */
    class IPluginRegistry
    {
    public:
        /**
         * @brief 虚析构函数。
         */
        virtual ~IPluginRegistry() = default;

        /**
         * @brief 插件调用此函数来注册一个组件类。
         *
         * @param[in] clsid 类的唯一ID (uint64_t)。
         * @param[in] factory 一个 lambda，用于创建该类的新实例。
         * @param[in] is_singleton
         * - false: 注册为普通组件 ("工具")。
         * - true: 注册为“单例服务” ("公共设施")。
         * @param[in] alias 一个可选的、人类可读的字符串别名。
         * 如果为空，则不注册别名。
         */
        virtual void RegisterComponent(ClassID clsid,
            FactoryFunction factory,
            bool is_singleton,
            const std::string& alias) = 0;
    };

} // namespace z3y

#endif // Z3Y_FRAMEWORK_I_PLUGIN_REGISTRY_H_