/**
 * @file simple_impl_b.cpp
 * @brief z3y::ISimple 接口的实现类 SimpleImplB 的源文件。
 * @author 孙鹏宇
 * @date 2025-11-10
 */

#include "simple_impl_b.h"
#include <iostream>

namespace z3y
{

    /**
     * @brief 构造函数。
     */
    SimpleImplB::SimpleImplB()
    {
        std::cout << "[plugin_example]: SimpleImplB created (构造函数)." << std::endl;
    }

    /**
     * @brief 析构函数。
     */
    SimpleImplB::~SimpleImplB()
    {
        std::cout << "[plugin_example]: SimpleImplB destroyed (析构函数)." << std::endl;
    }

    /**
     * @brief ISimple::Add 接口的实现。
     */
    int SimpleImplB::Add(int a, int b) const
    {
        std::cout << "[plugin_example]: SimpleImplB::Add() called." << std::endl;
        // 返回一个独特的值以示区别
        return a + b + 2000;
    }

} // namespace z3y