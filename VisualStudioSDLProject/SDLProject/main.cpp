#define LOG(argument) std::cout << argument << '\n'
#define STB_IMAGE_IMPLEMENTATION
#define GL_SILENCE_DEPRECATION
#define GL_GLEXT_PROTOTYPES 1
#define NUMBER_OF_ENEMIES 3
#define FIXED_TIMESTEP 0.0166666f
#define ACC_OF_GRAVITY -2.0f
#define PLATFORM_COUNT 7
#define TARGET_COUNT 3

#ifdef _WINDOWS
#include <GL/glew.h>
#endif

#include <SDL.h>
#include <SDL_opengl.h>
#include "glm/mat4x4.hpp"
#include "glm/gtc/matrix_transform.hpp"
#include "ShaderProgram.h"
#include "stb_image.h"
#include "cmath"
#include <ctime>
#include <vector>
#include "Entity.h"

/**
* Author: Faith Villarreal
* Assignment: Lunar Lander
* Date due: 2024-03-09, 11:59pm
* I pledge that I have completed this assignment without
* collaborating with anyone else, in conformance with the
* NYU School of Engineering Policies and Procedures on
* Academic Misconduct.
**/

// ————— STRUCTS AND ENUMS —————//
struct GameState
{
    Entity* player;
    Entity* platforms;
    Entity* game_end;
    Entity* stamina;
};


// ————— CONSTANTS ————— //
const int WINDOW_WIDTH = 900,
WINDOW_HEIGHT = 750;

const float BG_RED = 0.984f,
        BG_GREEN = 0.965f,
        BG_BLUE = 0.949f,
        BG_OPACITY = 1.0f;

const int VIEWPORT_X = 0,
VIEWPORT_Y = 0,
VIEWPORT_WIDTH = WINDOW_WIDTH,
VIEWPORT_HEIGHT = WINDOW_HEIGHT;

const char V_SHADER_PATH[] = "shaders/vertex_textured.glsl",
F_SHADER_PATH[] = "shaders/fragment_textured.glsl";

const float MILLISECONDS_IN_SECOND = 1000.0;
const char  SPRITESHEET_FILEPATH[] = "assets/bunny-player.png",
         PLATFORM_FILEPATH[] =       "assets/chocolate-ground.png",
         TARGET_FILEPATH[] =         "assets/carrot-target.png",
         GAME_FAIL_FILEPATH[] =      "assets/mission-fail.png",
         GAME_WIN_FILEPATH[] =       "assets/mission-success.png",
         STAMINA_CONT_FILEPATH[] =   "assets/stamina.png",
         STAMINA_FILL_FILEPATH[] =   "assets/stamina-fill.png";;

const int NUMBER_OF_TEXTURES = 1;  // to be generated, that is
const GLint LEVEL_OF_DETAIL = 0;  // base image level; Level n is the nth mipmap reduction image
const GLint TEXTURE_BORDER = 0;  // this value MUST be zero

// ————— VARIABLES ————— //
GameState g_game_state;

SDL_Window* g_display_window;
bool g_game_is_running = true;

bool g_game_win = false;
bool g_game_over = false;

ShaderProgram g_shader_program;
glm::mat4 g_view_matrix, g_projection_matrix;

float g_previous_ticks = 0.0f;
float g_time_accumulator = 0.0f;

//---- FUEL VARIABLES ----//
int stamina_value = 1000;
bool fuel_used = false;

// ———— GENERAL FUNCTIONS ———— //
GLuint load_texture(const char* filepath)
{
    int width, height, number_of_components;
    unsigned char* image = stbi_load(filepath, &width, &height, &number_of_components, STBI_rgb_alpha);

    if (image == NULL)
    {
        LOG("Unable to load image. Make sure the path is correct.");
        assert(false);
    }

    GLuint textureID;
    glGenTextures(NUMBER_OF_TEXTURES, &textureID);
    glBindTexture(GL_TEXTURE_2D, textureID);
    glTexImage2D(GL_TEXTURE_2D, LEVEL_OF_DETAIL, GL_RGBA, width, height, TEXTURE_BORDER, GL_RGBA, GL_UNSIGNED_BYTE, image);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);

    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

    stbi_image_free(image);

    return textureID;
}

void initialise()
{
    SDL_Init(SDL_INIT_VIDEO);
    g_display_window = SDL_CreateWindow("Lunar Lander but its a cute bunny trying its best to be healthy.",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        WINDOW_WIDTH, WINDOW_HEIGHT,
        SDL_WINDOW_OPENGL);

    SDL_GLContext context = SDL_GL_CreateContext(g_display_window);
    SDL_GL_MakeCurrent(g_display_window, context);

#ifdef _WINDOWS
    glewInit();
#endif

    glViewport(VIEWPORT_X, VIEWPORT_Y, VIEWPORT_WIDTH, VIEWPORT_HEIGHT);

    g_shader_program.Load(V_SHADER_PATH, F_SHADER_PATH);

    g_view_matrix = glm::mat4(1.0f);
    g_projection_matrix = glm::ortho(-5.0f, 5.0f, -3.75f, 3.75f, -1.0f, 1.0f);

    g_shader_program.SetProjectionMatrix(g_projection_matrix);
    g_shader_program.SetViewMatrix(g_view_matrix);

    glUseProgram(g_shader_program.programID);

    glClearColor(BG_RED, BG_BLUE, BG_GREEN, BG_OPACITY);

    // ————— PLAYER ————— //
    // Existing
    g_game_state.player = new Entity();
    g_game_state.player->entity_type = "player";
    g_game_state.player->set_position(glm::vec3(0.0f, 2.0f, 0.0f));
    g_game_state.player->set_movement(glm::vec3(0.0f));
    g_game_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY * 0.1, 0.0f));
    g_game_state.player->set_speed(1.0f);
    g_game_state.player->m_texture_id = load_texture(SPRITESHEET_FILEPATH);

    g_game_state.player->set_height(0.9f);
    g_game_state.player->set_width(0.9f);

    //--- STAMINA ---//
    g_game_state.stamina = new Entity[2]; //[0] is the static container,[1] is the chaning bar
    g_game_state.stamina[0].set_position(glm::vec3(-3.0f, 3.0f, 0.0f));
    g_game_state.stamina[0].m_texture_id = load_texture(STAMINA_CONT_FILEPATH);
    g_game_state.stamina[1].set_position(glm::vec3(-3.0f, 2.953f, 0.0f));
    g_game_state.stamina[1].m_texture_id = load_texture(STAMINA_FILL_FILEPATH);


    //--- GAME END ---//
    g_game_state.game_end = new Entity();
    g_game_state.game_end->set_position(glm::vec3(0.0f));
    g_game_state.game_end->m_texture_id = load_texture(GAME_FAIL_FILEPATH);


    // ————— PLATFORM ————— //
    g_game_state.platforms = new Entity[PLATFORM_COUNT];
    float platform_y_positions[] = { -1.5f, -3.0f, -3.0f, -1.5f, 0.0f, 0.0f, -1.5f };
    float platform_x_positions[] = { -4.5f, -3.0f, -1.5f,  0.0f, 1.5f, 3.0f,  4.5f };

    for (int i = 0; i < PLATFORM_COUNT; i++)
    {
        switch (i)
        {
        case 1:
        case 2:
        case 5:
            g_game_state.platforms[i].m_texture_id = load_texture(TARGET_FILEPATH);
            g_game_state.platforms[i].entity_type = "target";
            break;
        default:
            g_game_state.platforms[i].m_texture_id = load_texture(PLATFORM_FILEPATH);
            g_game_state.platforms[i].entity_type = "ground";
            break;
        }
        g_game_state.platforms[i].set_position(glm::vec3(platform_x_positions[i], platform_y_positions[i], 0.0f));
        g_game_state.platforms[i].update(0.0f, NULL, 0);
    }

    // ————— GENERAL ————— //
    glEnable(GL_BLEND);
    glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
}

void process_input()
{
    // VERY IMPORTANT: If nothing is pressed, we don't want to go anywhere
    g_game_state.player->set_movement(glm::vec3(0.0f));

    SDL_Event event;
    while (SDL_PollEvent(&event))
    {
        switch (event.type) {
            // End game
        case SDL_QUIT:
        case SDL_WINDOWEVENT_CLOSE:
            g_game_is_running = false;
            break;

        case SDL_KEYDOWN:
            switch (event.key.keysym.sym) {
            case SDLK_q:
                // Quit the game with a keystroke
                g_game_is_running = false;
                break;

            case SDLK_SPACE:
                // Jump
                if (g_game_state.player->m_collided_bottom) g_game_state.player->m_is_jumping = true;
                break;

            default:
                break;
            }

        default:
            break;
        }
    }

    const Uint8* key_state = SDL_GetKeyboardState(NULL);

    /* Only process player input if not game over. */
    if (!g_game_over) {

        if (key_state[SDL_SCANCODE_LEFT] && stamina_value > 0)
        {
            g_game_state.player->set_acceleration(glm::vec3(-1.0f, 0.0f, 0.0f));
            fuel_used = true;
        }
        else if (key_state[SDL_SCANCODE_RIGHT] && stamina_value > 0)
        {
            g_game_state.player->set_acceleration(glm::vec3(1.0f, 0.0f, 0.0f));
            fuel_used = true;
        }
        else {
            g_game_state.player->set_acceleration(glm::vec3(0.0f, ACC_OF_GRAVITY * 0.1, 0.0f));
        }
        if (key_state[SDL_SCANCODE_UP] && stamina_value > 0) {
            g_game_state.player->set_acceleration(glm::vec3(0.0f, 1.0f + ACC_OF_GRAVITY * 0.1, 0.0f));
            fuel_used = true;
        }
        else if (key_state[SDL_SCANCODE_DOWN] && stamina_value > 0) {
            g_game_state.player->set_acceleration(glm::vec3(0.0f, -1.0f + ACC_OF_GRAVITY * 0.1, 0.0f));
            fuel_used = true;
        }


        // This makes sure that the player can't move faster diagonally
        if (glm::length(g_game_state.player->get_movement()) > 1.0f)
        {
            g_game_state.player->set_movement(glm::normalize(g_game_state.player->get_movement()));
        }
    }
}

void update()
{
    // ————— DELTA TIME ————— //
    float ticks = (float)SDL_GetTicks() / MILLISECONDS_IN_SECOND; // get the current number of ticks
    float delta_time = ticks - g_previous_ticks; // the delta time is the difference from the last frame
    g_previous_ticks = ticks;

    // ————— FIXED TIMESTEP ————— //
    delta_time += g_time_accumulator;

    if (delta_time < FIXED_TIMESTEP)
    {
        g_time_accumulator = delta_time;
        return;
    }

    while (delta_time >= FIXED_TIMESTEP)
    {
        CollisionResult coll_res = NoCollision;
        if (!g_game_over) {
            coll_res = g_game_state.player->update(FIXED_TIMESTEP, g_game_state.platforms, PLATFORM_COUNT);
        }

        if (coll_res == CollidedWithGround) {
            g_game_over = true;
            g_game_win = false;
            g_game_state.game_end->set_acceleration(glm::vec3(0.0f));
        }
        else if (coll_res == CollidedWithTarget) {
            g_game_over = true;
            g_game_win = true;
            g_game_state.game_end->set_acceleration(glm::vec3(0.0f));
        }
        delta_time -= FIXED_TIMESTEP;
        if (fuel_used) {
            stamina_value--;
            fuel_used = false;
        }
    }

    g_time_accumulator = delta_time;

    g_game_state.stamina[0].update(FIXED_TIMESTEP, g_game_state.platforms, PLATFORM_COUNT);
    g_game_state.stamina[0].scale(glm::vec3(2.0f, 0.75f, 1.0f));

    float stamina_bar_width = float(stamina_value) / 1000.0f;
    g_game_state.stamina[1].update(FIXED_TIMESTEP, g_game_state.platforms, PLATFORM_COUNT);
    g_game_state.stamina[1].scale(glm::vec3(stamina_bar_width * 1.5, 0.15f, 1.0f));

}

void render()
{
    // ————— GENERAL ————— //
    glClear(GL_COLOR_BUFFER_BIT);

    // ————— PLAYER ————— //
    g_game_state.player->render(&g_shader_program);

    // --- STAMINA ---//
    g_game_state.stamina[0].render(&g_shader_program);
    g_game_state.stamina[1].render(&g_shader_program);

    //----GAME END----//
    if (g_game_over) {
        if (g_game_win) {
            g_game_state.game_end->m_texture_id = load_texture(GAME_WIN_FILEPATH);
        }
        g_game_state.game_end->render(&g_shader_program);
    }

    // ————— PLATFORM ————— //
    for (int i = 0; i < PLATFORM_COUNT; i++) g_game_state.platforms[i].render(&g_shader_program);

    // ————— GENERAL ————— //
    SDL_GL_SwapWindow(g_display_window);
}

void shutdown() { SDL_Quit(); }

// ————— DRIVER GAME LOOP ————— /
int main(int argc, char* argv[])
{
    initialise();

    while (g_game_is_running)
    {
        process_input();
        update();
        render();
    }

    shutdown();
    return 0;
}
