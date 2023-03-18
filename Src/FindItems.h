#pragma once

#include <vector>
#include "IPluginInterface.h"

class ZHM5Item;

struct ItemLineInfo
{
	bool s_LineDrawn = false;
	std::string label;
	float distance;
	SVector3 p_To;
	SVector3 p_From;
	SVector4 p_ToColor = SVector4(1.f, 1.f, 1.f, 1.f);
	SVector4 p_FromColor = SVector4(1.f, 1.f, 1.f, 1.f);
};

class FindItems : public IPluginInterface
{
public:
	FindItems();
	~FindItems() override;

	void Init() override;
	void OnEngineInitialized() override;
	void OnDrawMenu() override;
	void OnDrawUI(bool p_HasFocus) override;
	void OnDraw3D(IRenderer* p_Renderer) override;

private:
	void OnFrameUpdate(const SGameUpdateEvent& p_UpdateEvent);

private:
	void DrawItemsMenu(bool p_HasFocus);
	float4 GetHitmanTransformation();

private:
	bool m_ItemsMenuActive = false;
	std::unordered_map<std::string, std::vector<ZHM5Item*>> s_UniqueItems;
	std::unordered_map<ZHM5Item*, ItemLineInfo> s_Lines;
	float4 s_HitmanTransformation;

private:
	DEFINE_PLUGIN_DETOUR(FindItems, void, OnLoadScene, ZEntitySceneContext*, ZSceneData&);
	DEFINE_PLUGIN_DETOUR(FindItems, void, OnClearScene, ZEntitySceneContext*, bool);

};

DEFINE_ZHM_PLUGIN(FindItems);
