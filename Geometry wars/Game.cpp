#include "Game.h"

#include <math.h>
#include <iostream>
#include <SFML/Graphics.hpp>

Game::Game(const std::string & config)
{
	init(config);
}

void Game::init(const std::string& path)
{
	//reading config file here
	std::ifstream fin(path);
	while (fin >> m_configSetting)
	{
		if (m_configSetting == "Window")
		{
			fin >> m_windowConfig.W >> m_windowConfig.H >> m_windowConfig.FL
				>> m_windowConfig.FS;
		}

		if (m_configSetting == "Player")
		{
			fin >> m_playerConfig.SR >> m_playerConfig.CR >> m_playerConfig.S
				>> m_playerConfig.FR >> m_playerConfig.FG >> m_playerConfig.FB
				>> m_playerConfig.OR >> m_playerConfig.OG >> m_playerConfig.OB
				>> m_playerConfig.OT >> m_playerConfig.V;
		}

		if (m_configSetting == "Enemy")
		{
			fin >> m_enemyConfig.SR   >> m_enemyConfig.CR >> m_enemyConfig.SMIN
				>> m_enemyConfig.SMAX >> m_enemyConfig.OR >> m_enemyConfig.OG 
				>> m_enemyConfig.OB   >> m_enemyConfig.OT >> m_enemyConfig.VMIN 
				>> m_enemyConfig.VMAX >> m_enemyConfig.L  >> m_enemyConfig.SI;
		}
	
		if (m_configSetting == "Bullet")
		{
			fin >> m_bulletConfig.SR >> m_bulletConfig.CR >> m_bulletConfig.S
				>> m_bulletConfig.FR >> m_bulletConfig.FG >> m_bulletConfig.FB
				>> m_bulletConfig.OR >> m_bulletConfig.OG >> m_bulletConfig.OB
				>> m_bulletConfig.OT >> m_bulletConfig.V  >> m_bulletConfig.L;
		}
	}
	
	//setup default window
	m_window.create(sf::VideoMode(m_windowConfig.W, m_windowConfig.H), "Skibidi Toilet");
	m_window.setFramerateLimit(m_windowConfig.FL);

	spawnPlayer();

	if (!m_font.loadFromFile("fonts/Tecknaf.otf"))
	{
		std::cerr << "Couldnt load font!\n";
		exit(-1);
	}
}

void Game::run()
{
	while (m_running)
	{
		m_entities.update();

		if (!m_paused)
		{
			sEnemySpawner();
			sMovement();
			sCollision();
			sUserInput();
			sRender();
			//increment current frame
			m_currentFrame++;
		}
		else
		{
			sUserInput();
			
		}
	}
}

//HELPER FUNCTIONS

void Game::spawnPlayer()
{
	//All properties of the player with correct config values

	auto entity = m_entities.addEntity("player");
	
	float mx = m_window.getSize().x / 2.0f;
	float my = m_window.getSize().y / 2.0f;
	
	
	entity->cShape = std::make_shared<CShape>(m_playerConfig.SR, m_playerConfig.V, 
		sf::Color(m_playerConfig.FR, m_playerConfig.FG, m_playerConfig.FB),
		sf::Color(m_playerConfig.OR, m_playerConfig.OG, m_playerConfig.OB), m_playerConfig.OT);

	//setting origin of player circle shape to centre
	entity->cShape->circle.setOrigin(entity->cShape->circle.getRadius(), entity->cShape->circle.getRadius());

	entity->cTransform = std::make_shared<CTransform>(Vec2(mx, my), Vec2(0.0f, 0.0f), 0.0f);

	entity->cInput = std::make_shared<CInput>();

	entity->cCollision = std::make_shared<CCollision>(m_playerConfig.CR);

	m_player = entity;
}

void Game::spawnEnemy()
{
	//Making sure enemy is spawned properly with proper config 
	//and is spawned within window bounds
	
	auto entity = m_entities.addEntity("enemy");

	srand(time(0)); //seed on basis of time

	//random vertices between min and max vertices
	const int vertice_range = m_enemyConfig.VMAX - m_enemyConfig.VMIN + 1;
	int vertices = rand() % vertice_range + m_enemyConfig.VMIN;

	//Random fill color
	const int rand_r = rand() % 255 + 1;
	const int rand_g = rand() % 255 + 1;
	const int rand_b = rand() % 255 + 1;

	entity->cShape = std::make_shared<CShape>(m_enemyConfig.SR, vertices,
		sf::Color(rand_r, rand_g, rand_b), 
		sf::Color(m_enemyConfig.OR, m_enemyConfig.OG, m_enemyConfig.OB), 
		m_enemyConfig.OT);
	
	//setting the origin of the enemy to its middle instead of top left
	entity->cShape->circle.setOrigin(entity->cShape->circle.getRadius(), entity->cShape->circle.getRadius());

	//random speed
	const int speedrange = m_enemyConfig.SMAX - m_enemyConfig.SMIN + 1;
	int randspeed = rand() % speedrange + m_enemyConfig.SMIN;

	//random direction
	int rand_theta = rand() % 361;

	//random position in screen while avoiding edge overlap
	const int xmin = 0 + entity->cShape->circle.getRadius();
	const int xmax = m_window.getSize().x - entity->cShape->circle.getRadius();
	int ex = xmin + (rand() % (1 + xmax - xmin));
	
	const int ymin = 0 + entity->cShape->circle.getRadius();
	const int ymax = m_window.getSize().y - entity->cShape->circle.getRadius();
	float ey = ymin + (rand() % (1 + ymax - ymin));

	entity->cTransform = std::make_shared<CTransform>(Vec2(ex, ey), Vec2(randspeed * cos(rand_theta), randspeed * sin(rand_theta)), 0.0f);

	entity->cCollision = std::make_shared<CCollision>(m_enemyConfig.CR);

	entity->cScore = std::make_shared<CScore>(vertices * 100);

	//record when the most recent enemy was spawned
	m_lastEnemySpawnTime = m_currentFrame;
}

void Game::spawnSmallEnemies(std::shared_ptr<Entity> entity)
{
	//spawning small enemies at the location of the enemy after its death

	// when we create the smaller enemy, we have to read the values of the original enemy
	// - spawn a number of small enemies equal to the vertices of the original enemy
	// - set each small enemy to the same color as the original, half its size
	// - small enemies are worth double points of the original enemy

	const float radius = entity->cShape->circle.getRadius();
	const int vertices = entity->cShape->circle.getPointCount();
	const sf::Color &fill_color = entity->cShape->circle.getFillColor();
	const sf::Color &outline_color = entity->cShape->circle.getOutlineColor();
	const float thickness = entity->cShape->circle.getOutlineThickness();

	const Vec2 &position = entity->cTransform->pos;
	const int speed = entity->cTransform->velocity.length();

	//angle
	const auto angle = (2 * 3.14) / vertices; // 2*3.14 = 360degrees in radians

	for (int i = 1; i <= vertices; i++)
	{
		auto small_enemy = m_entities.addEntity("small_enemy");

		small_enemy->cShape = std::make_shared<CShape>(radius / 2, vertices, fill_color, outline_color, thickness);
		small_enemy->cShape->circle.setOrigin(small_enemy->cShape->circle.getRadius(), small_enemy->cShape->circle.getRadius());
		
		small_enemy->cTransform = std::make_shared<CTransform>(position, Vec2(speed * cos(angle * i), speed * sin(angle * i)), 0.0f);
		small_enemy->cLifespan = std::make_shared<CLifespan>(m_enemyConfig.L);
		small_enemy->cCollision = std::make_shared<CCollision>(radius / 2);
		small_enemy->cScore = std::make_shared<CScore>(vertices * 100 * 2);
	}

}

void Game::spawnBullet(std::shared_ptr<Entity> entity, const Vec2& target)
{
	//Implementing the spawning of a bullet which travels towards the target pos.
	//       - bullet speed is scalar
	//       - setting velocity with big brain

	auto bullet = m_entities.addEntity("bullet");

	Vec2 distvec = target - entity->cTransform->pos;
	Vec2 normalized_vec = distvec.normalize();
	float speed = m_bulletConfig.S;
	Vec2 velocity = Vec2(speed * normalized_vec.x, speed * normalized_vec.y);

	bullet->cTransform = std::make_shared<CTransform>(entity->cTransform->pos, velocity, 0);
	bullet->cShape = std::make_shared<CShape>(m_bulletConfig.SR, m_bulletConfig.V, 
		sf::Color(m_bulletConfig.FR, m_bulletConfig.FG, m_bulletConfig.FB),
		sf::Color(m_bulletConfig.OR, m_bulletConfig.OG, m_bulletConfig.OB), 
		m_bulletConfig.OT);
	bullet->cShape->circle.setOrigin(bullet->cShape->circle.getRadius(), bullet->cShape->circle.getRadius());
	bullet->cLifespan = std::make_shared<CLifespan>(m_bulletConfig.L);
	bullet->cCollision = std::make_shared<CCollision>(m_bulletConfig.CR);
}

void Game::spawnSpecialWeapon(std::shared_ptr<Entity> entity)
{
	//TODO

}

//SYSTEMS

void Game::sMovement()
{
	// Implement all movement in here 

	m_player->cTransform->velocity = { 0, 0 };

	//Player Movement by reading from m_player->cInput and Player collisions with edges
	
	if (m_player->cInput->up &&
		m_player->cTransform->pos.y - m_player->cCollision->radius >= 0)
	{
		m_player->cTransform->velocity.y -= m_playerConfig.S;
	}

	if (m_player->cInput->down &&
		m_player->cTransform->pos.y + m_player->cCollision->radius <= m_window.getSize().y)
	{
		m_player->cTransform->velocity.y += m_playerConfig.S;
	}

	if (m_player->cInput->left && 
		m_player->cTransform->pos.x - m_player->cCollision->radius >= 0)
	{
		m_player->cTransform->velocity.x -= m_playerConfig.S;
	}

	if (m_player->cInput->right &&
		m_player->cTransform->pos.x + m_player->cCollision->radius <= m_window.getSize().x)
	{
		m_player->cTransform->velocity.x += m_playerConfig.S;
	}

	//Update player movement
	m_player->cTransform->pos += m_player->cTransform->velocity;

	//Enemy movement
	for (auto & enemy: m_entities.getEntities("enemy"))
	{
		//Update enemy movement
		enemy->cTransform->pos += enemy->cTransform->velocity;
	}

	//Small Enemy movement 
	for (auto& small_enemy : m_entities.getEntities("small_enemy"))
	{
		//Update small enemy movement
		small_enemy->cTransform->pos += small_enemy->cTransform->velocity;
	}

	//Bullet movement
	for (auto & bullet:m_entities.getEntities("bullet"))
	{
		//Update bullet movement
		bullet->cTransform->pos += bullet->cTransform->velocity;
	}

}

void Game::sRender()
{
	m_window.clear();
	
	for (auto&e : m_entities.getEntities())
	{
		e->cShape->circle.setPosition(e->cTransform->pos.x, e->cTransform->pos.y);


		e->cTransform->angle += 1.0f;
		e->cShape->circle.setRotation(e->cTransform->angle);

		m_window.draw(e->cShape->circle);


		//Handle entity lifespan color change and destruction
		if (e->cLifespan)
		{
			const float elapsed_time_ratio = e->cLifespan->clock.getElapsedTime().asMilliseconds() / (float)e->cLifespan->lifespan;

			if (elapsed_time_ratio >= 1.0f)
			{
				e->destroy();
			}
			else
			{
				const auto alpha = (1.0f - elapsed_time_ratio) * 255;

				const float red = e->cShape->circle.getFillColor().r;
				const float green = e->cShape->circle.getFillColor().g;
				const float blue = e->cShape->circle.getFillColor().b;

				const float red_outline = e->cShape->circle.getOutlineColor().r;
				const float green_outline = e->cShape->circle.getOutlineColor().g;
				const float blue_outline = e->cShape->circle.getOutlineColor().b;

				e->cShape->circle.setFillColor(sf::Color(red, green, blue, alpha));
				e->cShape->circle.setOutlineColor(sf::Color(red_outline, green_outline, blue_outline, alpha));
			}
		}
	}

	std::string score_text = "SCORE: " +  std::to_string(m_score);

	m_text = sf::Text(score_text, m_font, 30);
	m_text.setFillColor(sf::Color(255, 255, 255));
	m_text.setPosition(sf::Vector2f(2,2));
	m_window.draw(m_text);

	m_window.display();
}

void Game::sEnemySpawner()
{
	
	if ((m_currentFrame - m_lastEnemySpawnTime) >= m_enemyConfig.SI)
	{
		spawnEnemy();
	}
}

void Game::sCollision()
{
	// Enemy Collisions 
	for (auto& enemy : m_entities.getEntities("enemy"))
	{

		//Against edge of the screen
		if (enemy->cTransform->pos.x - enemy->cCollision->radius <= 0 ||
			enemy->cTransform->pos.x + enemy->cCollision->radius >= m_window.getSize().x)
		{
			enemy->cTransform->velocity.x *= -1;
		}

		else if (enemy->cTransform->pos.y - enemy->cCollision->radius <= 0 ||
			enemy->cTransform->pos.y + enemy->cCollision->radius >= m_window.getSize().y)
		{
			enemy->cTransform->velocity.y *= -1;
		}

		//With bullet
		for (auto & bullet :m_entities.getEntities("bullet"))
		{

			Vec2 distvec = enemy->cTransform->pos - bullet->cTransform->pos;
			float distsquare = (distvec.x * distvec.x) + (distvec.y * distvec.y);

			float combined_radius = bullet->cCollision->radius + enemy->cCollision->radius;

			if (distsquare < (combined_radius * combined_radius))
			{
				enemy->destroy();
				bullet->destroy();
				spawnSmallEnemies(enemy);
				m_score += enemy->cScore->score;
			}
		}

		//With player
		Vec2 distvec = enemy->cTransform->pos - m_player->cTransform->pos;
		float distsquare = (distvec.x * distvec.x) + (distvec.y * distvec.y);

		float combined_radius = enemy->cCollision->radius + m_player->cCollision->radius;
		
		if (distsquare < (combined_radius * combined_radius))
		{
			enemy->destroy();
			m_player->cTransform->pos = Vec2(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f);
		}
	}

	//Small Enemy Collisions
	for (auto& small_e : m_entities.getEntities("small_enemy"))
	{
		//With bullet
		for (auto& bullet : m_entities.getEntities("bullet"))
		{
			Vec2 distvec = small_e->cTransform->pos - bullet->cTransform->pos;
			float distsquare = (distvec.x * distvec.x) + (distvec.y * distvec.y);
			float combined_radius = bullet->cCollision->radius + small_e->cCollision->radius;
			
			if (distsquare < (combined_radius * combined_radius))
			{
				small_e->destroy();
				bullet->destroy();
				m_score += small_e->cScore->score;
			}
		}

		//With player
		Vec2 distvec = small_e->cTransform->pos - m_player->cTransform->pos;
		float distsquare = (distvec.x * distvec.x) + (distvec.y * distvec.y);

		float combined_radius = small_e->cCollision->radius + m_player->cCollision->radius;

		if (distsquare < (combined_radius * combined_radius))
		{
			small_e->destroy();
			m_player->cTransform->pos = Vec2(m_window.getSize().x / 2.0f, m_window.getSize().y / 2.0f);
		}
	}

}

void Game::sUserInput()
{
	sf::Event event;
	while (m_window.pollEvent(event))
	{
		if (event.type == sf::Event::Closed)
		{
			m_running = false;
		}

		if (event.type == sf::Event::KeyPressed)
		{
			switch (event.key.code)
			{
			case sf::Keyboard::W:
				m_player->cInput->up = true;
				break;

			case sf::Keyboard::A:
				m_player->cInput->left = true;
				break;

			case sf::Keyboard::S:
				m_player->cInput->down = true;
				break;

			case sf::Keyboard::D:
				m_player->cInput->right = true;
				break;

			case sf::Keyboard::P:
				if (!m_paused)
				{
					m_paused = true;
				}
				else
				{
					m_paused = false;
				}
				
				break;

			case sf::Keyboard::Escape:
				m_running = false;
				break;

			default:
				break;
			}
		}

		if (event.type == sf::Event::KeyReleased)
		{
			switch (event.key.code)
			{
			case sf::Keyboard::W:
				m_player->cInput->up = false;
				break;

			case sf::Keyboard::A:
				m_player->cInput->left = false;
				break;

			case sf::Keyboard::S:
				m_player->cInput->down = false;
				break;

			case sf::Keyboard::D:
				m_player->cInput->right = false;
				break;

			default:
				break;
			}
		}

		if (event.type == sf::Event::MouseButtonPressed)
		{
			if (event.mouseButton.button == sf::Mouse::Left)
			{
				spawnBullet(m_player, Vec2(event.mouseButton.x, event.mouseButton.y));
			}

			if (event.mouseButton.button == sf::Mouse::Right)
			{
				std::cout << "Right Mouse Clicked at: " << event.mouseButton.x << "," <<
					event.mouseButton.y << "\n";

				//call special weapon here
			}
		}
	}
}
