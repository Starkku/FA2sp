#include "Body.h"

#include "../../Helpers/Translations.h"
#include "../../Helpers/STDHelpers.h"

#include "../../Miscs/TheaterInfo.h"

#include "../../FA2sp.h"

#include <CINI.h>
#include <CMapData.h>
#include <CIsoView.h>
#include <CTileTypeClass.h>

std::array<HTREEITEM, ObjectBrowserControlExt::Root_Count> ObjectBrowserControlExt::ExtNodes;
std::set<ppmfc::CString> ObjectBrowserControlExt::IgnoreSet;
std::set<ppmfc::CString> ObjectBrowserControlExt::ForceName;
std::set<ppmfc::CString> ObjectBrowserControlExt::ExtSets[Set_Count];
std::map<ppmfc::CString, int> ObjectBrowserControlExt::KnownItem;
std::map<ppmfc::CString, int> ObjectBrowserControlExt::Owners;

std::unique_ptr<CPropertyBuilding> ObjectBrowserControlExt::BuildingBrushDlg;
std::unique_ptr<CPropertyInfantry> ObjectBrowserControlExt::InfantryBrushDlg;
std::unique_ptr<CPropertyUnit> ObjectBrowserControlExt::VehicleBrushDlg;
std::unique_ptr<CPropertyAircraft> ObjectBrowserControlExt::AircraftBrushDlg;
bool ObjectBrowserControlExt::BuildingBrushBools[14];
bool ObjectBrowserControlExt::InfantryBrushBools[10];
bool ObjectBrowserControlExt::VehicleBrushBools[11];
bool ObjectBrowserControlExt::AircraftBrushBools[9];
bool ObjectBrowserControlExt::InitPropertyDlgFromProperty{ false };

HTREEITEM ObjectBrowserControlExt::InsertString(const char* pString, DWORD dwItemData,
    HTREEITEM hParent, HTREEITEM hInsertAfter)
{
    return this->InsertItem(5, pString, 0, 0, 0, 0, dwItemData, hParent, hInsertAfter);
}

HTREEITEM ObjectBrowserControlExt::InsertTranslatedString(const char* pOriginString, DWORD dwItemData,
    HTREEITEM hParent, HTREEITEM hInsertAfter)
{
    ppmfc::CString buffer;
    bool result = Translations::GetTranslationItem(pOriginString, buffer);
    return InsertString(result ? buffer : pOriginString, dwItemData, hParent, hInsertAfter);
}

ppmfc::CString ObjectBrowserControlExt::QueryUIName(const char* pRegName)
{
    if (ForceName.find(pRegName) != ForceName.end())
        return Variables::Rules.GetString(pRegName, "Name", pRegName);
    else
        return CMapData::GetUIName(pRegName);
}

void ObjectBrowserControlExt::Redraw()
{
    Redraw_Initialize();
    Redraw_MainList();
    Redraw_Ground();
    Redraw_Owner();
    Redraw_Infantry();
    Redraw_Vehicle();
    Redraw_Aircraft();
    Redraw_Building();
    Redraw_Terrain();
    Redraw_Smudge();
    Redraw_Overlay();
    Redraw_Waypoint();
    Redraw_Celltag();
    Redraw_Basenode();
    Redraw_Tunnel();
    Redraw_PlayerLocation(); // player location is just waypoints!
    Redraw_PropertyBrush();
}

void ObjectBrowserControlExt::Redraw_Initialize()
{
    for (auto root : ExtNodes)
        root = NULL;
    KnownItem.clear();
    IgnoreSet.clear();
    ForceName.clear();
    Owners.clear();
    this->DeleteAllItems();

    auto& rules = CINI::Rules();
    auto& fadata = CINI::FAData();
    auto& doc = CINI::CurrentDocument();

    auto loadSet = [](const char* pTypeName, int nType)
    {
        ExtSets[nType].clear();
        auto& section = Variables::Rules.GetSection(pTypeName);
        for (auto& itr : section)
            ExtSets[nType].insert(itr.second);
    };

    loadSet("BuildingTypes", Set_Building);
    loadSet("InfantryTypes", Set_Infantry);
    loadSet("VehicleTypes", Set_Vehicle);
    loadSet("AircraftTypes", Set_Aircraft);

    if (ExtConfigs::BrowserRedraw_GuessMode == 1)
    {
        auto loadOwner = []()
        {
            auto& sides = Variables::Rules.ParseIndicies("Sides", true);
            for (size_t i = 0, sz = sides.size(); i < sz; ++i)
                for (auto& owner : STDHelpers::SplitString(sides[i]))
                    Owners[owner] = i;
        };
        loadOwner();
    }

    if (auto knownSection = fadata.GetSection("ForceSides"))
    {
        for (auto& item : knownSection->GetEntities())
        {
            int sideIndex = STDHelpers::ParseToInt(item.second, -1);
            if (sideIndex >= fadata.GetKeyCount("Sides"))
                continue;
            if (sideIndex < -1)
                sideIndex = -1;
            KnownItem[item.first] = sideIndex;
        }
    }

    if (auto ignores = fadata.GetSection("IgnoreRA2"))
        for (auto& item : ignores->GetEntities())
            IgnoreSet.insert(item.second);

    if (auto forcenames = fadata.GetSection("ForceName"))
        for (auto& item : forcenames->GetEntities())
            ForceName.insert(item.second);

}

void ObjectBrowserControlExt::Redraw_MainList()
{
    ExtNodes[Root_Nothing] = this->InsertTranslatedString("NothingObList", -2);
    ExtNodes[Root_Ground] = this->InsertTranslatedString("GroundObList", 13);
    ExtNodes[Root_Owner] = this->InsertTranslatedString("ChangeOwnerObList");
    ExtNodes[Root_Infantry] = this->InsertTranslatedString("InfantryObList", 0);
    ExtNodes[Root_Vehicle] = this->InsertTranslatedString("VehiclesObList", 1);
    ExtNodes[Root_Aircraft] = this->InsertTranslatedString("AircraftObList", 2);
    ExtNodes[Root_Building] = this->InsertTranslatedString("StructuresObList", 3);
    ExtNodes[Root_Terrain] = this->InsertTranslatedString("TerrainObList", 4);
    ExtNodes[Root_Smudge] = this->InsertTranslatedString("SmudgesObList", 10);
    ExtNodes[Root_Overlay] = this->InsertTranslatedString("OverlayObList", 5);
    ExtNodes[Root_Waypoint] = this->InsertTranslatedString("WaypointsObList", 6);
    ExtNodes[Root_Celltag] = this->InsertTranslatedString("CelltagsObList", 7);
    ExtNodes[Root_Basenode] = this->InsertTranslatedString("BaseNodesObList", 8);
    ExtNodes[Root_Tunnel] = this->InsertTranslatedString("TunnelObList", 9);
    ExtNodes[Root_PlayerLocation] = this->InsertTranslatedString("StartpointsObList", 12);
    ExtNodes[Root_PropertyBrush] = this->InsertTranslatedString("PropertyBrushObList", 14);
    ExtNodes[Root_Delete] = this->InsertTranslatedString("DelObjObList", 10);
}

void ObjectBrowserControlExt::Redraw_Ground()
{
    HTREEITEM& hGround = ExtNodes[Root_Ground];
    if (hGround == NULL)    return;

    auto& doc = CINI::CurrentDocument();
    CString theater = doc.GetString("Map", "Theater");
    if (theater == "NEWURBAN")
        theater = "UBN";

    CString suffix;
    if (theater != "")
        suffix = theater.Mid(0, 3);

    this->InsertTranslatedString("GroundClearObList" + suffix, 61, hGround);
    this->InsertTranslatedString("GroundSandObList"  + suffix, 62, hGround);
    this->InsertTranslatedString("GroundRoughObList" + suffix, 63, hGround);
    this->InsertTranslatedString("GroundGreenObList" + suffix, 65, hGround);
    this->InsertTranslatedString("GroundPaveObList"  + suffix, 66, hGround);
    this->InsertTranslatedString("GroundWaterObList", 64, hGround);

    if (CINI::CurrentTheater)
    {
        int i = 67;
        for (auto& morphables : TheaterInfo::CurrentInfo)
        {
            auto InsertTile = [&](int nTileset)
            {
                FA2sp::Buffer.Format("TileSet%04d", nTileset);
                FA2sp::Buffer = CINI::CurrentTheater->GetString(FA2sp::Buffer, "SetName", FA2sp::Buffer);
                ppmfc::CString buffer;
                Translations::GetTranslationItem(FA2sp::Buffer, FA2sp::Buffer);
                return this->InsertString(FA2sp::Buffer, i++, hGround, TVI_LAST);
            };

            InsertTile(morphables.Morphable);
        }
    }
}

void ObjectBrowserControlExt::Redraw_Owner()
{
    HTREEITEM& hOwner = ExtNodes[Root_Owner];
    if (hOwner == NULL)    return;

    if (ExtConfigs::BrowserRedraw_SafeHouses)
    {
        if (CMapData::Instance->IsMultiOnly())
        {
            auto& section = Variables::Rules.GetSection("Countries");
            auto itr = section.begin();
            for (size_t i = 0, sz = section.size(); i < sz; ++i, ++itr)
                if (strcmp(itr->second, "Neutral") == 0 || strcmp(itr->second, "Special") == 0)
                    this->InsertString(itr->second, Const_House + i, hOwner);
        }
        else
        {
            auto& section = Variables::Rules.ParseIndicies("Houses", true);
            for (size_t i = 0, sz = section.size(); i < sz; ++i)
                this->InsertString(section[i], Const_House + i, hOwner);
        }
    }
    else
    {
        if (CMapData::Instance->IsMultiOnly())
        {
            if (auto pSection = CINI::Rules->GetSection("Countries"))
            {
                auto& section = pSection->GetEntities();
                size_t i = 0;
                for (auto& itr : section)
                    this->InsertString(itr.second, Const_House + i++, hOwner);
            }
        }
        else
        {
            if (auto pSection = CINI::CurrentDocument->GetSection("Houses"))
            {
                auto& section = pSection->GetEntities();
                size_t i = 0;
                for (auto& itr : section)
                    this->InsertString(itr.second, Const_House + i++, hOwner);
            }
        }
    }
}

void ObjectBrowserControlExt::Redraw_Infantry()
{
    HTREEITEM& hInfantry = ExtNodes[Root_Infantry];
    if (hInfantry == NULL)   return;

    std::map<int, HTREEITEM> subNodes;

    auto& fadata = CINI::FAData();

    int i = 0;
    if (auto sides = fadata.GetSection("Sides"))
        for (auto& itr : sides->GetEntities())
            subNodes[i++] = this->InsertString(itr.second, -1, hInfantry);
    else
    {
        subNodes[i++] = this->InsertString("Allied", -1, hInfantry);
        subNodes[i++] = this->InsertString("Soviet", -1, hInfantry);
        subNodes[i++] = this->InsertString("Yuri", -1, hInfantry);
    }
    subNodes[-1] = this->InsertTranslatedString("OthObList", -1, hInfantry);

    auto& infantries = Variables::Rules.GetSection("InfantryTypes");
    for (auto& inf : infantries)
    {
        if (IgnoreSet.find(inf.second) != IgnoreSet.end())
            continue;
        int index = STDHelpers::ParseToInt(inf.first, -1);
        if (index == -1)   continue;
        int side = GuessSide(inf.second, Set_Infantry);
        if (subNodes.find(side) == subNodes.end())
            side = -1;
        this->InsertString(
            QueryUIName(inf.second),
            Const_Infantry + index,
            subNodes[side]
        );
    }

    // Clear up
    if (ExtConfigs::BrowserRedraw_CleanUp)
    {
        for (auto& subnode : subNodes)
        {
            if (!this->ItemHasChildren(subnode.second))
                this->DeleteItem(subnode.second);
        }
    }
}

void ObjectBrowserControlExt::Redraw_Vehicle()
{
    HTREEITEM& hVehicle = ExtNodes[Root_Vehicle];
    if (hVehicle == NULL)   return;

    std::map<int, HTREEITEM> subNodes;

    auto& fadata = CINI::FAData();

    int i = 0;
    if (auto sides = fadata.GetSection("Sides"))
        for (auto& itr : sides->GetEntities())
            subNodes[i++] = this->InsertString(itr.second, -1, hVehicle);
    else
    {
        subNodes[i++] = this->InsertString("Allied", -1, hVehicle);
        subNodes[i++] = this->InsertString("Soviet", -1, hVehicle);
        subNodes[i++] = this->InsertString("Yuri", -1, hVehicle);
    }
    subNodes[-1] = this->InsertTranslatedString("OthObList", -1, hVehicle);

    auto& vehicles = Variables::Rules.GetSection("VehicleTypes");
    for (auto& veh : vehicles)
    {
        if (IgnoreSet.find(veh.second) != IgnoreSet.end())
            continue;
        int index = STDHelpers::ParseToInt(veh.first, -1);
        if (index == -1)   continue;
        int side = GuessSide(veh.second, Set_Vehicle);
        if (subNodes.find(side) == subNodes.end())
            side = -1;
        this->InsertString(
            QueryUIName(veh.second),
            Const_Vehicle + index,
            subNodes[side]
        );
    }

    // Clear up
    if (ExtConfigs::BrowserRedraw_CleanUp)
    {
        for (auto& subnode : subNodes)
        {
            if (!this->ItemHasChildren(subnode.second))
                this->DeleteItem(subnode.second);
        }
    }
}

void ObjectBrowserControlExt::Redraw_Aircraft()
{
    HTREEITEM& hAircraft = ExtNodes[Root_Aircraft];
    if (hAircraft == NULL)   return;

    std::map<int, HTREEITEM> subNodes;

    auto& rules = CINI::Rules();
    auto& fadata = CINI::FAData();

    int i = 0;
    if (auto sides = fadata.GetSection("Sides"))
        for (auto& itr : sides->GetEntities())
            subNodes[i++] = this->InsertString(itr.second, -1, hAircraft);
    else
    {
        subNodes[i++] = this->InsertString("Allied", -1, hAircraft);
        subNodes[i++] = this->InsertString("Soviet", -1, hAircraft);
        subNodes[i++] = this->InsertString("Yuri", -1, hAircraft);
    }
    subNodes[-1] = this->InsertTranslatedString("OthObList", -1, hAircraft);

    auto& aircrafts = Variables::Rules.GetSection("AircraftTypes");
    for (auto& air : aircrafts)
    {
        if (IgnoreSet.find(air.second) != IgnoreSet.end())
            continue;
        int index = STDHelpers::ParseToInt(air.first, -1);
        if (index == -1)   continue;
        int side = GuessSide(air.second, Set_Aircraft);
        if (subNodes.find(side) == subNodes.end())
            side = -1;
        this->InsertString(
            QueryUIName(air.second),
            Const_Aircraft + index,
            subNodes[side]
        );
    }

    // Clear up
    if (ExtConfigs::BrowserRedraw_CleanUp)
    {
        for (auto& subnode : subNodes)
        {
            if (!this->ItemHasChildren(subnode.second))
                this->DeleteItem(subnode.second);
        }
    }
}

void ObjectBrowserControlExt::Redraw_Building()
{
    HTREEITEM& hBuilding = ExtNodes[Root_Building];
    if (hBuilding == NULL)   return;

    std::map<int, HTREEITEM> subNodes;

    auto& rules = CINI::Rules();
    auto& fadata = CINI::FAData();

    int i = 0;
    if (auto sides = fadata.GetSection("Sides"))
        for (auto& itr : sides->GetEntities())
            subNodes[i++] = this->InsertString(itr.second, -1, hBuilding);
    else
    {
        subNodes[i++] = this->InsertString("Allied", -1, hBuilding);
        subNodes[i++] = this->InsertString("Soviet", -1, hBuilding);
        subNodes[i++] = this->InsertString("Yuri", -1, hBuilding);
    }
    subNodes[-1] = this->InsertTranslatedString("OthObList", -1, hBuilding);

    auto& buildings = Variables::Rules.GetSection("BuildingTypes");
    for (auto& bud : buildings)
    {
        if (IgnoreSet.find(bud.second) != IgnoreSet.end())
            continue;
        int index = STDHelpers::ParseToInt(bud.first, -1);
        if (index == -1)   continue;
        int side = GuessSide(bud.second, Set_Building);
        if (subNodes.find(side) == subNodes.end())
            side = -1;
        this->InsertString(
            QueryUIName(bud.second),
            Const_Building + index,
            subNodes[side]
        );
    }

    // Clear up
    if (ExtConfigs::BrowserRedraw_CleanUp)
    {
        for (auto& subnode : subNodes)
        {
            if (!this->ItemHasChildren(subnode.second))
                this->DeleteItem(subnode.second);
        }
    }
}

void ObjectBrowserControlExt::Redraw_Terrain()
{
    HTREEITEM& hTerrain = ExtNodes[Root_Terrain];
    if (hTerrain == NULL)   return;

    auto& terrains = Variables::Rules.ParseIndicies("TerrainTypes", true);

    HTREEITEM hTree, hTrff, hSign, hLight, hOther;
    
    hTree = this->InsertTranslatedString("TreesObList", -1, hTerrain);
    hTrff = this->InsertTranslatedString("TrafficLightsObList", -1, hTerrain);
    hSign = this->InsertTranslatedString("SignsObList", -1, hTerrain);
    hLight = this->InsertTranslatedString("LightPostsObList", -1, hTerrain);
    hOther = this->InsertTranslatedString("OthObList", -1, hTerrain);
    
    this->InsertTranslatedString("RndTreeObList", 50999, hTree);

    for (size_t i = 0, sz = terrains.size(); i < sz; ++i)
    {
        FA2sp::Buffer = QueryUIName(terrains[i]);
        FA2sp::Buffer += "(" + terrains[i] + ")";
        if (IgnoreSet.find(terrains[i]) == IgnoreSet.end())
        {
            if (terrains[i].Find("TREE") >= 0)  this->InsertString(FA2sp::Buffer, Const_Terrain + i, hTree);
            else if (terrains[i].Find("TRFF") >= 0)  this->InsertString(FA2sp::Buffer, Const_Terrain + i, hTrff);
            else if (terrains[i].Find("SIGN") >= 0)  this->InsertString(FA2sp::Buffer, Const_Terrain + i, hSign);
            else if (terrains[i].Find("LT") >= 0)  this->InsertString(FA2sp::Buffer, Const_Terrain + i, hLight);
            else this->InsertString(FA2sp::Buffer, Const_Terrain + i, hOther);
        }
    }
}

void ObjectBrowserControlExt::Redraw_Smudge()
{
    HTREEITEM& hSmudge = ExtNodes[Root_Smudge];
    if (hSmudge == NULL)   return;

    auto& smudges = Variables::Rules.ParseIndicies("SmudgeTypes", true);
    for (size_t i = 0, sz = smudges.size(); i < sz; ++i)
    {
        if (IgnoreSet.find(smudges[i]) == IgnoreSet.end())
            this->InsertString(smudges[i], Const_Smudge + i, hSmudge);
    }
}

void ObjectBrowserControlExt::Redraw_Overlay()
{
    HTREEITEM& hOverlay = ExtNodes[Root_Overlay];
    if (hOverlay == NULL)   return;

    auto& rules = CINI::Rules();

    HTREEITEM hTemp;
    hTemp = this->InsertTranslatedString("DelOvrlObList", -1, hOverlay);
    this->InsertTranslatedString("DelOvrl0ObList", 60100, hTemp);
    this->InsertTranslatedString("DelOvrl1ObList", 60101, hTemp);
    this->InsertTranslatedString("DelOvrl2ObList", 60102, hTemp);
    this->InsertTranslatedString("DelOvrl3ObList", 60103, hTemp);

    hTemp = this->InsertTranslatedString("GrTibObList", -1, hOverlay);
    this->InsertTranslatedString("DrawTibObList", 60210, hTemp);
    this->InsertTranslatedString("DrawTib2ObList", 60310, hTemp);

    hTemp = this->InsertTranslatedString("BridgesObList", -1, hOverlay);
    this->InsertTranslatedString("BigBridgeObList", 60500, hTemp);
    this->InsertTranslatedString("SmallBridgeObList", 60501, hTemp);
    this->InsertTranslatedString("BigTrackBridgeObList", 60502, hTemp);
    this->InsertTranslatedString("SmallConcreteBridgeObList", 60503, hTemp);

    // Walls
    HTREEITEM hWalls = this->InsertTranslatedString("OthObList", -1, hOverlay);

    hTemp = this->InsertTranslatedString("AllObList", -1, hOverlay);

    this->InsertTranslatedString("OvrlManuallyObList", 60001, hOverlay);
    this->InsertTranslatedString("OvrlDataManuallyObList", 60002, hOverlay);

    if (!rules.SectionExists("OverlayTypes"))
        return;

    // a rough support for tracks
    this->InsertTranslatedString("Tracks", Const_Overlay + 39, hOverlay);
    
    MultimapHelper mmh;
    mmh.AddINI(&CINI::Rules());
    auto& overlays = mmh.ParseIndicies("OverlayTypes", true);
    for (size_t i = 0, sz = std::min<unsigned int>(overlays.size(), 255); i < sz; ++i)
    {
        CString buffer;
        buffer = QueryUIName(overlays[i]);
        buffer += "(" + overlays[i] + ")";
        if (rules.GetBool(overlays[i], "Wall"))
            this->InsertString(
                QueryUIName(overlays[i]),
                Const_Overlay + i,
                hWalls
            );
        if (IgnoreSet.find(overlays[i]) == IgnoreSet.end())
            this->InsertString(buffer, Const_Overlay + i, hTemp);
    }
}

void ObjectBrowserControlExt::Redraw_Waypoint()
{
    HTREEITEM& hWaypoint = ExtNodes[Root_Waypoint];
    if (hWaypoint == NULL)   return;

    this->InsertTranslatedString("CreateWaypObList", 20, hWaypoint);
    this->InsertTranslatedString("DelWaypObList", 21, hWaypoint);
    this->InsertTranslatedString("CreateSpecWaypObList", 22, hWaypoint);
}

void ObjectBrowserControlExt::Redraw_Celltag()
{
    HTREEITEM& hCellTag = ExtNodes[Root_Celltag];
    if (hCellTag == NULL)   return;

    this->InsertTranslatedString("CreateCelltagObList", 36, hCellTag);
    this->InsertTranslatedString("DelCelltagObList", 37, hCellTag);
    this->InsertTranslatedString("CelltagPropObList", 38, hCellTag);
}

void ObjectBrowserControlExt::Redraw_Basenode()
{
    HTREEITEM& hBasenode = ExtNodes[Root_Basenode];
    if (hBasenode == NULL)   return;

    this->InsertTranslatedString("CreateNodeNoDelObList", 40, hBasenode);
    this->InsertTranslatedString("CreateNodeDelObList", 41, hBasenode);
    this->InsertTranslatedString("DelNodeObList", 42, hBasenode);
}

void ObjectBrowserControlExt::Redraw_Tunnel()
{
    HTREEITEM& hTunnel = ExtNodes[Root_Tunnel];
    if (hTunnel == NULL)   return;

    if (CINI::FAData->GetBool("Debug", "AllowTunnels"))
    {
        this->InsertTranslatedString("NewTunnelObList", 50, hTunnel);
        this->InsertTranslatedString("DelTunnelObList", 51, hTunnel);
    }
}

void ObjectBrowserControlExt::Redraw_PlayerLocation()
{
    HTREEITEM& hPlayerLocation = ExtNodes[Root_PlayerLocation];
    if (hPlayerLocation == NULL)   return;

    this->InsertTranslatedString("StartpointsDelete", 21, hPlayerLocation);

    if (CMapData::Instance->IsMultiOnly())
    {
        for (int i = 0; i < 8; ++i)
        {
            FA2sp::Buffer.Format("Player %d", i);
            this->InsertString(FA2sp::Buffer, 23 + i, hPlayerLocation);
        }
    }
}

void ObjectBrowserControlExt::Redraw_PropertyBrush()
{
    HTREEITEM& hPropertyBrush = ExtNodes[Root_PropertyBrush];
    if (hPropertyBrush == NULL)    return;

    this->InsertTranslatedString("PropertyBrushBuilding", Const_PropertyBrush + Set_Building, hPropertyBrush);
    this->InsertTranslatedString("PropertyBrushInfantry", Const_PropertyBrush + Set_Infantry, hPropertyBrush);
    this->InsertTranslatedString("PropertyBrushVehicle", Const_PropertyBrush + Set_Vehicle, hPropertyBrush);
    this->InsertTranslatedString("PropertyBrushAircraft", Const_PropertyBrush + Set_Aircraft, hPropertyBrush);
}

bool ObjectBrowserControlExt::DoPropertyBrush_Building()
{
    if (this->BuildingBrushDlg.get() == nullptr)
        this->BuildingBrushDlg = std::make_unique<CPropertyBuilding>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

    for (auto& v : this->BuildingBrushBools)
        v = false;

    return this->BuildingBrushDlg->FA2CDialog::DoModal() == IDOK;
}

bool ObjectBrowserControlExt::DoPropertyBrush_Aircraft()
{
    if (this->AircraftBrushDlg.get() == nullptr)
        this->AircraftBrushDlg = std::make_unique<CPropertyAircraft>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

    for (auto& v : this->AircraftBrushBools)
        v = false;

    return this->AircraftBrushDlg->FA2CDialog::DoModal() == IDOK;
}

bool ObjectBrowserControlExt::DoPropertyBrush_Vehicle()
{
    if (this->VehicleBrushDlg.get() == nullptr)
        this->VehicleBrushDlg = std::make_unique<CPropertyUnit>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

    for (auto& v : this->VehicleBrushBools)
        v = false;

    return this->VehicleBrushDlg->FA2CDialog::DoModal() == IDOK;
}

bool ObjectBrowserControlExt::DoPropertyBrush_Infantry()
{
    if (this->InfantryBrushDlg.get() == nullptr)
        this->InfantryBrushDlg = std::make_unique<CPropertyInfantry>(CFinalSunDlg::Instance->MyViewFrame.pIsoView);

    for (auto& v : this->InfantryBrushBools)
        v = false;

    return this->InfantryBrushDlg->FA2CDialog::DoModal() == IDOK;
}

void ObjectBrowserControlExt::ApplyPropertyBrush(int X, int Y)
{
    int nIndex = CMapData::Instance->GetCoordIndex(X, Y);
    const auto& CellData = CMapData::Instance->CellDatas[nIndex];

    if (CIsoView::CurrentType == Set_Building)
    {
        if (CellData.Structure != -1)
            ApplyPropertyBrush_Building(CellData.Structure);
    }
    else if (CIsoView::CurrentType == Set_Infantry)
    {
        if (CellData.Infantry[0] != -1)
            ApplyPropertyBrush_Infantry(CellData.Infantry[0]);
        if (CellData.Infantry[1] != -1)
            ApplyPropertyBrush_Infantry(CellData.Infantry[1]);
        if (CellData.Infantry[2] != -1)
            ApplyPropertyBrush_Infantry(CellData.Infantry[2]);
    }
    else if (CIsoView::CurrentType == Set_Vehicle)
    {
        if (CellData.Unit != -1)
            ApplyPropertyBrush_Vehicle(CellData.Unit);
    }
    else if (CIsoView::CurrentType == Set_Aircraft)
    {
        if (CellData.Aircraft != -1)
            ApplyPropertyBrush_Aircraft(CellData.Aircraft);
    }
}

void ObjectBrowserControlExt::ApplyPropertyBrush_Building(int nIndex)
{
    CStructureData data;
    CMapData::Instance->QueryStructureData(nIndex, data);
    
    auto ApplyValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
    {
        if (BuildingBrushBools[nCheckBoxIdx - 1300])
        {
            if (!src.IsEmpty())
                dst = src;
        }
    };

    ApplyValue(1300, BuildingBrushDlg->CString_House, data.House);
    ApplyValue(1301, BuildingBrushDlg->CString_HealthPoint, data.Health);
    ApplyValue(1302, BuildingBrushDlg->CString_Direction, data.Facing);
    ApplyValue(1303, BuildingBrushDlg->CString_Sellable, data.AISellable);
    ApplyValue(1304, BuildingBrushDlg->CString_Rebuildable, data.AIRebuildable);
    ApplyValue(1305, BuildingBrushDlg->CString_EnergySupport, data.PoweredOn);
    ApplyValue(1306, BuildingBrushDlg->CString_UpgradeCount, data.Upgrades);
    ApplyValue(1307, BuildingBrushDlg->CString_Spotlight, data.SpotLight);
    ApplyValue(1308, BuildingBrushDlg->CString_Upgrade1, data.Upgrade1);
    ApplyValue(1309, BuildingBrushDlg->CString_Upgrade2, data.Upgrade2);
    ApplyValue(1310, BuildingBrushDlg->CString_Upgrade3, data.Upgrade3);
    ApplyValue(1311, BuildingBrushDlg->CString_AIRepairs, data.AIRepairable);
    ApplyValue(1312, BuildingBrushDlg->CString_ShowName, data.Nominal);
    ApplyValue(1313, BuildingBrushDlg->CString_Tag, data.Tag);

    CMapData::Instance->DeleteStructureData(nIndex);
    CMapData::Instance->SetStructureData(data, nullptr, nullptr, 0, "");

    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

void ObjectBrowserControlExt::ApplyPropertyBrush_Infantry(int nIndex)
{
    CInfantryData data;
    CMapData::Instance->QueryInfantryData(nIndex, data);

    auto ApplyValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
    {
        if (InfantryBrushBools[nCheckBoxIdx - 1300])
        {
            if (!src.IsEmpty())
                dst = src;
        }
    };

    ApplyValue(1300, InfantryBrushDlg->CString_House, data.House);
    ApplyValue(1301, InfantryBrushDlg->CString_HealthPoint, data.Health);
    ApplyValue(1302, InfantryBrushDlg->CString_State, data.Status);
    ApplyValue(1303, InfantryBrushDlg->CString_Direction, data.Facing);
    ApplyValue(1304, InfantryBrushDlg->CString_VerteranStatus, data.VeterancyPercentage);
    ApplyValue(1305, InfantryBrushDlg->CString_Group, data.Group);
    ApplyValue(1306, InfantryBrushDlg->CString_OnBridge, data.IsAboveGround);
    ApplyValue(1307, InfantryBrushDlg->CString_AutoCreateNoRecruitable, data.AutoNORecruitType);
    ApplyValue(1308, InfantryBrushDlg->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType);
    ApplyValue(1309, InfantryBrushDlg->CString_Tag, data.Tag);

    CMapData::Instance->DeleteInfantryData(nIndex);
    CMapData::Instance->SetInfantryData(data, nullptr, nullptr, 0, -1);

    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

void ObjectBrowserControlExt::ApplyPropertyBrush_Aircraft(int nIndex)
{
    CAircraftData data;
    CMapData::Instance->QueryAircraftData(nIndex, data);

    auto ApplyValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
    {
        if (AircraftBrushBools[nCheckBoxIdx - 1300])
        {
            if (!src.IsEmpty())
                dst = src;
        }
    };

    ApplyValue(1300, AircraftBrushDlg->CString_House, data.House);
    ApplyValue(1301, AircraftBrushDlg->CString_HealthPoint, data.Health);
    ApplyValue(1302, AircraftBrushDlg->CString_Direction, data.Facing);
    ApplyValue(1303, AircraftBrushDlg->CString_Status, data.Status);
    ApplyValue(1304, AircraftBrushDlg->CString_VeteranLevel, data.VeterancyPercentage);
    ApplyValue(1305, AircraftBrushDlg->CString_Group, data.Group);
    ApplyValue(1306, AircraftBrushDlg->CString_AutoCreateNoRecruitable, data.AutoNORecruitType);
    ApplyValue(1307, AircraftBrushDlg->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType);
    ApplyValue(1308, AircraftBrushDlg->CString_Tag, data.Tag);

    CMapData::Instance->DeleteAircraftData(nIndex);
    CMapData::Instance->SetAircraftData(data, nullptr, nullptr, 0, "");

    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

void ObjectBrowserControlExt::ApplyPropertyBrush_Vehicle(int nIndex)
{
    CUnitData data;
    CMapData::Instance->QueryUnitData(nIndex, data);

    auto ApplyValue = [&](int nCheckBoxIdx, ppmfc::CString& src, ppmfc::CString& dst)
    {
        if (VehicleBrushBools[nCheckBoxIdx - 1300])
        {
            if (!src.IsEmpty())
                dst = src;
        }
    };

    ApplyValue(1300, VehicleBrushDlg->CString_House, data.House);
    ApplyValue(1301, VehicleBrushDlg->CString_HealthPoint, data.Health);
    ApplyValue(1302, VehicleBrushDlg->CString_State, data.Status);
    ApplyValue(1303, VehicleBrushDlg->CString_Direction, data.Facing);
    ApplyValue(1304, VehicleBrushDlg->CString_VeteranLevel, data.VeterancyPercentage);
    ApplyValue(1305, VehicleBrushDlg->CString_Group, data.Group);
    ApplyValue(1306, VehicleBrushDlg->CString_OnBridge, data.IsAboveGround);
    ApplyValue(1307, VehicleBrushDlg->CString_FollowerID, data.FollowsIndex);
    ApplyValue(1308, VehicleBrushDlg->CString_AutoCreateNoRecruitable, data.AutoNORecruitType);
    ApplyValue(1309, VehicleBrushDlg->CString_AutoCreateYesRecruitable, data.AutoYESRecruitType);
    ApplyValue(1310, VehicleBrushDlg->CString_Tag, data.Tag);

    CMapData::Instance->DeleteUnitData(nIndex);
    CMapData::Instance->SetUnitData(data, nullptr, nullptr, 0, "");

    ::RedrawWindow(CFinalSunDlg::Instance->MyViewFrame.pIsoView->m_hWnd, 0, 0, RDW_UPDATENOW | RDW_INVALIDATE);
}

int ObjectBrowserControlExt::GuessType(const char* pRegName)
{
    if (ExtSets[Set_Building].find(pRegName) != ExtSets[Set_Building].end())
        return Set_Building;
    if (ExtSets[Set_Infantry].find(pRegName) != ExtSets[Set_Infantry].end())
        return Set_Infantry;
    if (ExtSets[Set_Vehicle].find(pRegName) != ExtSets[Set_Vehicle].end())
        return Set_Vehicle;
    if (ExtSets[Set_Aircraft].find(pRegName) != ExtSets[Set_Aircraft].end())
        return Set_Aircraft;
    return -1;
}

int ObjectBrowserControlExt::GuessSide(const char* pRegName, int nType)
{
    auto& knownIterator = KnownItem.find(pRegName);
    if (knownIterator != KnownItem.end())
        return knownIterator->second;

    int result = -1;
    switch (nType)
    {
    case -1:
    default:
        break;
    case Set_Building:
        result = GuessBuildingSide(pRegName);
        break;
    case Set_Infantry:
        result = GuessGenericSide(pRegName, Set_Infantry);
        break;
    case Set_Vehicle:
        result = GuessGenericSide(pRegName, Set_Vehicle);
        break;
    case Set_Aircraft:
        result = GuessGenericSide(pRegName, Set_Aircraft);
        break;
    }
    KnownItem[pRegName] = result;
    return result;
}

int ObjectBrowserControlExt::GuessBuildingSide(const char* pRegName)
{
    auto& rules = CINI::Rules();

    int planning;
    planning = rules.GetInteger(pRegName, "AIBasePlanningSide", -1);
    if (planning >= rules.GetKeyCount("Sides"))
        return -1;
    if (planning >= 0)
        return planning;
    auto& cons = STDHelpers::SplitString(rules.GetString("AI", "BuildConst"));
    int i;
    for (i = 0; i < cons.size(); ++i)
    {
        if (cons[i] == pRegName)
            return i;
    }
    if (i >= rules.GetKeyCount("Sides"))
        return -1;
    return GuessGenericSide(pRegName, Set_Building);
}

int ObjectBrowserControlExt::GuessGenericSide(const char* pRegName, int nType)
{
    const auto& set = ExtSets[nType];

    if (set.find(pRegName) == set.end())
        return -1;

    switch (ExtConfigs::BrowserRedraw_GuessMode)
    {
    default:
    case 0:
    {
        for (auto& prep : STDHelpers::SplitString(Variables::Rules.GetString(pRegName, "Prerequisite")))
        {
            int guess = -1;
            for (auto& subprep : STDHelpers::SplitString(Variables::Rules.GetString("GenericPrerequisites", prep)))
            {
                guess = GuessSide(subprep, GuessType(subprep));
                if (guess != -1)
                    return guess;
            }
            guess = GuessSide(prep, GuessType(prep));
            if (guess != -1)
                return guess;
        }
        return -1;
    }
    case 1:
    {
        auto& owners = STDHelpers::SplitString(Variables::Rules.GetString(pRegName, "Owner"));
        if (owners.size() <= 0)
            return -1;
        auto& itr = Owners.find(owners[0]);
        if (itr == Owners.end())
            return -1;
        return itr->second;
    }
    }
}

// ObjectBrowserControlExt::OnSelectChanged
void ObjectBrowserControlExt::OnExeTerminate()
{
    IgnoreSet.clear();
    ForceName.clear();
    for (auto& set : ExtSets)
        set.clear();
    KnownItem.clear();
    Owners.clear();
}

bool ObjectBrowserControlExt::UpdateEngine(int nData)
{
    do
    {
        int nMorphable = nData - 67;
        if (nMorphable >= 0 && nMorphable < TheaterInfo::CurrentInfo.size())
        {
            int i;
            for (i = 0; i < *CTileTypeClass::InstanceCount; ++i)
                if ((*CTileTypeClass::Instance)[i].TileSet == TheaterInfo::CurrentInfo[nMorphable].Morphable)
                {
                    CIsoView::CurrentParam = 0;
                    CIsoView::CurrentHeight = 0;
                    CIsoView::CurrentType = i;
                    CIsoView::CurrentCommand = FACurrentCommand::TileDraw;
                    CBrushSize::UpdateBrushSize(i);
                    return true;
                }
        }
    } while (false);
    

    int nCode = nData / 10000;
    nData %= 10000;

    if (nCode == 9) // PropertyBrush
    {
        if (nData == Set_Building)
        {
            ObjectBrowserControlExt::InitPropertyDlgFromProperty = true;

            if (this->DoPropertyBrush_Building())
            {
                CIsoView::CurrentCommand = 0x17; // PropertyBrush
                CIsoView::CurrentType = Set_Building;
            }
            else
                CIsoView::CurrentCommand = FACurrentCommand::Nothing;

            ObjectBrowserControlExt::InitPropertyDlgFromProperty = false;

            return true;
        }
        else if (nData == Set_Infantry)
        {
            ObjectBrowserControlExt::InitPropertyDlgFromProperty = true;

            if (this->DoPropertyBrush_Infantry())
            {
                CIsoView::CurrentCommand = 0x17;
                CIsoView::CurrentType = Set_Infantry;
            }
            else
                CIsoView::CurrentCommand = FACurrentCommand::Nothing;

            ObjectBrowserControlExt::InitPropertyDlgFromProperty = false;

            return true;
        }
        else if (nData == Set_Vehicle)
        {
            ObjectBrowserControlExt::InitPropertyDlgFromProperty = true;

            if (this->DoPropertyBrush_Vehicle())
            {
                CIsoView::CurrentCommand = 0x17;
                CIsoView::CurrentType = Set_Vehicle;
            }
            else
                CIsoView::CurrentCommand = FACurrentCommand::Nothing;

            ObjectBrowserControlExt::InitPropertyDlgFromProperty = false;

            return true;
        }
        else if (nData == Set_Aircraft)
        {
            ObjectBrowserControlExt::InitPropertyDlgFromProperty = true;

            if (this->DoPropertyBrush_Aircraft())
            {
                CIsoView::CurrentCommand = 0x17;
                CIsoView::CurrentType = Set_Aircraft;
            }
            else
                CIsoView::CurrentCommand = FACurrentCommand::Nothing;

            ObjectBrowserControlExt::InitPropertyDlgFromProperty = false;

            return true;
        }
    }

    return false;
}