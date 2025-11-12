/**
 * @file plugin_cast.h
 * @brief 定义 z3y::PluginCast 模板，用于安全的接口转换。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [v2.2 修复]:
 * ...
 * [修改]
 * 1. 遵从 Google 命名约定 (ClassId)
 * 2. 添加 class_id.h 头文件
 * 3. [修改]
 * PluginCastImpl
 * 现在传递 T::kVersionMajor
 * 和 T::kVersionMinor
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
         * [修改]
         * 现在调用 QueryInterfaceRaw(iid,
         * T::kVersionMajor, T::kVersionMinor)
         */
        template <typename T>
        PluginPtr<T> PluginCastImpl(PluginPtr<IComponent> component) {
            if (!component) {
                return nullptr;
            }

            // 1. [核心] 
            //    [修改] 
            //    传递宿主编译时所知的 
            //    Major 
            //    和 Minor 
            //    版本
            void* interface_ptr = component->QueryInterfaceRaw(
                T::kIid, T::kVersionMajor, T::kVersionMinor);

            if (!interface_ptr) {
                // 2. 
                //    转换失败 (
                //    IID 
                //    不匹配或版本不兼容
                //    )
                return nullptr;
            }

            // 3. 转换成功：
            //    使用 aliasing
            //    constructor
            //    ... (
            //    逻辑不变
            //    )
            return PluginPtr<T>(component, static_cast<T*>(interface_ptr));
        }

    }  // namespace internal


    /**
     * @brief [框架核心工具] 在框架组件之间进行安全的动态类型转换。
     * ...
     */
    template <typename T>
    PluginPtr<T> PluginCast(PluginPtr<IComponent> component) {
        // [修改] 
        // 即使是 IComponent, 
        // 也必须通过 Impl 
        // 进行版本检查
        if constexpr (std::is_same_v<T, IComponent>) {
            return internal::PluginCastImpl<T>(component);
        }
        else {
            return internal::PluginCastImpl<T>(component);
        }
    }

    /**
     * @brief [框架核心工具]
     * PluginCast 的重载 ...
     */
    template <typename T, typename U>
    PluginPtr<T> PluginCast(PluginPtr<U> component_interface) {
        // 1. 
        //    (
        //    安全地转换回 IComponent)
        PluginPtr<IComponent> base_component =
            std::static_pointer_cast<IComponent>(component_interface);

        // 2. 调用标准实现
        return PluginCast<T>(base_component);
    }

}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_PLUGIN_CAST_H_