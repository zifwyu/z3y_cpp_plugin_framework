/**
 * @file simple_impl_a.h
 * @brief z3y::ISimple 接口的实现类 SimpleImplA 的头文件。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * @details
 * 演示了如何使用 z3y::PluginImpl 模板来实现一个插件组件。
 *
 * @design
 * ClassID (kCSimpleA) 在此头文件中定义，
 * 而不是在 i_simple.h 中。这允许宿主(Host) #include 这个文件
 * 来获取创建此实现所需的信息，而无需污染纯接口文件。
 */

#ifndef Z3Y_SRC_PLUGIN_EXAMPLE_SIMPLE_IMPL_A_H_
#define Z3Y_SRC_PLUGIN_EXAMPLE_SIMPLE_IMPL_A_H_

#include "framework/plugin_impl.h"
#include "interfaces_example/i_simple.h" // 依赖 ISimple 接口

namespace z3y
{
    /**
     * @brief SimpleImplA 实现类的 ClassID。
     */
    namespace clsid
    {
        constexpr ClassID kCSimpleA =
            ConstexprHash("z3y-example-csimple-impl-A-UUID");
    } // namespace clsid

    /**
     * @class SimpleImplA
     * @brief ISimple 接口的 *第一个* 具体实现。
     *
     * 继承 z3y::PluginImpl 并传入：
     * 1. ImplClass (自己): SimpleImplA
     * 2. kClsid: clsid::kCSimpleA
     * 3. Interfaces...: ISimple
     */
    class SimpleImplA : public z3y::PluginImpl<SimpleImplA,
        clsid::kCSimpleA,
        ISimple>
    {
    public:
        /**
         * @brief 构造函数。
         */
        SimpleImplA();

        /**
         * @brief 析构函数。
         */
        virtual ~SimpleImplA();

        // --- ISimple 接口的实现 ---

        /**
         * @brief ISimple::Add 接口的实现。
         */
        int Add(int a, int b) const override;
    };

} // namespace z3y

#endif // Z3Y_SRC_PLUGIN_EXAMPLE_SIMPLE_IMPL_A_H_