//
// Creative-style hotbar: 9 slots, keys 1-9 / scroll to select
//

#include "Hotbar.h"
#include "../World/Block/Grass.h"

#include <algorithm>

Hotbar::Hotbar()
{
    m_slots = {
        BlockId::Grass,
        BlockId::Dirt,
        BlockId::Stone,
        BlockId::OakBark,
        BlockId::Sand,
        BlockId::Water,
        BlockId::Torch,
        BlockId::TallGrass,
        BlockId::Snow,
    };
}

void Hotbar::selectSlot(int index)
{
    m_selected = std::clamp(index, 0, SLOT_COUNT - 1);
}

void Hotbar::cycleSlot(int delta)
{
    if (delta == 0)
        return;
    int next = (m_selected + delta) % SLOT_COUNT;
    if (next < 0)
        next += SLOT_COUNT;
    m_selected = next;
}

BlockId Hotbar::slot(int index) const
{
    if (index < 0 || index >= SLOT_COUNT)
        return BlockId::Air;
    return m_slots[index];
}

void Hotbar::setSlot(int index, BlockId id)
{
    if (index < 0 || index >= SLOT_COUNT)
        return;
    m_slots[index] = canonicalizePlaceable(id);
}
