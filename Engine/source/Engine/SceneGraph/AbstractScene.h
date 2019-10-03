#pragma once
#include <EtCore/Helper/Hash.h>

#include <Engine/Graphics/EnvironmentMap.h>
#include <Engine/Graphics/PostProcessingSettings.h>
#include "SceneEvents.h"


//forward declaration
class Entity;
class CameraComponent;
class LightComponent;
class Time;
struct SceneContext;
class SceneManager;
class Gbuffer;
class PostProcessingRenderer;
class AudioListenerComponent;
class Skybox;
class PhysicsWorld;


//---------------------------
// AbstractScene
//
// Root scene - currently works through inheritance
//
class AbstractScene
{
public:
	AbstractScene(std::string name);
	virtual ~AbstractScene();

	void AddEntity(Entity* pEntity);
	void RemoveEntity(Entity* pEntity, bool deleteEntity = true);
	void SetActiveCamera(CameraComponent* pCamera);
	void SetSkybox(T_Hash const assetId);
	void SetAudioListener(AudioListenerComponent* val) { m_AudioListener = val; }

	EnvironmentMap const* GetEnvironmentMap() const;
	Skybox* GetSkybox() { return m_pSkybox; }
	std::vector<LightComponent*> GetLights();
	const PostProcessingSettings& GetPostProcessingSettings() const;
	bool SkyboxEnabled() { return m_UseSkyBox; }
	PhysicsWorld* GetPhysicsWorld() const { return m_pPhysicsWorld; }
	AudioListenerComponent* GetAudioListener() const { return m_AudioListener; }

	std::string const& GetName() const { return m_Name; }
	std::vector<Entity*> const& GetEntities() { return m_pEntityVec; }
	std::vector<Entity*> const& GetEntities() const { return m_pEntityVec; }

	Entity* GetEntity(T_Hash const id) const;

	SceneEventDispatcher& GetEventDispatcher() { return m_EventDispatcher; }

protected:

	virtual void Initialize() = 0;
	virtual void Update() = 0;
	virtual void Draw() = 0;
	virtual void DrawForward() = 0;
	virtual void PostDraw() = 0;
	virtual void OnActivated() {}
	virtual void OnDeactivated() {}

	PostProcessingSettings m_PostProcessingSettings;

private:
	friend class SceneManager;
	friend class SceneRenderer;

	void RootInitialize();
	void RootUpdate();
	void RootOnActivated();
	void RootOnDeactivated();

	void GetUniqueEntityName(std::string const& suggestion, std::string& uniqueName) const;

	bool m_IsInitialized = false;
	std::string m_Name;
	std::vector<Entity*> m_pEntityVec;

	SceneContext* m_SceneContext = nullptr;

	AudioListenerComponent* m_AudioListener = nullptr;

	PhysicsWorld* m_pPhysicsWorld = nullptr;

	bool m_UseSkyBox = false;
	Skybox* m_pSkybox = nullptr;

	SceneEventDispatcher m_EventDispatcher;
};

