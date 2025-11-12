/**
 * @file i_component.h
 * @brief 定义插件框架的统一组件基类 IComponent 和标准智能指针 PluginPtr。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [v2.2 修复]:
 * 1. QueryInterfaceRaw
 * 的参数替换为 InterfaceID
 * 别名。
 * 2. 增加了在 class IComponent
 * 定义末尾遗漏的分号。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_I_COMPONENT_H_
#define Z3Y_FRAMEWORK_I_COMPONENT_H_

#include <memory>       // 为了使用 std::shared_ptr 作为生命周期管理器
#include <typeindex>    // 为了使用 std::type_index 作为 QueryInterfaceRaw 的参数
#include "class_id.h"   // 引入 ClassID, InterfaceID

namespace z3y
{
    /**
     * @typedef PluginPtr
     * @brief 框架中用于管理所有插件对象生命周期的标准智能指针。
     *
     * @design
     * 这是一个基于 std::shared_ptr 的类型别名，是框架自动内存管理的核心。
     * 所有的插件对象实例都应该通过 PluginPtr 来持有和传递。
     * 当最后一个 PluginPtr 实例被销毁时，它所管理的对象也会被自动释放，
     * 这种 RAII 机制能有效防止内存泄漏，简化插件和宿主的代码。
     */
    template <typename T>
    using PluginPtr = std::shared_ptr<T>;


    /**
     * @class IComponent
     * @brief 框架中所有“接口”和“实现”都必须继承的 *唯一* 统一基类。
     *
     * @design
     * IComponent 扮演着类似于 Microsoft COM 中 IUnknown 的关键角色。
     * 它通过C++的虚函数机制，强制所有组件实现两个核心功能：
     * 1. **生命周期管理**：它作为 PluginPtr (shared_ptr) 管理的对象，其生命周期被自动控制。
     * 2. **接口查询**：通过强制实现 QueryInterfaceRaw 方法，
     * 允许在模块（DLL/EXE）之间进行安全、可靠的类型转换，这是 PluginCast 功能的基础。
     *
     * 这种设计牺牲了定的接口纯洁性（例如 ISimple 也会暴露 QueryInterfaceRaw），
     * 但换来了跨模块类型识别的极致性能和可靠性。
     */
    class IComponent
    {
    public:
        /**
         * @brief 为 IComponent 基类本身定义一个唯一的接口 ID (IID)。
         */
        static constexpr ClassID kIID =
            ConstexprHash("z3y-core-IComponent-IID-A0000001");

        /**
         * @brief 虚析构函数（默认实现）。
         * @details
         * 这是 C++ 中实现多态基类的标准做法。
         * 它保证了当派生类实例通过基类指针被删除时，
         * 派生类的析构函数能被正确调用，从而防止资源泄漏。
         */
        virtual ~IComponent() = default;

        /**
         * @brief [核心] 手动运行时类型识别（RTTI）的查询函数（纯虚函数）。
         *
         * @design
         * C++ 原生的 dynamic_cast 在跨越动态链接库（DLL）和可执行文件（EXE）边界时，
         * M可能会因为类型信息不一致而失败。
         * 此函数通过虚函数表提供了一种可靠的“手动”类型查询机制。
         * 它被 PluginCast 函数在内部调用，开发者通常不需要直接调用它。
         *
         * [修改]
         * 参数从 ClassID 更改为 InterfaceID
         * 别名，
         * 以清晰地表明这是一个“接口查询”。
         *
         * @param[in] iid
         * 一个 z3y::InterfaceID (uint64_t)，
         * 代表您希望查询的目标接口类型的唯一ID
         * (例如 ISimple::kIID)。
         * @return
         * 如果当前对象实例支持（实现）了所请求的 `iid` 接口，则返回指向该接口的 void* 指针。
         * 如果不支持该接口，则必须返回 nullptr。
         */
        virtual void* QueryInterfaceRaw(InterfaceID iid) = 0;
    }; // <-- [修复] 
    // 请确保这个分号 100% 存在于您的文件中！
    // 这是导致所有编译错误的根源。


} // namespace z3y

#endif // Z3Y_FRAMEWORK_I_COMPONENT_H_