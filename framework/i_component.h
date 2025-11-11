/**
 * @file i_component.h
 * @brief 定义 z3y 插件框架的组件模型基类。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 此文件定义了 z3y::IComponent 和 z3y::ComponentBase。
 * 这种“双基类”设计是为了将“接口契约”与“实现工具”解耦。
 * 1. IComponent：是所有业务接口（如 ISimple）的基类，它只暴露
 * 一个用于 z3y::PluginCast 的“桥梁”函数。
 * 2. ComponentBase：是所有实现类（如 SimpleImpl）的基类，
 * 它为实现类提供了“手动RTTI”的契约。
 * 这种分离设计确保了业务接口的纯洁性，防止了实现细节（如 RTTI）
 * 污染业务接口。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_COMPONENT_H_
#define Z3Y_FRAMEWORK_I_COMPONENT_H_

#include <memory>       // 用于 std::shared_ptr
#include <typeindex>    // 用于 std::type_index

namespace z3y
{

    // --- 前向声明 ---
    class ComponentBase;
    class IComponent;

    // --- 框架智能指针别名 (类型命名: PascalCase) ---

    /**
     * @typedef PluginPtr
     * @brief 框架中用于管理所有插件对象生命周期的标准智能指针。
     *
     * @design
     * 使用 std::shared_ptr 是此框架的核心，
     * 它实现了自动的、RAII 风格的内存管理。
     */
    template <typename T>
    using PluginPtr = std::shared_ptr<T>;


    // --- 实现基类 (类型命名: PascalCase) ---

    /**
     * @class ComponentBase
     * @brief 所有插件“实现类”的（间接）基类。
     *
     * @design
     * 开发者不应直接继承此类，而应使用 z3y::PluginImpl 模板。
     * 此基类强制所有实现类提供 QueryInterfaceRaw 函数，
     * 充当手动的、跨模块（DLL/EXE）安全的 RTTI 系统。
     * 它不继承 std::enable_shared_from_this，
     * 以避免菱形继承问题。
     */
    class ComponentBase
    {
    public:
        /**
         * @brief 虚析构函数。
         * @details 保证通过基类指针删除派生类实例时，
         * 能正确调用派生类的析构函数。
         */
        virtual ~ComponentBase() = default;

        /**
         * @brief 核心的“手动 RTTI”查询函数。
         *
         * @design
         * C++ 原生的 dynamic_cast 无法在 DLL 和 EXE 之间可靠地工作。
         * 此函数通过虚函数调用，让对象在自己的模块（DLL）内部
         * 安全地进行类型识别。
         *
         * @param[in] type 目标接口的 std::type_index (例如 typeid(ISimple))。
         * @return 如果实现类支持此接口，则返回该接口的 void* 指针；
         * 否则返回 nullptr。
         */
        virtual void* QueryInterfaceRaw(std::type_index type) = 0;
    };


    // --- 接口基类 (类型命名: PascalCase) ---

    /**
     * @class IComponent
     * @brief 所有插件“接口” (例如 ISimple) 必须继承的基类。
     *
     * @design
     * 它的唯一职责是充当 z3y::PluginCast 的“桥梁”。
     * 它将“接口查询”(QueryInterfaceRaw) 和“生命周期获取”(GetComponentBase)
     * 分离到两个基类中，确保了业务接口 (ISimple)
     * 不会被实现细节 (QueryInterfaceRaw) 所污染。
     */
    class IComponent
    {
    public:
        /**
         * @brief 虚析构函数。
         */
        virtual ~IComponent() = default;

        /**
         * @brief 获取此接口的“实现对象”的基指针。
         *
         * @design
         * 这是实现 z3y::PluginCast 的关键。
         * 它允许 z3y::PluginCast 获取一个携带正确引用计数的
         * std::shared_ptr (即“生命周期控制器”)。
         *
         * @return 一个 PluginPtr<ComponentBase>。
         */
        virtual PluginPtr<ComponentBase> GetComponentBase() const = 0;
    };

} // namespace z3y

#endif // Z3Y_FRAMEWORK_I_COMPONENT_H_