/**
 * @file uuid_gen.cpp
 * @brief 实现 UUID v4 生成器。
 * @author 孙鹏宇
 * @date 2025-11-10
 */

#include "uuid_gen.h"
#include <random>    // C++11 标准库
#include <sstream>   // C++ 标准库
#include <iomanip>   // C++ 标准库
#include <cstdint>

namespace z3y
{
    namespace tool
    {
        std::string generate_uuid_v4()
        {
            // 1. 获取一个高质量的随机数种子
            std::random_device rd;

            // 2. 使用 Mersenne Twister 引擎
            std::mt19937_64 gen(rd());

            // 3. 定义一个均匀分布，用于生成 64 位无符号整数
            std::uniform_int_distribution<uint64_t> dis;

            // 4. 生成 128 位的随机数据 (两个 64 位)
            uint64_t data1 = dis(gen);
            uint64_t data2 = dis(gen);

            // 5. 设置 UUID v4 的特定位 (版本 4)
            data1 = (data1 & 0xFFFFFFFFFFFF0FFFULL) | 0x0000000000004000ULL;
            // 6. 设置变体位 (variant 1)
            data2 = (data2 & 0x3FFFFFFFFFFFFFFFULL) | 0x8000000000000000ULL;

            // 7. 格式化为标准 UUID 字符串
            std::stringstream ss;
            ss << std::hex << std::setfill('0');

            ss << std::setw(8) << (data1 >> 32);
            ss << '-';
            ss << std::setw(4) << ((data1 >> 16) & 0xFFFF);
            ss << '-';
            ss << std::setw(4) << (data1 & 0xFFFF);
            ss << '-';
            ss << std::setw(4) << (data2 >> 48);
            ss << '-';
            ss << std::setw(12) << (data2 & 0xFFFFFFFFFFFFFULL);

            return ss.str();
        }

    } // namespace tool
} // namespace z3y