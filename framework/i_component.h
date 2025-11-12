/**
 * @file i_component.h
 * @brief 定义插件框架的统一组件基类 IComponent 和标准智能指针 PluginPtr。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 * [修改]
 * 1.
 * 遵从 Google 命名约定
 * 2.
 * 使用 Z3Y_DEFINE_INTERFACE
 * 宏
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_COMPONENT_H_
#define Z3Y_FRAMEWORK_I_COMPONENT_H_

#include "framework/class_id.h"
#include "framework/interface_helpers.h" // [新]
#include <memory>
#include <typeindex>

namespace z3y {
    /**
     * @typedef PluginPtr
     * @brief 框架中用于管理所有插件对象生命周期的标准智能指针。
     */
    template <typename T>
    using PluginPtr = std::shared_ptr<T>;


    /**
     * @class IComponent
     * @brief 框架中所有“接口”和“实现”都必须继承的 *唯一* 统一基类。
     */
    class IComponent {
    public:
        /**
         * @brief [修改]
         * 使用 Z3Y_DEFINE_INTERFACE
         * 宏
         */
        Z3Y_DEFINE_INTERFACE(IComponent, "z3y-core-IComponent-IID-A0000001")

            /**
             * @brief 虚析构函数（默认实现）。
             */
            virtual ~IComponent() = default;

        /**
         * @brief [核心] 手动运行时类型识别（RTTI）的查询函数（纯虚函数）。
         *
         * [修改]
         * 参数从 ClassId
         * 更改为 InterfaceId
         */
        virtual void* QueryInterfaceRaw(InterfaceId iid) = 0;
    };


}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_I_COMPONENT_H_