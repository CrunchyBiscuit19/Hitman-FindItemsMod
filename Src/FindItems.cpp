#include "FindItems.h"

#include "Events.h"
#include "Functions.h"
#include "Logging.h"

#include <Glacier/ZActor.h>
#include <Glacier/ZKnowledge.h>
#include <Glacier/SGameUpdateEvent.h>
#include <Glacier/ZObject.h>
#include <Glacier/ZScene.h>

#include "Glacier/ZGameLoopManager.h"
#include <Glacier/ZAction.h>
#include <Glacier/ZItem.h>

#include <ImGuizmo.h>

#include "imgui.h"

FindItems::FindItems()
{
}

FindItems::~FindItems()
{
	const ZMemberDelegate<FindItems, void(const SGameUpdateEvent&)> s_Delegate(this, &FindItems::OnFrameUpdate);
	Globals::GameLoopManager->UnregisterFrameUpdate(s_Delegate, 0, EUpdateMode::eUpdatePlayMode);
}

void FindItems::Init()
{
	Hooks::ZEntitySceneContext_LoadScene->AddDetour(this, &FindItems::OnLoadScene);
	Hooks::ZEntitySceneContext_ClearScene->AddDetour(this, &FindItems::OnClearScene);
}

void FindItems::OnEngineInitialized()
{
	const ZMemberDelegate<FindItems, void(const SGameUpdateEvent&)> s_Delegate(this, &FindItems::OnFrameUpdate);
	Globals::GameLoopManager->RegisterFrameUpdate(s_Delegate, 0, EUpdateMode::eUpdatePlayMode);
}

void FindItems::OnDrawMenu()
{
	if (ImGui::Button("ITEMS"))
	{
		m_ItemsMenuActive = !m_ItemsMenuActive;
	}
}

void FindItems::OnDrawUI(bool p_HasFocus)
{
	ImGuizmo::BeginFrame();

	DrawItemsMenu(p_HasFocus);

	ImGuizmo::Enable(p_HasFocus);
}

float4 FindItems::GetHitmanTransformation()
{
	TEntityRef<ZHitman5> s_LocalHitman;
	float4 s_HitmanTransformation;
	Functions::ZPlayerRegistry_GetLocalPlayer->Call(Globals::PlayerRegistry, &s_LocalHitman);
	if (s_LocalHitman)
	{
		ZSpatialEntity* s_HitmanSpatial = s_LocalHitman.m_ref.QueryInterface<ZSpatialEntity>();
		s_HitmanTransformation = s_HitmanSpatial->GetWorldMatrix().Trans;
	}

	return s_HitmanTransformation;
}

void FindItems::DrawItemsMenu(bool p_HasFocus)
{
	if (!p_HasFocus || !m_ItemsMenuActive)
	{
		return;
	}

	ImGui::PushFont(SDK()->GetImGuiBlackFont());
	auto s_Showing = ImGui::Begin("ITEMS MENU", &m_ItemsMenuActive);
	ImGui::PushFont(SDK()->GetImGuiRegularFont());

	// GET PLAYER LOCATION
	s_HitmanTransformation = GetHitmanTransformation();

	// GET PICKUP ITEMS
	const ZHM5ActionManager* s_Hm5ActionManager = Globals::HM5ActionManager;
	std::vector<ZHM5Action*> s_Actions;
	for (unsigned int i = 0; i < s_Hm5ActionManager->m_Actions.size(); ++i)
	{
		ZHM5Action* s_Action = s_Hm5ActionManager->m_Actions[i];
		if (s_Action && s_Action->m_eActionType == EActionType::AT_PICKUP)
		{
			s_Actions.push_back(s_Action);
		}
	}

	// SEARCH BAR
	static char s_ItemQuery[256] { "" };
	ImGui::InputText("##Search", s_ItemQuery, sizeof(s_ItemQuery));

	ImGui::BeginChild("left pane", ImVec2(300, 0), true, ImGuiWindowFlags_HorizontalScrollbar);

	// GET ITEMS INFO
	static std::string s_Selected;
	s_UniqueItems = {};
	for (size_t i = 0; i < s_Actions.size(); i++)
	{
		ZHM5Action* s_Action = s_Actions[i];
		ZHM5Item* s_Item = s_Action->m_Object.QueryInterface<ZHM5Item>();
		std::string s_Title = s_Item->m_pItemConfigDescriptor->m_sTitle.c_str();

		s_UniqueItems[s_Title].push_back({ s_Item });
		if (s_Lines.find(s_Item) == s_Lines.end())
		{
			s_Lines[s_Item] = ItemLineInfo {};
		}
	}

	// DISPLAY ITEMS AS SELECTABLE ELEMENTS
	for (auto it = s_UniqueItems.begin(); it != s_UniqueItems.end(); it++)
	{
		if (!strstr(it->first.c_str(), s_ItemQuery))
		{
			continue;
		}
		if (ImGui::Selectable(std::format("x{} {}", it->second.size(), it->first).c_str(), !s_Selected.compare(it->first))) //inverse because comparing true returns 0
		{
			s_Selected = it->first;
		}
	}

	ImGui::EndChild();
	ImGui::SameLine();

	ImGui::BeginGroup();
	ImGui::BeginChild("right pane", ImVec2(0, -ImGui::GetFrameHeightWithSpacing()));

	// ITERATE SELECTED ITEM
	for (auto [it, i] = std::tuple { s_UniqueItems[s_Selected].begin(), 0 }; it != s_UniqueItems[s_Selected].end(); it++, i++)
	{
		float4 s_ItemTransformation = (*it)->m_rGeomentity.m_pInterfaceRef->GetWorldMatrix().Trans;
		float4 s_TransformationDiff = s_HitmanTransformation - s_ItemTransformation;
		float s_DistanceAway = s_HitmanTransformation.Distance(s_HitmanTransformation, s_ItemTransformation);

		std::string s_SpecificItemID = std::format("{} #{}", s_Selected, i + 1);
		ImGui::TextUnformatted(s_SpecificItemID.c_str());
		ImGui::Text(std::format("X: {:.2f} Y: {:.2f} Z: {:.2f}", s_TransformationDiff.x, s_TransformationDiff.y, s_TransformationDiff.z).c_str());
		ImGui::Text(std::format("Distance: {:.2f}m", s_DistanceAway).c_str());

		// DRAW LINE TO EXACT LOCATION OF ITEMS
		ImGui::Checkbox(std::format("Draw Line##{}", s_SpecificItemID).c_str(), &s_Lines[*it].s_LineDrawn);

		// FILL DETAILS
		s_Lines[*it].label = s_SpecificItemID;
		s_Lines[*it].p_From = SVector3(s_HitmanTransformation.x, s_HitmanTransformation.y, s_HitmanTransformation.z);
		s_Lines[*it].p_To = SVector3(s_ItemTransformation.x, s_ItemTransformation.y, s_ItemTransformation.z);

		ImGui::Text("");
		ImGui::Separator();
		ImGui::Text("");
	}

	ImGui::EndChild();
	ImGui::EndGroup();


	ImGui::PopFont();
	ImGui::End();
	ImGui::PopFont();
}

void FindItems::OnDraw3D(IRenderer* p_Renderer)
{
	// GET PLAYER LOCATION
	s_HitmanTransformation = GetHitmanTransformation();

	for (auto &s_Line :s_Lines)
	{
		if (!s_Line.second.s_LineDrawn)
		{
			continue;
		}

		// GET ITEM LOCATION AND DISTANCE
		float4 s_ItemTransformation = s_Line.first->m_rGeomentity.m_pInterfaceRef->GetWorldMatrix().Trans;
		float4 s_TransformationDiff = s_HitmanTransformation - s_ItemTransformation;
		float s_DistanceAway = s_HitmanTransformation.Distance(s_HitmanTransformation, s_ItemTransformation);

		// LINE
		s_Line.second.p_From = SVector3(s_HitmanTransformation.x, s_HitmanTransformation.y, s_HitmanTransformation.z);
		s_Line.second.p_To = SVector3(s_ItemTransformation.x, s_ItemTransformation.y, s_ItemTransformation.z);
		s_Line.second.p_FromColor = SVector4(256, 256, 256, 1);
		s_Line.second.p_ToColor = s_Line.second.p_FromColor;
		p_Renderer->DrawLine3D(s_Line.second.p_From, s_Line.second.p_To, s_Line.second.p_FromColor, s_Line.second.p_ToColor);

		// ITEM INFO
		s_Line.second.distance = s_DistanceAway;
		SVector2 s_ScreenPos;
		if (p_Renderer->WorldToScreen(SVector3(s_ItemTransformation.x, s_ItemTransformation.y, s_ItemTransformation.z + 0.5f), s_ScreenPos))
			p_Renderer->DrawText2D(ZString(std::format("[{} {:.2f}m]", s_Line.second.label, s_Line.second.distance)), s_ScreenPos, s_Line.second.p_FromColor, 0.f, 0.4f);
	}
}

void FindItems::OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent)
{
}

DECLARE_PLUGIN_DETOUR(FindItems, void, OnLoadScene, ZEntitySceneContext* th, ZSceneData& sceneData)
{
	Logger::Debug("Loading scene: {}", sceneData.m_sceneName);
	s_Lines.clear();
	return HookResult<void>(HookAction::Continue());
}
DECLARE_PLUGIN_DETOUR(FindItems, void, OnClearScene, ZEntitySceneContext* th, bool)
{
	Logger::Debug("Clearing scene.");
	s_Lines.clear();
	return HookResult<void>(HookAction::Continue());
}
DECLARE_ZHM_PLUGIN(FindItems);
