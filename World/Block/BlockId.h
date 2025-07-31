//
// Created by 郭珂桢 on 2024/5/22.
//

#ifndef MINECRAFT_BLOCKID_H
#define MINECRAFT_BLOCKID_H

#include <cstdint>

typedef uint8_t Block_t;

enum class BlockId : Block_t {
    Air,
    Grass,
    Dirt,
    Stone,
    OakBark,
    OakLeaf,
    Sand,
    Water,
    Cactus,
    Rose,
    TallGrass,
    DeadShrub,

    NUM_TYPES
};

#endif //MINECRAFT_BLOCKID_H
