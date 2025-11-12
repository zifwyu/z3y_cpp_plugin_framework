/**
 * @file plugin_cast.h
 * @brief 定义 z3y::PluginCast 模板，用于安全的接口转换。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [v2.2 修复]:
 * 1. QueryInterfaceRaw
 * 的参数类型已从 std::type_index
 * 更改为 InterfaceID。
 * 2.
 * 相关的模板特化和实现也已相应更新。
 *
 * [修改]
 * 1. 遵从 Google 命名约定 (ClassId)
 * 2. 添加 class_id.h 头文件
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_CAST_H_
#define Z3Y_FRAMEWORK_PLUGIN_CAST_H_

#include "framework/i_component.h"
#include "framework/class_id.h"  // [新增]
#include <memory>                // 依赖 std::static_pointer_cast

namespace z3y {
    // --- 内部实现 (对用户隐藏) ---
    namespace internal {
        /**
         * @brief [内部] PluginCast 的核心实现。
         *
         * @tparam T 目标接口类型 (例如 ISimple)。
         * @param[in] component
         * 源 IComponent 智能指针。
         * @return 转换后的目标接口智能指针，
         * 如果失败则为 nullptr。
         */
        template <typename T>
        PluginPtr<T> PluginCastImpl(PluginPtr<IComponent> component) {
            if (!component) {
                return nullptr;
            }

            // 1. [核心] 调用虚函数表 (v-table)
            //    进行手动的运行时类型识别
            //    (RTTI)
            void* interface_ptr = component->QueryInterfaceRaw(T::kIid);

            if (!interface_ptr) {
                // 2. 转换失败：
                //    该组件不支持此接口 (T::kIid)
                return nullptr;
            }

            // 3. 转换成功：
            //    使用 aliasing
            //    constructor
            //    (别名构造函数)
            //    创建一个新的
            //    shared_ptr。
            //
            //    @design
            //    我们 *不能* 使用
            //    std::static_pointer_cast，
            //    因为它依赖于 C++ RTTI，
            //    在跨模块时可能失败。
            //    我们也 *不能*
            //    返回
            //    PluginPtr<T>(static_cast<T*>(interface_ptr))，
            //    因为这会创建一个新的控制块，
            //    导致“重复释放”
            //    (double-free) 错误。
            //
            //    aliasing constructor
            //    是唯一的正确选择：
            //    它共享 'component'
            //    的引用计数（生命周期），
            //    但指向 'interface_ptr'
            //    （转换后的地址）。
            return PluginPtr<T>(component, static_cast<T*>(interface_ptr));
        }

    }  // namespace internal


    /**
     * @brief [框架核心工具] 在框架组件之间进行安全的动态类型转换。
     *
     * @design
     * 它的作用类似于 dynamic_cast，
     * 但是是基于我们手动的
     * IComponent::QueryInterfaceRaw
     * 虚函数实现的，
     * 从而确保在跨 DLL/SO 边界时 100% 可靠。
     *
     * @tparam T
     * 您希望转换到的目标 *接口* 类型 (例如
     * ISimple)。
     * @param[in] component
     * 一个指向源组件的 PluginPtr (通常是
     * PluginPtr<IComponent>)。
     * @return
     * 转换后的 PluginPtr<T>。如果转换失败，
     * 则返回 nullptr。
     */
    template <typename T>
    PluginPtr<T> PluginCast(PluginPtr<IComponent> component) {
        // 特化：如果目标是 IComponent
        // 本身，
        // 直接返回
        if constexpr (std::is_same_v<T, IComponent>) {
            return component;
        }
        else {
            return internal::PluginCastImpl<T>(component);
        }
    }

    /**
     * @brief [框架核心工具]
     * PluginCast 的重载，
     * 允许从一个具体接口转换到另一个接口。
     * (例如 PluginCast<ISimple2>(my_simple1_ptr))
     *
     * @tparam T
     * 您希望转换到的目标 *接口* 类型 (例如
     * ISimple2)。
     * @tparam U
     * 您持有的源 *接口* 类型 (例如 ISimple)。
     * @param[in] component_interface
     * 一个指向源接口的 PluginPtr。
     * @return
     * 转换后的 PluginPtr<T>。如果转换失败，
     * 则返回 nullptr。
     */
    template <typename T, typename U>
    PluginPtr<T> PluginCast(PluginPtr<U> component_interface) {
        // 1. [安全]
        //    首先通过 static_cast
        //    安全地转换回 IComponent
        //    基类。
        PluginPtr<IComponent> base_component =
            std::static_pointer_cast<IComponent>(component_interface);

        // 2. 调用标准实现
        return PluginCast<T>(base_component);
    }

}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_PLUGIN_CAST_H_