//
// Creative catalog: one of each placeable block (biome grass variants omitted)
//

#ifndef MINECRAFT_INVENTORY_H
#define MINECRAFT_INVENTORY_H

#include "../World/Block/BlockId.h"

namespace Inventory {

constexpr int COLS = 9;

constexpr BlockId kItems[] = {
    BlockId::Grass,
    BlockId::Dirt,
    BlockId::Stone,
    BlockId::Sand,
    BlockId::Sandstone,
    BlockId::OakBark,
    BlockId::OakLeaf,
    BlockId::SavannaBark,
    BlockId::SavannaLeaf,
    BlockId::SpruceBark,
    BlockId::SpruceLeaf,
    BlockId::JungleBark,
    BlockId::JungleLeaf,
    BlockId::Cactus,
    BlockId::Ice,
    BlockId::Snow,
    BlockId::Water,
    BlockId::Torch,
    BlockId::Rose,
    BlockId::TallGrass,
    BlockId::SavannaTallGrass,
    BlockId::Fern,
    BlockId::DeadShrub,
};

constexpr int COUNT = static_cast<int>(sizeof(kItems) / sizeof(kItems[0]));

} // namespace Inventory

#endif
