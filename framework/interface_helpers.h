/**
 * @file interface_helpers.h
 * @brief [新]
 * 定义 Z3Y_DEFINE_INTERFACE
 * 宏，
 * 用于简化接口定义。
 * @author (您的名字)
 * @date 2025-11-12
 */
#pragma once

#ifndef Z3Y_FRAMEWORK_INTERFACE_HELPERS_H_
#define Z3Y_FRAMEWORK_INTERFACE_HELPERS_H_

#include "framework/class_id.h"

 /**
  * @brief [框架辅助宏]
  * 用于在接口类中统一定义 kIid 和 kName。
  *
  * @details
  * 此宏利用 C++
  * 预处理器的“字符串化
  * (#)”
  * 功能，
  * 自动将 ClassName
  * 转换为 "ClassName"
  * 字符串，
  * 同时使用 UUID
  * 生成编译期的哈希 ID。
  *
  * @param ClassName
  * 接口的类名 (例如 ISimple)。
  * @param UuidString
  * 用于生成 IID
  * 的唯一 UUID 字符串 (
  * 必须是编译期常量)。
  */
#define Z3Y_DEFINE_INTERFACE(ClassName, UuidString) \
    /** \
     * @brief 接口的唯一 IID (由 Z3Y_DEFINE_INTERFACE 自动生成)。 \
     */ \
    static constexpr z3y::InterfaceId kIid = \
        z3y::ConstexprHash(UuidString); \
    /** \
     * @brief 接口的人类可读名称 (由 Z3Y_DEFINE_INTERFACE 自动生成)。 \
     */ \
    static constexpr const char* kName = #ClassName;

#endif // Z3Y_FRAMEWORK_INTERFACE_HELPERS_H_