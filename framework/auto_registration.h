/**
 * @file auto_registration.h
 * @brief [新]
 * 定义自动注册宏和全局注册列表。
 * @author (您的名字)
 * @date 2025-11-13
 *
 * [修复]：
 * 1.
 * 修复了宏连接 (##)
 * 无法正确展开 __LINE__
 * 的问题。
 * 2. [新增] [!!]
 * 增加了 Z3Y_DEFINE_PLUGIN_ENTRY
 * 宏，
 * 极简插件入口文件。
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_AUTO_REGISTRATION_H_
#define Z3Y_FRAMEWORK_AUTO_REGISTRATION_H_

#include "framework/plugin_registration.h"
#include "framework/i_plugin_registry.h" // [!! 
 // 新增 !!] 
 // 
 // 
#include <vector>
#include <functional>
#include <memory>
#include <type_traits> // 
                       // 
                       // 

namespace z3y {
    namespace internal {

        // 
        // 
        // 
        using RegistryFunc = std::function<void(IPluginRegistry*)>;

        /**
         * @brief
         * 获取全局静态注册函数列表。
         */
        inline std::vector<RegistryFunc>& GetGlobalRegisterList() {
            static std::vector<RegistryFunc> g_list;
            return g_list;
        }

        /**
         * @brief
         * 自动注册器：
         * * */
        struct AutoRegistrar {
            AutoRegistrar(RegistryFunc func) {
                GetGlobalRegisterList().push_back(std::move(func));
            }
        };

        /**
         * @brief [宏辅助]
         * * */
#define Z3Y_AUTO_CONCAT_INNER(a, b) a##b
         /**
          * @brief [宏辅助]
          * * * */
#define Z3Y_AUTO_CONCAT(a, b) Z3Y_AUTO_CONCAT_INNER(a, b)


    } // namespace internal

    /**
     * @brief [框架辅助宏]
     * 自动注册一个
     * *普通组件*。
     * * @param ClassName
     * 实现类名 (
     * 必须包含命名空间
     * )
     * @param Alias
     * 字符串别名
     * @param IsDefault
     * bool (true/false)
     * 是否为默认实现
     */
#define Z3Y_AUTO_REGISTER_COMPONENT(ClassName, Alias, IsDefault) \
    static z3y::internal::AutoRegistrar Z3Y_AUTO_CONCAT(s_auto_reg_at_line_, __LINE__) ( \
        [=](z3y::IPluginRegistry* r) { \
            z3y::RegisterComponent<ClassName>(r, Alias, IsDefault); \
        } \
    );

     /**
      * @brief [框架辅助宏]
      * 自动注册一个
      * *单例服务*。
      * * @param ClassName
      * 实现类名 (
      * 必须包含命名空间
      * )
      * @param Alias
      * 字符串别名
      * @param IsDefault
      * bool (true/false)
      * 是否为默认实现
      */
#define Z3Y_AUTO_REGISTER_SERVICE(ClassName, Alias, IsDefault) \
    static z3y::internal::AutoRegistrar Z3Y_AUTO_CONCAT(s_auto_reg_at_line_, __LINE__) ( \
        [=](z3y::IPluginRegistry* r) { \
            z3y::RegisterService<ClassName>(r, Alias, IsDefault); \
        } \
    );

      /**
       * @brief [!!
       * 极简入口宏 !!]
       * * * */
#define Z3Y_DEFINE_PLUGIN_ENTRY \
    extern "C" Z3Y_PLUGIN_API void z3yPluginInit(z3y::IPluginRegistry* registry) \
    { \
        if (!registry) \
        { \
            return; \
        } \
        \
        for (const auto& reg_func : z3y::internal::GetGlobalRegisterList()) \
        { \
            reg_func(registry); \
        } \
    }

} // namespace z3y

#endif // Z3Y_FRAMEWORK_AUTO_REGISTRATION_H_