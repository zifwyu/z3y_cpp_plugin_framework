/**
 * @file plugin_impl.h
 * @brief 定义 z3y::PluginImpl 模板助手类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * [重构 v5.1 - Bug 修复]
 * 1.
 * `PluginImpl`
 * 现在继承 `public virtual IComponent`。
 * 2.
 * 这是为了解决“钻石
 * (Diamond)
 * 继承”
 * 歧义 (C2594)。
 * 3.
 * `PluginImpl`
 * 同时继承 `IComponent`
 * (Path 1)
 * 和 `Interfaces...` (Path 2)，
 * 而 `Interfaces...`
 * (如 `ISimple`)
 * 也继承 `IComponent`。
 * 4. `virtual`
 * 关键字确保 `ImplClass`
 * 中
 * 只有 *一个* `IComponent`
 * 实例。
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
     *
     * @design
     * [重构 v5.1]
     * 必须使用 `public virtual IComponent`
     * 来
     * 解决钻石继承问题。
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
         *
         * @details
         * 允许框架通过 `ImplClass::kClsid`
         * 自动获取类的ID，
         * 用于 `RegisterComponent`
         * 等辅助函数。
         */
        static constexpr ClassID kClsid = kClsidParam;

    private:

        // --- 内部辅助模板 ---

        /**
         * @brief 编译期检查：
         * 确保所有接口都继承自 IComponent。
         * @return
         * 总是返回 true，
         * 如果检查失败则通过 static_assert
         * 触发编译错误。
         */
        template <typename First, typename... Rest>
        static constexpr bool AllDeriveFromIComponent()
        {
            static_assert(std::is_base_of_v<IComponent, First>,
                "Template parameter pack 'Interfaces...' must all derive from z3y::IComponent.");

            if constexpr (sizeof...(Rest) > 0)
            {
                return AllDeriveFromIComponent<Rest...>();
            }
            return true;
        }

        /**
         * @brief 编译期递归：
         * QueryInterfaceRaw 的核心实现。
         * @param[in] type
         * 要查询的 std::type_index。
         * @return
         * 匹配的接口指针 (void*)，
         * 或 nullptr。
         */
        template <typename First, typename... Rest>
        void* QueryRecursive(std::type_index type)
        {
            // 检查当前接口
            if (type == std::type_index(typeid(First)))
            {
                // 关键：
                //    将 this (指向 ImplClass) 
                //    转换为它所实现的接口 (First)
                return static_cast<First*>(static_cast<ImplClass*>(this));
            }

            // C++17: 
            //    如果还有剩余类型，
            //    则继续递归
            if constexpr (sizeof...(Rest) > 0)
            {
                return QueryRecursive<Rest...>(type);
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
         * @design
         * 此函数是框架“手动 RTTI”
         * 的核心实现。
         * 它在编译期 (通过
         * QueryRecursive)
         * 生成代码，
         * 遍历所有在模板参数中声明的接口 (Interfaces...)
         *
         * @param[in] type
         * 要查询的 std::type_index。
         * @return
         * 匹配的接口指针 (void*)，
         * 或 nullptr。
         */
        void* QueryInterfaceRaw(std::type_index type) override
        {
            // 
            // 触发编译期检查
            [[maybe_unused]] constexpr bool check = AllDeriveFromIComponent<Interfaces...>();

            // 自动检查 IComponent 基类本身
            if (type == std::type_index(typeid(IComponent)))
            {
                // [Fix] 
                //    此 static_cast 
                //    现在是明确无歧义的。
                return static_cast<IComponent*>(static_cast<ImplClass*>(this));
            }

            // 自动遍历所有模板参数 (Interfaces...)
            if constexpr (sizeof...(Interfaces) > 0)
            {
                return QueryRecursive<Interfaces...>(type);
            }

            return nullptr;
        }
    };

} // namespace z3y

#endif // Z3Y_FRAMEWORK_PLUGIN_IMPL_H_