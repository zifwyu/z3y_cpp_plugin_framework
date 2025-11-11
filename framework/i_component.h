/**
 * @file i_component.h
 * @brief [重构] 定义 z3y 插件框架的 *统一* 组件模型基类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * [重构 v5 - 性能优化]
 * 1. 移除了 `ComponentBase`。
 * 2. 将 `QueryInterfaceRaw`
 * 上移到 `IComponent` 中。
 * 3. 移除了 `GetComponentBase`，
 * 因为 `IComponent`
 * 本身现在就是“生命周期控制器”
 * (通过 `shared_ptr`
 * 体现)。
 * 4.
 * 这使得 `PluginCast`
 * 的开销从 2
 * 次虚调用降低到 1
 * 次。
 *
 * @design
 * 这种“统一基类”
 * 的设计类似于 Microsoft COM
 * 中的 `IUnknown`。
 * 它牺牲了一定的“接口纯洁性”
 * (即 `ISimple`
 * 接口也会暴露 `QueryInterfaceRaw`
 * 方法)，
 * 但换来了极致的接口查询性能。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_COMPONENT_H_
#define Z3Y_FRAMEWORK_I_COMPONENT_H_

#include <memory>       // 用于 std::shared_ptr
#include <typeindex>    // 用于 std::type_index

namespace z3y
{
    /**
     * @typedef PluginPtr
     * @brief 框架中用于管理所有插件对象生命周期的标准智能指针。
     *
     * @design
     * 使用 std::shared_ptr
     * 是此框架的核心，
     * 它实现了自动的、RAII
     * 风格的内存管理。
     */
    template <typename T>
    using PluginPtr = std::shared_ptr<T>;


    /**
     * @class IComponent
     * @brief [重构]
     * 所有插件“接口”和“实现”
     * 必须继承的 *唯一* 基类。
     *
     * @design
     * 它现在同时承担了类似 `IUnknown`
     * 的两个核心职责：
     * 1.
     * **生命周期管理**:
     * 此对象的生命周期由
     * `PluginPtr` (std::shared_ptr)
     * 管理。
     * 2.
     * **接口查询**:
     * 此对象必须实现 `QueryInterfaceRaw`
     * 方法，
     * 以允许 `PluginCast`
     * 进行跨模块类型转换。
     */
    class IComponent
    {
    public:
        /**
         * @brief 虚析构函数。
         * @details
         * 保证通过基类指针删除派生类实例时，
         * 能正确调用派生类的析构函数。
         */
        virtual ~IComponent() = default;

        /**
         * @brief [重构]
         * 核心的“手动 RTTI”查询函数。
         *
         * @design
         * C++ 原生的 dynamic_cast
         * 无法在 DLL
         * 和 EXE 之间可靠地工作。
         * 此函数通过虚函数调用，
         * 允许在 1
         * 次调用内完成跨模块的类型识别。
         *
         * @param[in] type
         * 目标接口的 std::type_index (例如 typeid(ISimple))。
         * @return
         * 如果实现类支持此接口，
         * 则返回该接口的 void* * 指针；
         * 否则返回 nullptr。
         */
        virtual void* QueryInterfaceRaw(std::type_index type) = 0;
    };


} // namespace z3y

#endif // Z3Y_FRAMEWORK_I_COMPONENT_H_