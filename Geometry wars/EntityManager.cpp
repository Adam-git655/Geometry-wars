#include "Common.h"
#include "EntityManager.h"

EntityManager::EntityManager()
{

}

void EntityManager::update()
{
	//add entities from m_entitestoadd to proper locations

	for (auto &e: m_entitiesToAdd)
	{
		//       - add them to vector of all entities (m_entities)
		m_entities.push_back(e);


		//       - add them to the vector inside of the map, with the tag as the key
		m_entityMap[e->tag()].push_back(e);
	}


	m_entitiesToAdd.clear();

	//remove dead entities from the vector of all entities
	removeDeadEntities(m_entities);

	//remove dead entities from each vector in the entity map
	for (auto & pair : m_entityMap)
	{
		removeDeadEntities(pair.second);
	}
}

void EntityManager::removeDeadEntities(EntityVec& vec)
{
	//Remove all the dead entities from the input vec. 
	//      This is called by the update() function.

	vec.erase(std::remove_if(vec.begin(), vec.end(), [](const auto& e) {
		return e == nullptr || !e->m_active;
		}),
		vec.end()
	);
}


std::shared_ptr<Entity> EntityManager::addEntity(const std::string& tag)
{
	auto entity = std::shared_ptr<Entity>(new Entity(m_totalEntities++, tag));

	m_entitiesToAdd.push_back(entity);

	return entity;
}

const EntityVec& EntityManager::getEntities()
{
	return m_entities;
}

const EntityVec& EntityManager::getEntities(const std::string& tag	)
{
	return m_entityMap[tag];
}