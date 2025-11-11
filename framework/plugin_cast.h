/**
 * @file plugin_cast.h
 * @brief 定义 z3y::PluginCast<T>() 模板函数。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 此文件提供了 z3y::PluginCast，这是一个跨模块、类型安全、
 * 且自动管理生命周期的接口转换函数。
 *
 * @design
 * 它取代了 std::dynamic_pointer_cast。
 * 它通过 IComponent::GetComponentBase() 获取“生命周期控制器” (shared_ptr)，
 * 然后通过 ComponentBase::QueryInterfaceRaw() 获取“目标接口指针” (void*)，
 * 最后使用 std::shared_ptr 的“别名构造函数”将两者合并。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_CAST_H_
#define Z3Y_FRAMEWORK_PLUGIN_CAST_H_

#include "i_component.h"  // 依赖 z3y::IComponent, z3y::ComponentBase, 和 z3y::PluginPtr

namespace z3y
{

    /**
     * @brief [框架核心] 跨模块、类型安全、自动管理生命周期的接口转换函数。
     *
     * @tparam To 目标接口类型 (例如 ISimple2)。
     * @tparam From 源接口类型 (例如 ISimple)。
     * @param[in] from 源接口的智能指针 (PluginPtr<From>)。
     * @return 转换成功则返回 PluginPtr<To>，失败则返回 nullptr。
     */
    template <typename To, typename From>
    PluginPtr<To> PluginCast(PluginPtr<From> from)
    {
        // 编译期检查：确保 To 和 From 都是 IComponent 的子类
        static_assert(std::is_base_of_v<IComponent, To>,
            "Target type 'To' must inherit from z3y::IComponent.");
        static_assert(std::is_base_of_v<IComponent, From>,
            "Source type 'From' must inherit from z3y::IComponent.");

        // 1. 空指针检查
        if (!from)
        {
            return nullptr;
        }

        // 2. 尝试C++原生转换 (在同一模块内会成功)
        if (auto p = std::dynamic_pointer_cast<To>(from))
        {
            return p;
        }

        // 3. (原生转换失败) 开始执行我们的“手动跨模块”协议

        // 4. 获取“生命周期控制器”
        PluginPtr<ComponentBase> base_impl = from->GetComponentBase();
        if (!base_impl)
        {
            return nullptr;
        }

        // 5. 获取“目标接口的原始指针”
        void* raw_iface_ptr = base_impl->QueryInterfaceRaw(std::type_index(typeid(To)));
        if (!raw_iface_ptr)
        {
            // 实现类不支持 'To' 接口，转换失败。
            return nullptr;
        }

        // 6. 合并“生命周期”和“接口指针”
        //
        // 关键一步：使用 std::shared_ptr 的“别名构造函数” (Aliasing Constructor)。
        return PluginPtr<To>(base_impl, static_cast<To*>(raw_iface_ptr));
    }

} // namespace z3y

#endif // Z3Y_FRAMEWORK_PLUGIN_CAST_H_