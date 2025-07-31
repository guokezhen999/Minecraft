//
// Created by 郭珂桢 on 2024/4/23.
//

#ifndef MINECRAFT_NONCOPYABLE_H
#define MINECRAFT_NONCOPYABLE_H

struct NonCopyable {
    NonCopyable() = default;
    // 设置为不能调用构造函数
    NonCopyable(const NonCopyable &) = delete;
    NonCopyable &operator=(const NonCopyable &) = delete;
};
#endif //MINECRAFT_NONCOPYABLE_H
