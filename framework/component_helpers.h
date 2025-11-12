/**
 * @file component_helpers.h
 * @brief [新]
 * 定义 Z3Y_DEFINE_COMPONENT_ID
 * 宏，
 * 用于简化实现类。
 * @author (您的名字)
 * @date 2025-11-12
 *
 * [修改]
 * 1. [FIX]
 * 恢复为正确的“单参数”
 * 宏。
 * 2.
 * 开发者应将其放置在实现类的
 * public
 * 访问区内。
 */
#pragma once

#ifndef Z3Y_FRAMEWORK_COMPONENT_HELPERS_H_
#define Z3Y_FRAMEWORK_COMPONENT_HELPERS_H_

#include "framework/class_id.h"

 /**
  * @brief [框架辅助宏]
  * 用于在实现类中统一定义 kClsid。
  *
  * @details
  * 这是一个非侵入式宏，
  * 应放置在实现类的 public
  * 访问区内。
  *
  * @param UuidString
  * 用于生成 ClassId
  * 的唯一 UUID 字符串 (
  * 必须是编译期常量)。
  */
#define Z3Y_DEFINE_COMPONENT_ID(UuidString) \
    /** \
     * @brief 
     * 实现类的唯一 ClassId (
     * 由 Z3Y_DEFINE_COMPONENT_ID
     * 自动生成)。 \
     */ \
         static constexpr z3y::ClassId kClsid = z3y::ConstexprHash(UuidString);

#endif // Z3Y_FRAMEWORK_COMPONENT_HELPERS_H_