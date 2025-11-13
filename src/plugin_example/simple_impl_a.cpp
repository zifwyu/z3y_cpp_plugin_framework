/**
 * @file simple_impl_a.cpp
 * @brief SimpleImplA 类的实现。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * [修改]
 * 1. [修改]
 * 移入 z3y::example
 * 命名空间
 * 2. [FIX]
 * 移除
 * 3. [FIX]
 * 移除错误的 'Add'
 * 函数
 * 4. [修改] [!!]
 * 增加自动注册宏
 */

#include "simple_impl_a.h"
#include "framework/z3y_plugin_sdk.h"
 // 新增 !!]
#include <iostream>
#include <sstream>

// [!! 
// 新增 !!] 
// 
// 
Z3Y_AUTO_REGISTER_COMPONENT(z3y::example::SimpleImplA, "Simple.A", true /* is_default */);


namespace z3y {
    namespace example { // [修改]

        SimpleImplA::SimpleImplA() {
            std::cout << "  [SimpleImplA] Instance Created (Constructor)."
                << std::endl;
        }

        SimpleImplA::~SimpleImplA() {
            std::cout << "  [SimpleImplA] Instance Destroyed (Destructor)."
                << std::endl;
        }

        // --- ISimple 接口实现 ---

        std::string SimpleImplA::GetSimpleString() {
            return "Hello from SimpleImplA";
        }

    }  // namespace example
}  // namespace z3y