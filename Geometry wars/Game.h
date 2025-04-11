#pragma once

#include "Common.h"
#include "Entity.h"
#include "EntityManager.h"

struct WindowConfig { int W, H, FL, FS; };
struct PlayerConfig { int SR, CR, FR, FG, FB, OR, OG, OB, OT, V; float S; };
struct EnemyConfig { int SR, CR, OR, OG, OB, OT, VMIN, VMAX, L, SI; float SMIN, SMAX; };
struct BulletConfig { int SR, CR, FR, FG, FB, OR, OG, OB, OT, V, L; float S; };

class Game
{
    sf::RenderWindow    m_window;           // the window we will draw to
    bool                m_paused = false;   // whether we update game logic
    bool                m_running = true;   // whether the game is running
    EntityManager       m_entities;         // vector of entities to maintain
    sf::Font            m_font;             // the font we will use to draw
    sf::Text            m_text;             // the text that will be drawn for score
    std::string         m_configSetting;    // the setting in config file (i.e Window/Player/Enemy/Bullet)
    WindowConfig        m_windowConfig;     // variable that stores window config
    PlayerConfig        m_playerConfig;     // variable that stores player config
    EnemyConfig         m_enemyConfig;      // variable that stores enemy config
    BulletConfig        m_bulletConfig;     // variable that stores bullet config
    int                 m_score = 0;        // the score the player has accumulated
    int                 m_currentFrame = 0;
    int                 m_lastEnemySpawnTime = 0;

    std::shared_ptr<Entity> m_player;       // separate variable for our player entity

    void init(const std::string& config);  // initialize the GameState with a config file path

    void sMovement();                       // System: Entity position / movement update
    void sUserInput();                      // System: User Input
    void sRender();                         // System: Render / Drawing
    void sEnemySpawner();                   // System: Spawns Enemies
    void sCollision();                      // System: Collisions

    // helper functions
    void spawnPlayer();
    void spawnEnemy();
    void spawnSmallEnemies(std::shared_ptr<Entity> entity);
    void spawnBullet(std::shared_ptr<Entity> entity, const Vec2& target);
    void spawnSpecialWeapon(std::shared_ptr<Entity> entity);

public:

    Game(const std::string& config);  // constructor, takes in game config

    void run();
};