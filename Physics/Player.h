//
// First-person player: gravity, collision, jump, sneak, fly toggle
//

#ifndef MINECRAFT_PLAYER_H
#define MINECRAFT_PLAYER_H

#include <glm/glm.hpp>
#include "AABB.h"

class World;
class Camera;

class Player {
public:
    Player();

    void setPosition(const glm::vec3& feetPos);
    const glm::vec3& getPosition() const { return m_position; }

    void setFlying(bool flying);
    bool isFlying() const { return m_flying; }
    void toggleFlying();

    bool isOnGround() const { return m_onGround; }
    bool isSneaking() const { return m_sneaking; }

    AABB getAABB() const;

    // True if a unit block at (bx,by,bz) overlaps the player body
    bool intersectsBlock(int bx, int by, int bz) const;

    // Keyboard wishes for this frame (call before update)
    void setMoveInput(const glm::vec3& wishDir); // xz horizontal wish, y used in fly
    void setJumpPressed(bool pressed);
    void setSneaking(bool sneaking);

    void update(World& world, float dt);
    void syncCamera(Camera& camera) const;

    float eyeHeight() const;

private:
    void collideAndMove(World& world, glm::vec3 velocity, float dt);
    void resolveAxis(World& world, int axis);

    glm::vec3 m_position{0.0f}; // Feet (bottom center)
    glm::vec3 m_velocity{0.0f};
    glm::vec3 m_wishDir{0.0f};

    bool m_onGround = false;
    bool m_flying = false;
    bool m_sneaking = false;
    bool m_jumpPressed = false;
    bool m_jumpWasPressed = false;
};

#endif //MINECRAFT_PLAYER_H
