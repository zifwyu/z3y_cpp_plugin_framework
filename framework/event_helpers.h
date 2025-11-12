/**
 * @file event_helpers.h
 * @brief [新]
 * 定义 Z3Y_DEFINE_EVENT
 * 宏，
 * 用于简化事件定义。
 * @author (您的名字)
 * @date 2025-11-12
 */
#pragma once

#ifndef Z3Y_FRAMEWORK_EVENT_HELPERS_H_
#define Z3Y_FRAMEWORK_EVENT_HELPERS_H_

#include "framework/class_id.h"

 /**
  * @brief [框架辅助宏]
  * 用于在事件结构体中统一定义 kEventId 和 kName。
  *
  * @param ClassName
  * 事件的结构体名 (例如 PluginLoadSuccessEvent)。
  * @param UuidString
  * 用于生成 EventId
  * 的唯一 UUID 字符串。
  */
#define Z3Y_DEFINE_EVENT(ClassName, UuidString) \
    /** \
     * @brief 
     * 事件的唯一 ID (
     * 由 Z3Y_DEFINE_EVENT
     * 自动生成)。 \
     */ \
         static constexpr z3y::EventId kEventId = \
         z3y::ConstexprHash(UuidString); \
         /** \
          * @brief
          * 事件的人类可读名称 (
          * 由 Z3Y_DEFINE_EVENT
          * 自动生成)。 \
          */ \
         static constexpr const char* kName = #ClassName;

#endif // Z3Y_FRAMEWORK_EVENT_HELPERS_H_