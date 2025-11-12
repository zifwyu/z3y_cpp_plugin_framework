/**
 * @file plugin_impl.h
 * @brief 定义 z3y::PluginImpl 模板助手类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [v2.2 修复]:
 * 1. QueryInterfaceRaw
 * 的签名和实现已更新，
 * 使用 InterfaceID
 * 别名替换 ClassID。
 * 2. 确保类定义末尾的分号存在。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_IMPL_H_
#define Z3Y_FRAMEWORK_PLUGIN_IMPL_H_

#include "i_component.h"  // 依赖 z3y::IComponent
#include "class_id.h"     // 依赖 z3y::ClassID
#include <type_traits>    // 依赖 std::is_base_of_v (C++17)
#include <memory>         // 依赖 std::enable_shared_from_this

namespace z3y
{
    /**
     * @class PluginImpl
     * @brief [框架核心] 插件实现类的模板助手 (使用 CRTP 模式)。
     *
     * @tparam ImplClass
     * 最终的实现类名 (例如 SimpleImpl)。
     * @tparam kClsidParam
     * 此实现类对应的 ClassID (例如 clsid::kCSimple)。
     * @tparam Interfaces...
     * 该类所实现的所有 z3y::IComponent
     * 接口
     * (例如 ISimple, ISimple2)。
     */
    template <typename ImplClass,
        ClassID kClsidParam,
        typename... Interfaces>
    class PluginImpl : public virtual IComponent, // <-- [THE FIX] 
        //      添加 virtual
        public std::enable_shared_from_this<ImplClass>,
        public Interfaces...
    {
    public:
        /**
         * @brief 静态 ClassID 成员。
         */
        static constexpr ClassID kClsid = kClsidParam;

    private:

        // --- 内部辅助模板 ---

        /**
         * @brief 编译期检查：
         * 确保所有接口都继承自 IComponent。
         */
        template <typename First, typename... Rest>
        static constexpr bool AllDeriveFromIComponent()
        {
            static_assert(std::is_base_of_v<IComponent, First>,
                "Template parameter pack 'Interfaces...' must all derive from z3y::IComponent.");

            static_assert(std::is_same_v<decltype(First::kIID), const ClassID>,
                "Interface 'First' must define 'static constexpr ClassID kIID'.");


            if constexpr (sizeof...(Rest) > 0)
            {
                return AllDeriveFromIComponent<Rest...>();
            }
            return true;
        }

        /**
         * @brief 编译期递归：
         * QueryInterfaceRaw 的核心实现。
         *
         * @param[in] iid
         * 要查询的接口 ID。
         */
        template <typename First, typename... Rest>
        void* QueryRecursive(InterfaceID iid)
        {
            if (iid == First::kIID)
            {
                return static_cast<First*>(static_cast<ImplClass*>(this));
            }

            if constexpr (sizeof...(Rest) > 0)
            {
                return QueryRecursive<Rest...>(iid);
            }

            return nullptr; // 遍历完毕，未找到
        }

    public:
        // --- [重构] 
        //   GetComponentBase() 
        //   的 override 
        //   已被移除 ---

        /**
         * @brief 自动实现的
         * IComponent::QueryInterfaceRaw()
         * 虚函数。
         *
         * @param[in] iid
         * 要查询的接口 ID。
         */
        void* QueryInterfaceRaw(InterfaceID iid) override
        {
            [[maybe_unused]] constexpr bool check = AllDeriveFromIComponent<Interfaces...>();

            if (iid == IComponent::kIID)
            {
                return static_cast<IComponent*>(static_cast<ImplClass*>(this));
            }

            if constexpr (sizeof...(Interfaces) > 0)
            {
                return QueryRecursive<Interfaces...>(iid);
            }

            return nullptr;
        }
    }; // <-- [修复] 确保这个分号存在


} // namespace z3y

#endif // Z3Y_FRAMEWORK_PLUGIN_IMPL_H_