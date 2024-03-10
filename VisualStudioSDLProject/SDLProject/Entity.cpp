#define GL_SILENCE_DEPRECATION
#define STB_IMAGE_IMPLEMENTATION

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#define GL_GLEXT_PROTOTYPES 1
#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "Entity.h"

Entity::Entity()
{
    // ––––– PHYSICS ––––– //
    m_position = glm::vec3(0.0f);
    m_velocity = glm::vec3(0.0f);
    m_acceleration = glm::vec3(0.0f);

    // ––––– TRANSLATION ––––– //
    m_movement = glm::vec3(0.0f);
    m_speed = 0;
    m_model_matrix = glm::mat4(1.0f);
}

Entity::~Entity()
{
}

CollisionResult Entity::update(float delta_time, Entity* collidable_entities, int collidable_entity_count)
{
    if (!m_is_active) return NoCollision;

    m_collided_top = false;
    m_collided_bottom = false;
    m_collided_left = false;
    m_collided_right = false;

    // ––––– GRAVITY ––––– //

    // ——— Apply acceleration to velocity ——— //
    m_velocity += m_acceleration * delta_time;

    // Limit maximum velocity to prevent object from moving too fast
    const float MAX_VELOCITY = 5.0f;
    if (glm::length(m_velocity) > MAX_VELOCITY)
    {
        m_velocity = glm::normalize(m_velocity) * MAX_VELOCITY;
    }

    // ——— Update position based on velocity ——— //
    m_position += m_velocity * delta_time;

    CollisionResult y_col = check_collision_y(collidable_entities, collidable_entity_count);
    CollisionResult x_col = check_collision_x(collidable_entities, collidable_entity_count);

    if (y_col == CollidedWithGround || x_col == CollidedWithGround) {
        return CollidedWithGround;
    }
    else if (y_col == CollidedWithTarget || x_col == CollidedWithTarget) {
        return CollidedWithTarget;
    }
        
    // ––––– TRANSFORMATIONS ––––– //
    m_model_matrix = glm::mat4(1.0f);
    m_model_matrix = glm::translate(m_model_matrix, m_position);

    return NoCollision;
}

CollisionResult const Entity::check_collision_y(Entity* collidable_entities, int collidable_entity_count)
{
    CollisionResult res = NoCollision;

    for (int i = 0; i < collidable_entity_count; i++)
    {
        // STEP 1: For every entity that our player can collide with...
        Entity* collidable_entity = &collidable_entities[i];

        if (check_collision(collidable_entity))
        {
            // STEP 2: Calculate the distance between its centre and our centre
            //         and use that to calculate the amount of overlap between
            //         both bodies.
            float y_distance = fabs(m_position.y - collidable_entity->m_position.y);
            float y_overlap = fabs(y_distance - (m_height / 2.0f) - (collidable_entity->m_height / 2.0f));

            // STEP 3: "Unclip" ourselves from the other entity, and zero our
            //         vertical velocity.
            if (m_velocity.y > 0) {
                m_position.y -= y_overlap;
                m_velocity.y = 0;
                m_collided_top = true;
            }
            else if (m_velocity.y < 0) {
                m_position.y += y_overlap;
                m_velocity.y = 0;
                m_collided_bottom = true;
            }

            if (collidable_entity->entity_type == "ground") {
                return CollidedWithGround;
            }
            else if (collidable_entity->entity_type == "target") {
                res = CollidedWithTarget;
            }
        }
    }
    return res;
}

CollisionResult const Entity::check_collision_x(Entity* collidable_entities, int collidable_entity_count)
{

    CollisionResult res = NoCollision;

    for (int i = 0; i < collidable_entity_count; i++)
    {
        Entity* collidable_entity = &collidable_entities[i];

        if (check_collision(collidable_entity))
        {
            float x_distance = fabs(m_position.x - collidable_entity->m_position.x);
            float x_overlap = fabs(x_distance - (m_width / 2.0f) - (collidable_entity->m_width / 2.0f));
            if (m_velocity.x > 0) {
                m_position.x -= x_overlap;
                m_velocity.x = 0;
                m_collided_right = true;
            }
            else if (m_velocity.x < 0) {
                m_position.x += x_overlap;
                m_velocity.x = 0;
                m_collided_left = true;
            }

            if (collidable_entity->entity_type == "ground") {
                return CollidedWithGround;
            }
            else if (collidable_entity->entity_type == "target") {
                res = CollidedWithTarget;
            }

        }
    }
    return res;
}

void Entity::render(ShaderProgram* program)
{
    program->SetModelMatrix(m_model_matrix);

    float vertices[] = { -0.5, -0.5, 0.5, -0.5, 0.5, 0.5, -0.5, -0.5, 0.5, 0.5, -0.5, 0.5 };
    float tex_coords[] = { 0.0,  1.0, 1.0,  1.0, 1.0, 0.0,  0.0,  1.0, 1.0, 0.0,  0.0, 0.0 };

    glBindTexture(GL_TEXTURE_2D, m_texture_id);

    glVertexAttribPointer(program->positionAttribute, 2, GL_FLOAT, false, 0, vertices);
    glEnableVertexAttribArray(program->positionAttribute);
    glVertexAttribPointer(program->texCoordAttribute, 2, GL_FLOAT, false, 0, tex_coords);
    glEnableVertexAttribArray(program->texCoordAttribute);

    glDrawArrays(GL_TRIANGLES, 0, 6);

    glDisableVertexAttribArray(program->positionAttribute);
    glDisableVertexAttribArray(program->texCoordAttribute);
}

bool const Entity::check_collision(Entity* other) const
{
    // If either entity is inactive, there shouldn't be any collision
    if (!m_is_active || !other->m_is_active) return false;

    float x_distance = fabs(m_position.x - other->m_position.x) - ((m_width + other->m_width) / 2.0f);
    float y_distance = fabs(m_position.y - other->m_position.y) - ((m_height + other->m_height) / 2.0f);

    return x_distance < 0.0f && y_distance < 0.0f;
}
