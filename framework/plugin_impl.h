/**
 * @file plugin_impl.h
 * @brief 定义 z3y::PluginImpl 模板助手类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 * [修改]
 * 7. [修改] [!!]
 * CheckHasClsid
 * 已更新为 C++17
 * 兼容的 SFINAE
 * (std::void_t)
 * 8. [修改] [!!]
 * 移除了 'static constexpr ClassId kClsid =
 * ImplClass::kClsid;'
 * (这是 C2039
 * 错误的根源)
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_IMPL_H_
#define Z3Y_FRAMEWORK_PLUGIN_IMPL_H_

#include "framework/i_component.h"  // 依赖 z3y::IComponent
#include "framework/class_id.h"     // 依赖 z3y::ClassId
#include "framework/i_plugin_query.h" // [新] 依赖 InterfaceDetails
#include <type_traits>    // 依赖 std::is_base_of_v (C++17)
#include <memory>         // 依赖 std::enable_shared_from_this
#include <vector>         // [新增]
#include <type_traits>    // [新] 用于 SFINAE

namespace z3y {
    /**
     * @class PluginImpl
     * @brief [框架核心] 插件实现类的模板助手 (使用 CRTP 模式)。
     *
     * @tparam ImplClass
     * 最终的实现类名 (例如 SimpleImpl)。
     * @tparam Interfaces...
     * 该类所实现的所有 z3y::IComponent
     * 接口
     * (例如 ISimple, ISimple2)。
     */
    template <typename ImplClass, typename... Interfaces> // [修改]
    class PluginImpl : public virtual IComponent,
        public std::enable_shared_from_this<ImplClass>,
        public virtual Interfaces...
    {
    public:
        /**
         * @brief [修改]
         * 移除了 'static constexpr ClassId kClsid =
         * ImplClass::kClsid;'
         * * 这一行是导致 C2039/C2065
         * 错误的原因，
         * 因为 ImplClass
         * 在基类实例化时
         * 仍然是一个不完整类型。
         *
         * 框架的其他部分 (
         * 如 plugin_registration.h)
         * 已经直接从 ImplClass::kClsid
         * 获取 ID，
         * 所以 PluginImpl
         * 自身不需要复制这个成员。
         */
         // static constexpr ClassId kClsid = ImplClass::kClsid; 
         // <-- [已删除]

    private:
        // --- 内部辅助模板 ---

        /**
         * @brief [新] [C++17 SFINAE]
         * 检查 T
         * 是否有 kClsid
         * 成员
         */
        template<typename T, typename = std::void_t<>>
        struct has_kClsid : std::false_type {};
        template<typename T>
        struct has_kClsid<T, std::void_t<decltype(T::kClsid)>> : std::true_type {};

        /**
         * @brief [新]
         * 编译期检查：
         * 确保 ImplClass
         * 定义了 kClsid
         * (通过 Z3Y_DEFINE_COMPONENT_ID)
         */
        template <typename T = ImplClass>
        static constexpr bool CheckHasClsid() {

            // [修改] 
            // 使用 C++17 
            // 兼容的 SFINAE 
            // 检查
            static_assert(has_kClsid<T>::value,
                "ImplClass must define 'static constexpr z3y::ClassId "
                "kClsid'. (Hint: Use Z3Y_DEFINE_COMPONENT_ID macro inside "
                "your class)");

            // 
            // 检查 kClsid 
            // 类型是否正确 (
            // 仅在 kClsid 
            // 存在时才检查)
            if constexpr (has_kClsid<T>::value) {
                static_assert(
                    std::is_same_v<decltype(T::kClsid), const ClassId>,
                    "'kClsid' must be of type 'const z3y::ClassId'. (Hint: Use "
                    "Z3Y_DEFINE_COMPONENT_ID macro)");
            }
            return true;
        }


        /**
         * @brief 编译期检查：
         * 确保所有接口都继承自 IComponent
         * 且定义了 kIid 和 kName。
         */
        template <typename First, typename... Rest>
        static constexpr bool AllDeriveFromIComponent() {
            static_assert(std::is_base_of_v<IComponent, First>,
                "Template parameter pack 'Interfaces...' must all derive "
                "from z3y::IComponent.");

            // 检查 kIid
            static_assert(
                std::is_same_v<decltype(First::kIid), const InterfaceId>,
                "Interface 'First' must define 'static constexpr "
                "z3y::InterfaceId kIid'. (Hint: Use Z3Y_DEFINE_INTERFACE)");

            // [新] 检查 kName
            static_assert(
                std::is_same_v<decltype(First::kName), const char* const>,
                "Interface 'First' must define 'static constexpr const "
                "char* kName'. (Hint: Use Z3Y_DEFINE_INTERFACE)");


            if constexpr (sizeof...(Rest) > 0) {
                return AllDeriveFromIComponent<Rest...>();
            }
            return true;
        }

        /**
         * @brief 编译期递归：
         * QueryInterfaceRaw 的核心实现。
         * (此函数不变,
         * 仍然只依赖 kIid)
         */
        template <typename First, typename... Rest>
        void* QueryRecursive(InterfaceId iid) {
            if (iid == First::kIid) {
                return static_cast<First*>(static_cast<ImplClass*>(this));
            }

            if constexpr (sizeof...(Rest) > 0) {
                return QueryRecursive<Rest...>(iid);
            }

            return nullptr;  // 遍历完毕，未找到
        }

        /**
         * @brief [修改] 编译期递归：
         * GetInterfaceDetails 的核心实现。
         */
        template <typename First, typename... Rest>
        static void CollectDetailsRecursive(
            std::vector<InterfaceDetails>& details) {
            // [修改] 
            // 收集 kIid 和 kName
            details.push_back(InterfaceDetails{ First::kIid, First::kName });
            if constexpr (sizeof...(Rest) > 0) {
                CollectDetailsRecursive<Rest...>(details);
            }
        }

    public:
        /**
         * @brief 自动实现的
         * IComponent::QueryInterfaceRaw()
         * 虚函数。
         *
         * @param[in] iid
         * 要查询的接口 ID。
         */
        void* QueryInterfaceRaw(InterfaceId iid) override {
            // [修改] 
            // 增加 kClsid 
            // 检查
            // 
            // (
            // 在函数调用时，
            // ImplClass 
            // 是完整类型，
            // 可以安全检查
            // )
            [[maybe_unused]] constexpr bool check_clsid = CheckHasClsid();
            [[maybe_unused]] constexpr bool check_iids =
                AllDeriveFromIComponent<Interfaces...>();

            if (iid == IComponent::kIid) {
                return static_cast<IComponent*>(static_cast<ImplClass*>(this));
            }

            if constexpr (sizeof...(Interfaces) > 0) {
                return QueryRecursive<Interfaces...>(iid);
            }

            return nullptr;
        }

        /**
         * @brief [修改]
         * 静态函数，
         * 返回此实现类实现的所有接口详情。
         */
        static std::vector<InterfaceDetails> GetInterfaceDetails() {
            // [修改] 
            // 增加 kClsid 
            // 检查
            [[maybe_unused]] constexpr bool check_clsid = CheckHasClsid();
            [[maybe_unused]] constexpr bool check_iids =
                AllDeriveFromIComponent<Interfaces...>();

            std::vector<InterfaceDetails> details;

            // 添加 IComponent 自身
            details.push_back(
                InterfaceDetails{ IComponent::kIid, IComponent::kName });

            if constexpr (sizeof...(Interfaces) > 0) {
                CollectDetailsRecursive<Interfaces...>(details);
            }
            return details;
        }
    };


}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_PLUGIN_IMPL_H_