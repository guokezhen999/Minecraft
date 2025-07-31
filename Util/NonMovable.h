//
// Created by 郭珂桢 on 2024/4/23.
//

#ifndef MINECRAFT_NONMOVABLE_H
#define MINECRAFT_NONMOVABLE_H

class NonMovable {
public:
    NonMovable(NonMovable &&) = delete;
    NonMovable &operator=(NonMovable &&) = delete;
protected:
    NonMovable() = default;
};

#endif //MINECRAFT_NONMOVABLE_H
