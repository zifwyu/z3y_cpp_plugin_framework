/**
 * @file i_simple.h
 * @brief 定义一个简单的示例接口 ISimple。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 这是一个纯粹的业务接口，演示了框架的用法。
 * 它不包含任何实现类的 ClassID，以保持接口和实现的分离。
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
     * 所有接口都必须公有继承 z3y::IComponent。
     */
    class ISimple : public IComponent
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