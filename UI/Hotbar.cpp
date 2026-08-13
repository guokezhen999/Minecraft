//
// Creative-style hotbar: 9 slots, keys 1-9 / scroll to select
//

#include "Hotbar.h"

#include <algorithm>

Hotbar::Hotbar()
{
    // Default placeable kit (inventory deferred — hotkeys only)
    m_slots = {
        BlockId::Stone,
        BlockId::Dirt,
        BlockId::Grass,
        BlockId::Sand,
        BlockId::OakBark,
        BlockId::OakLeaf,
        BlockId::Cactus,
        BlockId::Water,
        BlockId::Torch,
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
    m_slots[index] = id;
}
