//
// Created by 郭珂桢 on 2024/4/23.
//

#ifndef MINECRAFT_RANDOM_H
#define MINECRAFT_RANDOM_H

#include <ctime>
#include <random>

#include "Singleton.h"

class RandomSingleton : public Singleton {
public:
    static RandomSingleton &get();

    template<typename T> T intInRange(T low, T high)
    {
        static_assert(std::is_integral<T>::value, "Not integral type!");
        std::uniform_int_distribution<T> dist(low, high);
        return dist(m_randomEngine);
    }

private:
    RandomSingleton();
    std::mt19937 m_randomEngine;
};

template <typename REngine = std::mt19937> class Random {
public:
    Random(int n = std::time(nullptr))
    {
        m_randomEngine.seed(n);
    }

    template <typename T> T intInRange(T low, T high)
    {
        static_assert(std::is_integral<T>::value, "Not integral type!");
        std::uniform_int_distribution<T> dist(low, high);
        return dist(m_randomEngine);
    }

    void setSeed(int seed)
    {
        m_randomEngine.seed(seed);
    }
private:
    REngine m_randomEngine;
};

#endif //MINECRAFT_RANDOM_H
