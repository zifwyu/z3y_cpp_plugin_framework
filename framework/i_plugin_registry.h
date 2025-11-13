/**
 * @file i_plugin_registry.h
 * @brief 定义 z3y::IPluginRegistry 接口，用于插件注册。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]：
 * 1. [修改]
 * RegisterComponent
 * 增加
 * implemented_interfaces
 * 参数
 * 类型从 vector<InterfaceId>
 * 变为
 * vector<InterfaceDetails>
 * 2. 遵从 Google 命名约定
 * 3. [修改] [!!]
 * RegisterComponent
 * 增加
 * "bool is_default"
 * 标记
 * 4. [修改] [!!]
 * 定义 Z3Y_PLUGIN_API
 * 宏
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_PLUGIN_REGISTRY_H_
#define Z3Y_FRAMEWORK_I_PLUGIN_REGISTRY_H_

#include "framework/class_id.h"
#include "framework/i_component.h"
#include "framework/i_plugin_query.h" // [新] 依赖 InterfaceDetails
#include <functional>
#include <string>
#include <vector>

namespace z3y {

    // --- [!! 
    // 新增 !!] 
    // 
    // 
    // 
#ifdef _WIN32
#define Z3Y_PLUGIN_API __declspec(dllexport)
#else
#define Z3Y_PLUGIN_API __attribute__((visibility("default")))
#endif

/**
 * @typedef FactoryFunction
 * @brief 定义一个“工厂函数”的标准签名。
 */
    using FactoryFunction = std::function<PluginPtr<IComponent>()>;

    /**
     * @class IPluginRegistry
     * @brief [框架核心] 插件注册表接口。
     */
    class IPluginRegistry {
    public:
        /**
         * @brief 虚析构函数。
         */
        virtual ~IPluginRegistry() = default;

        /**
         * @brief 插件调用此函数来注册一个组件类。
         * [修改]
         * 参数类型改为
         * std::vector<InterfaceDetails>
         * [修改] [!!]
         * 增加 is_default
         * 参数
         *
         * @param[in] clsid 类的唯一ID (uint64_t)。
         * @param[in] factory 一个 lambda，用于创建该类的新实例。
         * @param[in] is_singleton
         * - false: 注册为普通组件 ("工具")。
         * - true: 注册为“单例服务” ("公共设施")。
         * @param[in] alias 一个可选的、人类可读的字符串别名。
         * @param[in] implemented_interfaces [修改]
         * 此组件实现的所有接口详情 (Id
         * 和 Name)
         * 列表。
         * @param[in] is_default [!!
         * 新增 !!]
         * 如果为 true,
         * 将此组件注册为所有
         * *
         * 非 IComponent
         * * * 接口的
         * *
         * 默认
         * *
         * 实现。
         */
        virtual void RegisterComponent(
            ClassId clsid, FactoryFunction factory, bool is_singleton,
            const std::string& alias,
            std::vector<InterfaceDetails> implemented_interfaces,
            bool is_default = false) = 0; // [!! 
        // 新增 !!]
    };

}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_I_PLUGIN_REGISTRY_H_