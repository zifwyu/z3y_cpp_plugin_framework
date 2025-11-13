/**
 * @file simple_impl_b.cpp
 * @brief SimpleImplB 类的实现。
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

#include "simple_impl_b.h"
#include "framework/z3y_plugin_sdk.h"
 // 新增 !!]
#include <iostream>
#include <sstream>

// [!! 
// 新增 !!] 
// 
// 
Z3Y_AUTO_REGISTER_COMPONENT(z3y::example::SimpleImplB, "Simple.B", false /* is_default */);

namespace z3y {
    namespace example { // [修改]

        SimpleImplB::SimpleImplB() {
            std::cout << "  [SimpleImplB] Instance Created (Constructor)."
                << std::endl;
        }

        SimpleImplB::~SimpleImplB() {
            std::cout << "  [SimpleImplB] Instance Destroyed (Destructor)."
                << std::endl;
        }

        // --- ISimple 接口实现 ---

        std::string SimpleImplB::GetSimpleString() {
            return "Hello from SimpleImplB";
        }

    }  // namespace example
}  // namespace z3y