/**
 * @file i_simple.h
 * @brief 定义一个简单的示例接口 ISimple。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 这是一个纯粹的业务接口，演示了框架的用法。
 *
 * [重构 v5.1 - Bug 修复]
 * 1. `ISimple`
 * 现在继承 `public virtual IComponent`。
 * 2.
 * 这是为了配合 `PluginImpl`
 * 解决“钻石继承”
 * 歧义 (C2594)。
 */

#ifndef Z3Y_SRC_INTERFACES_EXAMPLE_I_SIMPLE_H_
#define Z3Y_SRC_INTERFACES_EXAMPLE_I_SIMPLE_H_

#include "framework/i_component.h" // 依赖 z3y::IComponent
#include "framework/class_id.h"    // 依赖 z3y::ClassID

namespace z3y
{
    /**
     * @class ISimple
     * @brief 一个简单的示例接口。
     *
     * @design
     * [Fix]
     * 必须使用 `public virtual IComponent`
     * 继承，
     * 以防止 `SimpleImplA`
     * 等实现类
     * 出现 `IComponent`
     * 的歧义。
     */
    class ISimple : public virtual IComponent // <-- [THE FIX] 
        //      添加 virtual
    {
    public:
        /**
         * @brief 虚析构函数。
         */
        virtual ~ISimple() = default;

        /**
         * @brief 一个示例性的业务方法。
         * @param[in] a 第一个加数
         * @param[in] b 第二个加数
         * @return a 和 b 的和。
         */
        virtual int Add(int a, int b) const = 0;
    };

} // namespace z3y

#endif // Z3Y_SRC_INTERFACES_EXAMPLE_I_SIMPLE_H_