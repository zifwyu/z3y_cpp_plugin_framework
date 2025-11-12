/**
 * @file simple_impl_a.h
 * @brief 定义 z3y::example::SimpleImplA (普通组件)。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ...
 * [修改]
 * 1. [修改]
 * 移入 z3y::example
 * 命名空间
 * 2. [修改] [!!]
 * Z3Y_DEFINE_COMPONENT_ID
 * 宏已移回类 *内部*。
 * 3. [修改] [!!]
 * 简化 PluginImpl
 * 继承 (
 * 移除 kClsid
 * 模板参数)
 */

#pragma once

#ifndef Z3Y_PLUGIN_EXAMPLE_SIMPLE_IMPL_A_H_
#define Z3Y_PLUGIN_EXAMPLE_SIMPLE_IMPL_A_H_

#include "interfaces_example/i_simple.h"  // 依赖 ISimple
#include "framework/plugin_impl.h"        // 依赖 PluginImpl
#include "framework/class_id.h"
#include "framework/component_helpers.h"  // [新]
#include <string>

namespace z3y {
    namespace example { // [修改]

        // --- 1. [修改] ClassId 
        // 定义已移入类内部 ---
        // (旧的 namespace clsid 
        // 已删除)

        /**
         * @class SimpleImplA
         * @brief ISimple 接口的一个普通组件实现 ("A")。
         */
        class SimpleImplA
            : public PluginImpl<SimpleImplA,
            ISimple> // [修改] 
            // 移除了 kClsid 
            // 模板参数
        {
        public:
            /**
             * @brief [新]
             * 使用 Z3Y_DEFINE_COMPONENT_ID
             * 定义 kClsid
             */
            Z3Y_DEFINE_COMPONENT_ID("z3y-example-CSimpleImplA-UUID-A9407176")

        public:
            SimpleImplA();
            virtual ~SimpleImplA();

            // --- ISimple 接口实现 ---
            std::string GetSimpleString() override;
        };

    }  // namespace example
}  // namespace z3y

#endif  // Z3Y_PLUGIN_EXAMPLE_SIMPLE_IMPL_A_H_