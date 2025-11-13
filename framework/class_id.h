/**
 * @file class_id.h
 * @brief 定义 z3y::ClassId 和编译期哈希函数。
 * @author 孙鹏宇
 * @date 2025-11-10
 *
 * ... (v2.1 修复日志) ...
 * [修改] 遵从 Google 命名约定 (UpperCamelCase for
 * types)
 */

#pragma once

#ifndef Z3Y_FRAMEWORK_CLASS_ID_H_
#define Z3Y_FRAMEWORK_CLASS_ID_H_

#include <cstdint>  // 用于 uint64_t, C++11 标准库

namespace z3y {

    /**
     * @typedef ClassId
     * @brief 框架中所有“实现类” (Component Class) 的唯一标识符。
     *
     * 这是一个 64 位无符号整数，由 z3y::ConstexprHash 在编译期计算得出。
     */
    using ClassId = uint64_t;

    /**
     * @typedef InterfaceId
     * @brief [修改] 框架中所有“接口” (Interface) 的唯一标识符。
     *
     * @design
     * 它在底层与 ClassId 相同 (uint64_t)，
     * 但在函数签名中使用
     * (例如 QueryInterfaceRaw(InterfaceId iid))
     * 可以清晰地表明意图。
     */
    using InterfaceId = ClassId;

    /**
     * @typedef EventId
     * @brief [修改] 框架中所有“事件” (Event) 的唯一标识符。
     *
     * @design
     * 它在底层与 ClassId 相同 (uint64_t)，
     * 但在事件总线 (EventBus)
     * 的函数签名中使用
     * (例如 FireGlobalImpl(EventId event_id, ...))
     * 可以清晰地表明意图。
     */
    using EventId = ClassId;


    // --- 编译期哈希 (Compile-Time Hashing) ---
    // (内部实现，对用户隐藏)
    namespace internal {
        //! FNV-1a 算法的 64 位偏移基准
        constexpr uint64_t kFnvOffsetBasis = 0xcbf29ce484222325ULL;
        //! FNV-1a 算法的 64 位质数
        constexpr uint64_t kFnvPrime = 0x100000001b3ULL;

        /**
         * @brief 编译期哈希的递归实现函数 (C++14/17)。
         * @param[in] str C风格字符串 (必须是编译期常量)。
         * @param[in] hash 当前的哈希累计值。
         * @return 最终的 64 位哈希值。
         */
        constexpr uint64_t Fnv1aHashRt(const char* str,
            uint64_t hash = kFnvOffsetBasis) {
            return (*str == '\0')
                ? hash
                : Fnv1aHashRt(str + 1,
                    (hash ^ static_cast<uint64_t>(*str)) * kFnvPrime);
        }
    }  // namespace internal


    /**
     * @brief [框架核心工具] 将一个字符串在“编译期”转换为一个 64 位的 ClassId。
     *
     * @example
     * \code{.cpp}
     * namespace z3y {
     * namespace clsid {
     * constexpr z3y::ClassId kCSimple =
     * z3y::ConstexprHash("94071767-ba6b-4769-9eb4-2ebf469289f3");
     * } // namespace clsid
     * } // namespace z3y
     * \endcode
     *
     * @param[in] str 必须是一个编译期常量字符串 (例如 "uuid-string")。
     * @return 64 位的 ClassId (uint64_t)。
     */
    constexpr ClassId ConstexprHash(const char* str) {
        return (str == nullptr || *str == '\0')
            ? 0  // 0 通常被视为无效 ClassId
            : internal::Fnv1aHashRt(str);
    }

}  // namespace z3y

#endif  // Z3Y_FRAMEWORK_CLASS_ID_H_