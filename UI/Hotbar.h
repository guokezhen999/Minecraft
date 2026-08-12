//
// Creative-style hotbar: 9 slots, keys 1-9 / scroll to select
//

#ifndef MINECRAFT_HOTBAR_H
#define MINECRAFT_HOTBAR_H

#include "../World/Block/BlockId.h"

#include <array>

class Hotbar {
public:
    static constexpr int SLOT_COUNT = 9;

    Hotbar();

    void selectSlot(int index); // clamps to [0, SLOT_COUNT)
    void cycleSlot(int delta);  // wrap around (mouse wheel)

    int selectedIndex() const { return m_selected; }
    BlockId selectedBlock() const { return m_slots[m_selected]; }
    BlockId slot(int index) const;

    void setSlot(int index, BlockId id);

private:
    std::array<BlockId, SLOT_COUNT> m_slots{};
    int m_selected = 0;
};

#endif // MINECRAFT_HOTBAR_H
