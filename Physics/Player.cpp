//
// First-person player: gravity, collision, jump, sneak, fly toggle
//

#include "Player.h"
#include "../Camera.h"
#include "../World/World.h"
#include "../World/WorldConstants.h"

#include <cmath>
#include <algorithm>

namespace {

constexpr float PLAYER_WIDTH = 0.6f;
constexpr float PLAYER_HEIGHT_STAND = 1.8f;
constexpr float PLAYER_HEIGHT_SNEAK = 1.5f;
constexpr float PLAYER_EYE_STAND = 1.62f;
constexpr float PLAYER_EYE_SNEAK = 1.27f;

constexpr float GRAVITY = 28.0f;
constexpr float JUMP_SPEED = 9.0f;
constexpr float WALK_SPEED = 4.3f;
constexpr float SNEAK_SPEED = 1.3f;
constexpr float FLY_SPEED = 10.0f;

constexpr float MAX_FALL_SPEED = 50.0f;

float playerHeight(bool sneaking) {
    return sneaking ? PLAYER_HEIGHT_SNEAK : PLAYER_HEIGHT_STAND;
}

} // namespace

Player::Player() = default;

void Player::setPosition(const glm::vec3& feetPos) {
    m_position = feetPos;
    m_velocity = glm::vec3(0.0f);
}

void Player::setFlying(bool flying) {
    m_flying = flying;
    if (flying)
        m_velocity.y = 0.0f;
}

void Player::toggleFlying() {
    setFlying(!m_flying);
}

void Player::setMoveInput(const glm::vec3& wishDir) {
    m_wishDir = wishDir;
}

void Player::setJumpPressed(bool pressed) {
    m_jumpPressed = pressed;
}

void Player::setSneaking(bool sneaking) {
    m_sneaking = sneaking;
}

float Player::eyeHeight() const {
    return m_sneaking ? PLAYER_EYE_SNEAK : PLAYER_EYE_STAND;
}

AABB Player::getAABB() const {
    const float h = playerHeight(m_sneaking);
    const float half = PLAYER_WIDTH * 0.5f;
    return AABB(glm::vec3(m_position.x - half, m_position.y, m_position.z - half),
                glm::vec3(PLAYER_WIDTH, h, PLAYER_WIDTH));
}

bool Player::intersectsBlock(int bx, int by, int bz) const {
    return getAABB().intersectsBlock(bx, by, bz);
}

void Player::syncCamera(Camera& camera) const {
    camera.SetPosition(m_position + glm::vec3(0.0f, eyeHeight(), 0.0f));
}

void Player::update(World& world, float dt) {
    if (dt <= 0.0f)
        return;
    dt = std::min(dt, 0.05f);

    // Cannot stand up if standing AABB would intersect solid blocks
    if (!m_sneaking) {
        const float half = PLAYER_WIDTH * 0.5f;
        AABB standBox(glm::vec3(m_position.x - half, m_position.y, m_position.z - half),
                      glm::vec3(PLAYER_WIDTH, PLAYER_HEIGHT_STAND, PLAYER_WIDTH));
        const glm::vec3 bMin = standBox.min();
        const glm::vec3 bMax = standBox.max();
        const int x0 = static_cast<int>(std::floor(bMin.x));
        const int y0 = static_cast<int>(std::floor(bMin.y));
        const int z0 = static_cast<int>(std::floor(bMin.z));
        const int x1 = static_cast<int>(std::floor(bMax.x - 1e-5f));
        const int y1 = static_cast<int>(std::floor(bMax.y - 1e-5f));
        const int z1 = static_cast<int>(std::floor(bMax.z - 1e-5f));
        for (int y = y0; y <= y1 && !m_sneaking; ++y) {
            for (int z = z0; z <= z1 && !m_sneaking; ++z) {
                for (int x = x0; x <= x1; ++x) {
                    if (world.isCollidable(x, y, z) && standBox.intersectsBlock(x, y, z)) {
                        m_sneaking = true;
                        break;
                    }
                }
            }
        }
    }

    const bool jumpEdge = m_jumpPressed && !m_jumpWasPressed;
    m_jumpWasPressed = m_jumpPressed;

    if (m_flying) {
        glm::vec3 wish = m_wishDir;
        if (glm::dot(wish, wish) > 1.0f)
            wish = glm::normalize(wish);

        m_velocity = wish * FLY_SPEED;
        collideAndMove(world, m_velocity, dt);
        m_onGround = false;
        return;
    }

    // Horizontal wish (xz only)
    glm::vec3 wishXZ(m_wishDir.x, 0.0f, m_wishDir.z);
    if (glm::dot(wishXZ, wishXZ) > 1e-6f)
        wishXZ = glm::normalize(wishXZ);

    const float moveSpeed = m_sneaking ? SNEAK_SPEED : WALK_SPEED;
    m_velocity.x = wishXZ.x * moveSpeed;
    m_velocity.z = wishXZ.z * moveSpeed;

    // Gravity
    m_velocity.y -= GRAVITY * dt;
    if (m_velocity.y < -MAX_FALL_SPEED)
        m_velocity.y = -MAX_FALL_SPEED;

    // Jump on rising edge while grounded
    if (jumpEdge && m_onGround) {
        m_velocity.y = JUMP_SPEED;
        m_onGround = false;
    }

    collideAndMove(world, m_velocity, dt);
}

void Player::collideAndMove(World& world, glm::vec3 /*velocity*/, float dt) {
    // Move one axis at a time (swept discrete resolve)
    m_position.x += m_velocity.x * dt;
    resolveAxis(world, 0);

    m_position.z += m_velocity.z * dt;
    resolveAxis(world, 2);

    m_onGround = false;
    m_position.y += m_velocity.y * dt;
    resolveAxis(world, 1);
}

void Player::resolveAxis(World& world, int axis) {
    AABB box = getAABB();
    const glm::vec3 bMin = box.min();
    const glm::vec3 bMax = box.max();

    const int x0 = static_cast<int>(std::floor(bMin.x));
    const int y0 = static_cast<int>(std::floor(bMin.y));
    const int z0 = static_cast<int>(std::floor(bMin.z));
    const int x1 = static_cast<int>(std::floor(bMax.x - 1e-5f));
    const int y1 = static_cast<int>(std::floor(bMax.y - 1e-5f));
    const int z1 = static_cast<int>(std::floor(bMax.z - 1e-5f));

    for (int y = y0; y <= y1; ++y) {
        for (int z = z0; z <= z1; ++z) {
            for (int x = x0; x <= x1; ++x) {
                if (!world.isCollidable(x, y, z))
                    continue;

                AABB block(glm::vec3(x, y, z), glm::vec3(1.0f));
                if (!box.intersects(block))
                    continue;

                const float halfW = PLAYER_WIDTH * 0.5f;
                const float h = playerHeight(m_sneaking);

                if (axis == 0) {
                    if (m_velocity.x > 0.0f) {
                        m_position.x = block.min().x - halfW;
                    } else if (m_velocity.x < 0.0f) {
                        m_position.x = block.max().x + halfW;
                    } else {
                        const float penPos = bMax.x - block.min().x;
                        const float penNeg = block.max().x - bMin.x;
                        m_position.x = (penPos < penNeg)
                                           ? block.min().x - halfW
                                           : block.max().x + halfW;
                    }
                    m_velocity.x = 0.0f;
                } else if (axis == 2) {
                    if (m_velocity.z > 0.0f) {
                        m_position.z = block.min().z - halfW;
                    } else if (m_velocity.z < 0.0f) {
                        m_position.z = block.max().z + halfW;
                    } else {
                        const float penPos = bMax.z - block.min().z;
                        const float penNeg = block.max().z - bMin.z;
                        m_position.z = (penPos < penNeg)
                                           ? block.min().z - halfW
                                           : block.max().z + halfW;
                    }
                    m_velocity.z = 0.0f;
                } else { // Y
                    if (m_velocity.y > 0.0f) {
                        m_position.y = block.min().y - h;
                        m_velocity.y = 0.0f;
                    } else if (m_velocity.y < 0.0f) {
                        m_position.y = block.max().y;
                        m_velocity.y = 0.0f;
                        m_onGround = true;
                    } else {
                        const float penUp = block.max().y - bMin.y;
                        const float penDown = bMax.y - block.min().y;
                        if (penUp < penDown) {
                            m_position.y = block.max().y;
                            m_onGround = true;
                        } else {
                            m_position.y = block.min().y - h;
                        }
                    }
                }

                box = getAABB();
            }
        }
    }
}
