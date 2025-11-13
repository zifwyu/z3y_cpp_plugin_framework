/**
 * @file i_simple.h
 * @brief 定义 z3y::example::ISimple 接口。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * 1.
 * 使用 Z3Y_DEFINE_INTERFACE
 * 宏 (
 * 版本 1.0)
 * 2.
 * 添加 <string>
 * 3. [修改]
 * 移入 z3y::example
 * 命名空间
 * 4. [修改] [!!]
 * * * */

#pragma once

#ifndef Z3Y_INTERFACES_EXAMPLE_I_SIMPLE_H_
#define Z3Y_INTERFACES_EXAMPLE_I_SIMPLE_H_

#include "framework/z3y_plugin_sdk.h" // [!! 
 // 修改 !!] 
 // 
#include <string>  // [FIX]

namespace z3y {
    namespace example { // [修改]

        /**
         * @class ISimple
         * @brief 一个示例“组件”接口。
         */
        class ISimple : public virtual IComponent {
        public:
            /**
             * @brief [修改]
             * 使用 Z3Y_DEFINE_INTERFACE
             * 宏 (
             * 版本 1.0
             * 版本)
             */
            Z3Y_DEFINE_INTERFACE(ISimple, "z3y-example-ISimple-IID-A4736128", \
                1, 0)

                /**
                 * @brief 获取一个示例字符串。
                 * @return 一个 std::string。
                 */
                virtual std::string GetSimpleString() = 0;
        };

    }  // namespace example
}  // namespace z3y

#endif  // Z3Y_INTERFACES_EXAMPLE_I_SIMPLE_H_