/**
 * @file simple_impl_a.cpp
 * @brief z3y::ISimple 接口的实现类 SimpleImplA 的源文件。
 * @author 孙鹏宇
 * @date 2025-11-10
 */

#include "simple_impl_a.h"
#include <iostream>       // 用于在控制台打印日志

namespace z3y
{

    /**
     * @brief 构造函数。
     */
    SimpleImplA::SimpleImplA()
    {
        std::cout << "[plugin_example]: SimpleImplA created (构造函数)." << std::endl;
    }

    /**
     * @brief 析构函数。
     */
    SimpleImplA::~SimpleImplA()
    {
        std::cout << "[plugin_example]: SimpleImplA destroyed (析构函数)." << std::endl;
    }

    /**
     * @brief ISimple::Add 接口的实现。
     */
    int SimpleImplA::Add(int a, int b) const
    {
        std::cout << "[plugin_example]: SimpleImplA::Add() called." << std::endl;
        // 返回一个独特的值以示区别
        return a + b + 100;
    }

} // namespace z3y