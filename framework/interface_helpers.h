/**
 * @file interface_helpers.h
 * @brief [新]
 * 定义 Z3Y_DEFINE_INTERFACE
 * 宏，
 * 用于简化接口定义。
 * @author (您的名字)
 * @date 2025-11-12
 *
 * [修改]
 * 1. Z3Y_DEFINE_INTERFACE
 * 宏现在接受 (ClassName, UuidString,
 * VersionMajor, VersionMinor)
 * 2.
 * 宏现在定义 kVersionMajor
 * 和 kVersionMinor
 * 3. [FIX]
 * 移除了 kName
 * 定义行末尾多余的分号 (;)
 */
#pragma once

#ifndef Z3Y_FRAMEWORK_INTERFACE_HELPERS_H_
#define Z3Y_FRAMEWORK_INTERFACE_HELPERS_H_

#include "framework/class_id.h"

 /**
  * @brief [框架辅助宏]
  * 用于在接口类中统一定义 kIid, kName,
  * 和 SemVer
  * 版本。
  *
  * @param ClassName
  * 接口的类名 (例如 ISimple)。
  * @param UuidString
  * 用于生成 IID
  * 的唯一 UUID 字符串。
  * @param VersionMajor
  * [新]
  * 主版本号 (
  * 用于破坏性 ABI
  * 更改
  * )。
  * @param VersionMinor
  * [新]
  * 次版本号 (
  * 用于向后兼容的 ABI
  * 更改
  * )。
  */
#define Z3Y_DEFINE_INTERFACE(ClassName, UuidString, VersionMajor, \
    VersionMinor) \
    /** \
     * @brief 接口的唯一 IID (由 Z3Y_DEFINE_INTERFACE 自动生成)。 \
     */ \
    static constexpr z3y::InterfaceId kIid = \
        z3y::ConstexprHash(UuidString); \
    /** \
     * @brief 接口的人类可读名称 (由 Z3Y_DEFINE_INTERFACE 自动生成)。 \
     */ \
    static constexpr const char* kName = #ClassName; /* [FIX] 
                                                    * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * Stop */ \
    /** \
        * @brief [新]
        * 接口的主版本号 (
        * 用于 ABI
        * 破坏性变更
        * )。 \
        */ \
        static constexpr uint32_t kVersionMajor = VersionMajor; \
        /** \
            * @brief [新]
            * 接口的次版本号 (
            * 用于 ABI
            * 兼容性变更
            * )。 \
            */ \
        static constexpr uint32_t kVersionMinor = VersionMinor;

#endif // Z3Y_FRAMEWORK_INTERFACE_HELPERS_H_