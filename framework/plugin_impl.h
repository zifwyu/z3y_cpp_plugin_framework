/**
 * @file plugin_impl.h
 * @brief 定义 z3y::PluginImpl 模板助手类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 * 12. [修改] [!!]
 * QueryRecursive
 * 和 QueryInterfaceRaw
 * 现在接受并设置
 * InstanceError&
 * out_result
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_PLUGIN_IMPL_H_
#define Z3Y_FRAMEWORK_PLUGIN_IMPL_H_

#include "framework/i_component.h"  // 依赖 z3y::IComponent
#include "framework/class_id.h"     // 依赖 z3y::ClassId
#include "framework/i_plugin_query.h" // [新] 依赖 InterfaceDetails
#include "framework/plugin_exceptions.h"     // [!! 
 // 新增 !!]
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
    template <typename ImplClass, typename... Interfaces>
    class PluginImpl : public virtual IComponent,
        public std::enable_shared_from_this<ImplClass>,
        public virtual Interfaces...
    {
    public:

        // Note: This is just a basic implementation. You might need to adjust it based on your actual needs.)
        // static constexpr ClassId kClsid = ImplClass::kClsid;

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
            static_assert(has_kClsid<T>::value,
                "ImplClass must define 'static constexpr z3y::ClassId "
                "kClsid'. (Hint: Use Z3Y_DEFINE_COMPONENT_ID macro inside "
                "your class)");
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
         * [修改]
         * 检查 kVersionMajor
         * 和 kVersionMinor
         * 是否存在
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

            // 检查 kName
            static_assert(
                std::is_same_v<decltype(First::kName), const char* const>,
                "Interface 'First' must define 'static constexpr const "
                "char* kName'. (Hint: Use Z3Y_DEFINE_INTERFACE)");

            // [新] 检查版本
            static_assert(
                std::is_same_v<decltype(First::kVersionMajor), const uint32_t>,
                "Interface 'First' must define 'static constexpr const "
                "uint32_t kVersionMajor'. (Hint: Use Z3Y_DEFINE_INTERFACE)");
            static_assert(
                std::is_same_v<decltype(First::kVersionMinor), const uint32_t>,
                "Interface 'First' must define 'static constexpr const "
                "uint32_t kVersionMinor'. (Hint: Use Z3Y_DEFINE_INTERFACE)");


            if constexpr (sizeof...(Rest) > 0) {
                return AllDeriveFromIComponent<Rest...>();
            }
            return true;
        }

        /**
         * @brief [修改]
         * 编译期递归：
         * QueryInterfaceRaw
         * 的核心实现。
         * [修改]
         * 增加了 SemVer
         * 检查逻辑并设置
         * out_result
         * 。
         */
        template <typename First, typename... Rest>
        void* QueryRecursive(InterfaceId iid, uint32_t host_major,
            uint32_t host_minor, InstanceError& out_result) { // [!! 
            // 修改 !!]

            if (iid == First::kIid) {

                const uint32_t my_major = First::kVersionMajor;
                const uint32_t my_minor = First::kVersionMinor;

                // 1. 
                //    主版本不匹配
                if (my_major != host_major) {
                    out_result = InstanceError::kErrorVersionMajorMismatch; // [!! 
                    // 修改 !!]
                    return nullptr;
                }

                // 2. 
                //    次版本过低
                if (my_minor < host_minor) {
                    out_result = InstanceError::kErrorVersionMinorTooLow; // [!! 
                    // 修改 !!]
                    return nullptr;
                }

                // 
                // 成功
                out_result = InstanceError::kSuccess; // [!! 
                // 修改 !!]
                return static_cast<First*>(static_cast<ImplClass*>(this));
            }

            if constexpr (sizeof...(Rest) > 0) {
                return QueryRecursive<Rest...>(iid, host_major, host_minor, out_result);
            }

            out_result = InstanceError::kErrorInterfaceNotImpl; // [!! 
            // 修改 !!]
            return nullptr;  // 遍历完毕，未找到
        }

        /**
         * @brief [修改] 编译期递归：
         * GetInterfaceDetails 的核心实现。
         */
        template <typename First, typename... Rest>
        static void CollectDetailsRecursive(
            std::vector<InterfaceDetails>& details) {

            // (
            // 上一轮已修改
            // )
            details.push_back(InterfaceDetails{
                First::kIid,
                First::kName,
                InterfaceVersion { // [新]
                    First::kVersionMajor,
                    First::kVersionMinor
                }
                });

            if constexpr (sizeof...(Rest) > 0) {
                CollectDetailsRecursive<Rest...>(details);
            }
        }

    public:
        /**
         * @brief 自动实现的
         * IComponent::QueryInterfaceRaw()
         * 虚函数。
         * [!!
         * 修改 !!]
         * 更新签名
         */
        void* QueryInterfaceRaw(InterfaceId iid, uint32_t major,
            uint32_t minor, InstanceError& out_result) override { // [!! 
            // 修改 !!]

            [[maybe_unused]] constexpr bool check_clsid = CheckHasClsid();
            [[maybe_unused]] constexpr bool check_iids =
                AllDeriveFromIComponent<Interfaces...>();

            // 
            // 检查 IComponent 
            // 自身
            if (iid == IComponent::kIid) {
                const uint32_t my_major = IComponent::kVersionMajor;
                const uint32_t my_minor = IComponent::kVersionMinor;

                if (my_major != major) {
                    out_result = InstanceError::kErrorVersionMajorMismatch; // [!! 
                    // 修改 !!]
                    return nullptr;
                }
                if (my_minor < minor) {
                    out_result = InstanceError::kErrorVersionMinorTooLow; // [!! 
                    // 修改 !!]
                    return nullptr;
                }

                out_result = InstanceError::kSuccess; // [!! 
                // 修改 !!]
                return static_cast<IComponent*>(
                    static_cast<ImplClass*>(this));
            }

            // 
            // 递归检查其他接口
            if constexpr (sizeof...(Interfaces) > 0) {
                return QueryRecursive<Interfaces...>(iid, major, minor, out_result);
            }

            // 
            // 未实现
            out_result = InstanceError::kErrorInterfaceNotImpl; // [!! 
            // 修改 !!]
            return nullptr;
        }

        /**
         * @brief [修改]
         * 静态函数，
         * ...
         */
        static std::vector<InterfaceDetails> GetInterfaceDetails() {

            [[maybe_unused]] constexpr bool check_clsid = CheckHasClsid();
            [[maybe_unused]] constexpr bool check_iids =
                AllDeriveFromIComponent<Interfaces...>();

            std::vector<InterfaceDetails> details;

            // (
            // 上一轮已修改
            // )
            details.push_back(
                InterfaceDetails{
                    IComponent::kIid,
                    IComponent::kName,
                    InterfaceVersion { // [新]
                        IComponent::kVersionMajor,
                        IComponent::kVersionMinor
                    }
                });

            if constexpr (sizeof...(Interfaces) > 0) {
                CollectDetailsRecursive<Interfaces...>(details);
            }
            return details;
        }
    };


}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_PLUGIN_IMPL_H_