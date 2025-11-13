/**
 * @file uuid_gen.h
 * @brief 声明一个跨平台的 UUID v4 生成器。
 * @author 孙鹏宇
 * @date 2025-11-10
 */

#ifndef Z3Y_TOOL_CREATE_PLUGIN_UUID_GEN_H_
#define Z3Y_TOOL_CREATE_PLUGIN_UUID_GEN_H_

#include <string>

namespace z3y
{
    namespace tool
    {
        /**
         * @brief 使用 C++11 <random> 生成一个 UUID v4 兼容的字符串。
         * @return 一个格式如 "xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx" 的 UUID 字符串。
         */
        std::string generate_uuid_v4();

    } // namespace tool
} // namespace z3y

#endif // Z3Y_TOOL_CREATE_PLUGIN_UUID_GEN_H_