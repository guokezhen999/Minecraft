//
// Created by 郭珂桢 on 2024/4/23.
//

#ifndef MINECRAFT_SINGLETON_H
#define MINECRAFT_SINGLETON_H

#include "NonCopyable.h"
#include "NonMovable.h"

class Singleton : public NonMovable, public NonCopyable {
};

#endif //MINECRAFT_SINGLETON_H
