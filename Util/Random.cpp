//
// Created by 郭珂桢 on 2024/4/23.
//

#include "Random.h"

RandomSingleton &RandomSingleton::get() {
    static RandomSingleton r;
    return r;
}

RandomSingleton::RandomSingleton() {
    m_randomEngine.seed(static_cast<unsigned>(std::time(nullptr)));
    for (int i = 0; i < 5; i++)
    {
        intInRange(i, i * 5);
    }
}
