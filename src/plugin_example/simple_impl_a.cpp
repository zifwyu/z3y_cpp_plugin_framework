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
 */

#include "simple_impl_a.h"
#include <iostream>
#include <sstream>

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