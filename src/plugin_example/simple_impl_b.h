/**
 * @file simple_impl_b.h
 * @brief z3y::ISimple 接口的实现类 SimpleImplB 的头文件。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 演示了如何在同一个插件中为同一个接口提供第二个实现。
 */

#ifndef Z3Y_SRC_PLUGIN_EXAMPLE_SIMPLE_IMPL_B_H_
#define Z3Y_SRC_PLUGIN_EXAMPLE_SIMPLE_IMPL_B_H_

#include "framework/plugin_impl.h"
#include "interfaces_example/i_simple.h" // 依赖 ISimple 接口

namespace z3y
{
    /**
     * @brief SimpleImplB 实现类的 ClassID。
     */
    namespace clsid
    {
        constexpr ClassID kCSimpleB =
            ConstexprHash("z3y-example-csimple-impl-B-UUID");
    } // namespace clsid

    /**
     * @class SimpleImplB
     * @brief ISimple 接口的 *第二个* 具体实现。
     */
    class SimpleImplB : public z3y::PluginImpl<SimpleImplB,
        clsid::kCSimpleB,
        ISimple>
    {
    public:
        /**
         * @brief 构造函数。
         */
        SimpleImplB();

        /**
         * @brief 析构函数。
         */
        virtual ~SimpleImplB();

        /**
         * @brief ISimple::Add 接口的实现。
         */
        int Add(int a, int b) const override;
    };

} // namespace z3y

#endif // Z3Y_SRC_PLUGIN_EXAMPLE_SIMPLE_IMPL_B_H_