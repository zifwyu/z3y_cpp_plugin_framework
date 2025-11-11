/**
 * @file plugin_impl.h
 * @brief 定义 z3y::PluginImpl 模板助手类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 此文件提供了 z3y::PluginImpl，这是一个 CRTP (奇异递归模板模式) 模板类。
 * 它自动为插件实现类生成所有必需的框架样板代码，
 * (GetComponentBase 和 QueryInterfaceRaw)。
 *
 * @design
 * 这是对传统 C 风格宏（如 X3BEGIN_CLASS_DECLARE）
 * 的现代C++替代品。
 * 它通过模板元编程（if constexpr）在编译期生成代码，
 * 具有类型安全、可调试、IDE友好等优点。
 * 它还将 ClassID 绑定到类类型，为 RegisterComponent 辅助函数提供便利。
 * 它还负责继承 std::enable_shared_from_this<ImplClass>，
 * 以解决菱形继承问题并提供正确的 shared_from_this()。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_IMPL_H_
#define Z3Y_FRAMEWORK_PLUGIN_IMPL_H_

#include "i_component.h"  // 依赖 z3y::IComponent 和 z3y::ComponentBase
#include "class_id.h"     // 依赖 z3y::ClassID
#include <type_traits>    // 依赖 std::is_base_of_v (C++17)
#include <memory>         // 依赖 std::enable_shared_from_this

namespace z3y
{
    /**
     * @class PluginImpl
     * @brief [框架核心] 插件实现类的模板助手 (使用 CRTP 模式)。
     *
     * @tparam ImplClass 最终的实现类名 (例如 SimpleImpl)。
     * @tparam kClsidParam 此实现类对应的 ClassID (例如 clsid::kCSimple)。
     * @tparam Interfaces... 该类所实现的所有 z3y::IComponent 接口
     * (例如 ISimple, ISimple2)。
     */
    template <typename ImplClass,
        ClassID kClsidParam,
        typename... Interfaces>
    class PluginImpl : public ComponentBase,
        public std::enable_shared_from_this<ImplClass>,
        public Interfaces...
    {
    public:
        /**
         * @brief 静态 ClassID 成员。
         *
         * 允许框架通过 `ImplClass::kClsid` 自动获取类的ID。
         */
        static constexpr ClassID kClsid = kClsidParam;

    private:

        // --- 内部辅助模板 ---

        /**
         * @brief 编译期检查：确保所有接口都继承自 IComponent。
         * @return 总是返回 true，如果检查失败则通过 static_assert 触发编译错误。
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
         * @brief 编译期递归：QueryInterfaceRaw 的核心实现。
         * @param[in] type 要查询的 std::type_index。
         * @return 匹配的接口指针 (void*)，或 nullptr。
         */
        template <typename First, typename... Rest>
        void* QueryRecursive(std::type_index type)
        {
            // 检查当前接口
            if (type == std::type_index(typeid(First)))
            {
                // 关键：将 this (指向 ImplClass) 转换为它所实现的接口 (First)
                return static_cast<First*>(static_cast<ImplClass*>(this));
            }

            // C++17: 如果还有剩余类型，则继续递归
            if constexpr (sizeof...(Rest) > 0)
            {
                return QueryRecursive<Rest...>(type);
            }

            return nullptr; // 遍历完毕，未找到
        }

    public:
        /**
         * @brief 自动实现的 IComponent::GetComponentBase() 虚函数。
         *
         * @return 一个指向完整实现类(ImplClass)的 shared_ptr，
         * 它携带了正确的引用计数，并被转型为 ComponentBase。
         */
        PluginPtr<ComponentBase> GetComponentBase() const override
        {
            [[maybe_unused]] constexpr bool check = AllDeriveFromIComponent<Interfaces...>();

            // 调用 std::enable_shared_from_this<ImplClass>::shared_from_this()
            return const_cast<ImplClass*>(static_cast<const ImplClass*>(this))
                ->shared_from_this();
        }

        /**
         * @brief 自动实现的 ComponentBase::QueryInterfaceRaw() 虚函数。
         *
         * 遍历所有在模板参数中声明的接口 (Interfaces...)
         * @param[in] type 要查询的 std::type_index。
         * @return 匹配的接口指针 (void*)，或 nullptr。
         */
        void* QueryInterfaceRaw(std::type_index type) override
        {
            // 自动检查 ComponentBase 基类本身
            if (type == std::type_index(typeid(ComponentBase)))
            {
                return static_cast<ComponentBase*>(static_cast<ImplClass*>(this));
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