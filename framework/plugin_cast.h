/**
 * @file plugin_cast.h
 * @brief 定义 z3y::PluginCast 模板，用于安全的接口转换。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [v2.2 修复]:
 * ...
 * 4. [修改] [!!]
 * PluginCastImpl
 * 现在接受并传递
 * InstanceError&
 * out_result
 * 5. [修改] [!!]
 * 移除了已废弃的 (
 * 不带 error
 * )
 * 的
 * PluginCast
 * 版本
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_CAST_H_
#define Z3Y_FRAMEWORK_PLUGIN_CAST_H_

#include "framework/i_component.h"
#include "framework/class_id.h"  // [新增]
#include "framework/plugin_exceptions.h"  // [!! 
 // 新增 !!]
#include <memory>                // 依赖 std::static_pointer_cast

namespace z3y {
    // --- 内部实现 (对用户隐藏) ---
    namespace internal {
        /**
         * @brief [内部] PluginCast 的核心实现。
         *
         * [修改] [!!]
         * 现在调用
         * QueryInterfaceRaw(...,
         * out_result)
         */
        template <typename T>
        PluginPtr<T> PluginCastImpl(PluginPtr<IComponent> component,
            InstanceError& out_result) { // [!! 
            // 修改 !!]
            if (!component) {
                // 
                // (
                // 这种情况理论上应由
                // PluginManager
                // 捕获
                // )
                out_result = InstanceError::kErrorInternal; // [!! 
                // 修改 !!]
                return nullptr;
            }

            // 1. [核心] 
            //    [修改] 
            //    传递版本并接收
            //    out_result
            void* interface_ptr = component->QueryInterfaceRaw(
                T::kIid, T::kVersionMajor, T::kVersionMinor,
                out_result); // [!! 
            // 修改 !!]

            if (!interface_ptr) {
                // 2. 
                //    转换失败
                //    (out_result
                //    已由 QueryInterfaceRaw
                //    设置
                //    )
                return nullptr;
            }

            // 3. 
            //    转换成功
            //    (out_result
            //    已由 QueryInterfaceRaw
            //    设置为 kSuccess
            //    )
            return PluginPtr<T>(component, static_cast<T*>(interface_ptr));
        }

    }  // namespace internal


    /**
     * @brief [框架核心工具] 在框架组件之间进行安全的动态类型转换。
     * [!!
     * 修改 !!]
     * 此版本现在是内部 API
     * ，
     * 被 PluginManager
     * 调用
     */
    template <typename T>
    PluginPtr<T> PluginCast(PluginPtr<IComponent> component,
        InstanceError& out_result) { // [!! 
        // 修改 !!]
// [修改] 
// 即使是 IComponent, 
// 也必须通过 Impl 
// 进行版本检查
        if constexpr (std::is_same_v<T, IComponent>) {
            return internal::PluginCastImpl<T>(component, out_result);
        }
        else {
            return internal::PluginCastImpl<T>(component, out_result);
        }
    }

    /**
     * @brief [框架核心工具]
     * PluginCast
     * 的重载
     * [!!
     * 修改 !!]
     * 此版本现在是内部 API
     * ，
     * 被 PluginManager
     * 调用
     */
    template <typename T, typename U>
    PluginPtr<T> PluginCast(PluginPtr<U> component_interface,
        InstanceError& out_result) { // [!! 
        // 修改 !!]
// 1. 
//    (
//    安全地转换回 IComponent)
        PluginPtr<IComponent> base_component =
            std::static_pointer_cast<IComponent>(component_interface);

        // 2. 
        //    调用标准实现
        return PluginCast<T>(base_component, out_result);
    }

}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_PLUGIN_CAST_H_