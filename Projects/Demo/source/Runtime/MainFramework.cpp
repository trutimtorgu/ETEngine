﻿#include "stdafx.h"
#include "MainFramework.h"

#include "SpawnSystem.h"
#include "LightControlSystem.h"
#include "SwirlyLightSystem.h"
#include "CelestialBodySystem.h"
#include "PlaylistSystem.h"

#include <EtCore/Content/ResourceManager.h>

#include <EtRendering/SceneRendering/ShadedSceneRenderer.h>
#include <EtRendering/GlobalRenderingSystems/GlobalRenderingSystems.h>

#include <EtGUI/Fonts/SdfFont.h>

#include <EtFramework/SceneGraph/UnifiedScene.h>
#include <EtFramework/Audio/AudioManager.h>

#include <Common/CustomTestAsset.h>


namespace et {
namespace demo {


//================
// Main Framework
//================


//-----------------------------
// MainFramework::OnSystemInit
//
void MainFramework::OnSystemInit()
{
	fw::EcsController& ecs = fw::UnifiedScene::Instance().GetEcs();

	ecs.RegisterSystem<FreeCameraSystem>();
	ecs.RegisterSystem<SpawnSystem>();
	ecs.RegisterSystem<LightControlSystem>();
	ecs.RegisterSystem<SwirlyLightSystem>();
	ecs.RegisterSystem<CelestialBodySystem>();
	ecs.RegisterSystem<PlaylistSystem>();
	ecs.RegisterSystem<DemoUISystem>();
}

//--------------------------
// MainFramework::OnInit
//
void MainFramework::OnInit()
{
	// Fonts
	m_DebugFont = core::ResourceManager::Instance()->GetAssetData<gui::SdfFont>(core::HashString("Fonts/Ubuntu-Regular.ttf"));

	// scenes
	fw::UnifiedScene::Instance().GetEventDispatcher().Register( fw::E_SceneEvent::Activated, 
		fw::T_SceneEventCallback([this](fw::T_SceneEventFlags const flags, fw::SceneEventData const* const evnt)
			{
				UNUSED(flags);
				UNUSED(evnt);
				OnSceneActivated();
				DemoUI::OnSceneActivated();
			}));

	// gui
	DemoUI::SetupDataModels();

	// audio
	fw::AudioManager::GetInstance()->SetDistanceModel(AL_INVERSE_DISTANCE);

	AssetPtr<TestData> m_TestAsset = core::ResourceManager::Instance()->GetAssetData<TestData>(core::HashString("test.txt"));
	ET_ASSERT(m_TestAsset != nullptr);

	LOG(FS("Custom Assets:\n\t%s", std::string(m_TestAsset->GetText(), m_TestAsset->GetLength()).c_str()));
}

//--------------------------
// MainFramework::OnTick
//
// Demo specific updates independent of scene
//
void MainFramework::OnTick()
{
	fw::UnifiedScene& uniScene = fw::UnifiedScene::Instance();
	core::InputManager* const input = core::InputManager::GetInstance();

	// Scene switching
	//-----------------
	if(input->GetKeyState(E_KbdKey::F3) == E_KeyState::Pressed)
	{
		std::vector<core::HashString> const& scenes = GetSceneIds();
		if (--m_CurrentScene >= scenes.size())
		{
			m_CurrentScene = scenes.size() - 1;
		}

		uniScene.LoadScene(scenes[m_CurrentScene]);
	}

	if(input->GetKeyState(E_KbdKey::F4) == E_KeyState::Pressed)
	{
		std::vector<core::HashString> const& scenes = GetSceneIds();
		m_CurrentScene = (m_CurrentScene + 1) % scenes.size();

		uniScene.LoadScene(scenes[m_CurrentScene]);
	}

	// Screenshots
	//-------------
	if (input->GetKeyState(E_KbdKey::F12) == E_KeyState::Pressed)
	{
		m_ScreenshotCapture.Take(render::Viewport::GetCurrentViewport());
	}

	// view
	//------
	bool const up = (input->GetKeyState(E_KbdKey::Up) == E_KeyState::Down);
	if (up || (input->GetKeyState(E_KbdKey::Down) == E_KeyState::Down))
	{
		render::Scene& renderScene = uniScene.GetRenderScene();
		render::PostProcessingSettings ppSettings = renderScene.GetPostProcessingSettings();

		float const newExp = ppSettings.exposure * 4.f;
		ppSettings.exposure += (newExp - ppSettings.exposure) * TIME->DeltaTime() * (up ? 1.f : -1.f);

		LOG(FS("Exposure: %f", ppSettings.exposure));

		renderScene.SetPostProcessingSettings(ppSettings);
	}

	// debug info
	//------------
	if (input->GetKeyState(E_KbdKey::H) == E_KeyState::Pressed)
	{
		m_DrawDebugInfo = !m_DrawDebugInfo;
	}

	if (input->GetKeyState(E_KbdKey::J) == E_KeyState::Pressed)
	{
		m_DrawFontAtlas = !m_DrawFontAtlas;
	}

	if (m_DebugFont != nullptr)
	{
		if (m_DrawDebugInfo)
		{
			gui::TextRenderer& textRenderer = m_GuiRenderer.GetTextRenderer();
			textRenderer.SetFont(m_DebugFont);

			textRenderer.SetColor(vec4(1, 0.3f, 0.3f, 1));
			textRenderer.DrawText(FS("FPS: %i", PERFORMANCE->GetRegularFPS()), vec2(20, 20 + (m_DebugFont->GetFontSize()*1.1f) * 1));

			textRenderer.SetColor(vec4(1, 1, 1, 1));
			textRenderer.DrawText(FS("Frame ms: %f", PERFORMANCE->GetFrameMS()), vec2(20, 20 + (m_DebugFont->GetFontSize()*1.1f) * 2));
			textRenderer.DrawText(FS("Draw Calls: %u", PERFORMANCE->m_PrevDrawCalls), vec2(20, 100 + (m_DebugFont->GetFontSize()*1.1f) * 3));
		}

		if (m_DrawFontAtlas)
		{
			gui::SpriteRenderer::E_ScalingMode const scalingMode = gui::SpriteRenderer::E_ScalingMode::TextureAbs;
			m_GuiRenderer.GetSpriteRenderer().Draw(ToPtr(m_DebugFont->GetAtlas()), vec2(1000, 0), vec4(1), vec2(0), vec2(1), 0, 0, scalingMode);
		}
	}
}

//---------------------------------
// MainFramework::OnSceneActivated
//
// Keep track of current scene for switching and set up the camera
//
void MainFramework::OnSceneActivated()
{
	// active scene as loaded by boot config
	std::vector<core::HashString> const& scenes = GetSceneIds();
	auto foundSceneIt = std::find(scenes.cbegin(), scenes.cend(), fw::UnifiedScene::Instance().GetSceneId());
	ET_ASSERT(foundSceneIt != scenes.cend());
	m_CurrentScene = foundSceneIt - scenes.cbegin();

	// set up camera
	fw::EcsController& ecs = fw::UnifiedScene::Instance().GetEcs();
	fw::T_EntityId const camEnt = fw::UnifiedScene::Instance().GetActiveCamera();

	ET_ASSERT(camEnt != fw::INVALID_ENTITY_ID);
	ET_ASSERT(!ecs.HasComponent<FreeCameraComponent>(camEnt));

	ecs.AddComponents(camEnt, FreeCameraComponent());
}


} // namespace demo
} // namespace et
