#include "StdInc.h"
#include "FurnitureSubGroup_c.h"
#include "FurnitureManager_c.h"

void FurnitureSubGroup_c::InjectHooks() {
    RH_ScopedClass(FurnitureSubGroup_c);
    RH_ScopedCategory("Interior");

    RH_ScopedInstall(GetFurniture, 0x590EE0);
    RH_ScopedInstall(GetRandomId, 0x590FD0);
    RH_ScopedInstall(AddFurniture, 0x5C00C0);
}

// 0x5C00C0
bool FurnitureSubGroup_c::AddFurniture(uint16 modelId, int16 id, uint8 wealthMin, uint8 wealthMax, uint8 maxAng) {
    const auto f = FurnitureManager_c::NewFurniture();
    if (!f) {
        return false;
    }

    f->m_nModelId = modelId;
    f->m_nId      = id;

    const auto& bb   = CModelInfo::GetModelInfo(modelId)->GetColModel()->GetBoundingBox();
    const auto  size = m_bCanSteal
        ? CVector2D{ bb.m_vecMax - bb.m_vecMin }
        : CVector2D{ bb.m_vecMax } + CVector2D{ 0.5f };

    // Round up to whole tiles, but ignore anything under 0.02
    const auto ToTiles = [](float s) {
        const auto tiles = (int32)s;
        return (int8)(s - (float)tiles >= 0.02f ? tiles + 1 : tiles);
    };
    f->m_nWidthX = ToTiles(size.x);
    f->m_nWidthY = ToTiles(size.y);

    f->m_nWealthMin               = wealthMin;
    f->m_nWealthMax               = wealthMax;
    f->m_nMaxAng                  = maxAng;
    f->m_bCanPlaceInFrontOfWindow = m_bCanPlaceInFrontOfWindow;
    f->m_bIsTall                  = m_bIsTall;
    f->m_bCanSteal                = m_bCanSteal;

    m_Furnitures.AddItem(f);

    return true;
}

// 0x590EE0
Furniture_c* FurnitureSubGroup_c::GetFurniture(int16 id, uint8 wealth) {
    if (id >= 0) {
        for (auto& f : m_Furnitures) {
            if (f.m_nId == id) {
                return &f;
            }
        }
        return nullptr;
    }

    // NOTE: The original also has a branch for `wealth == -1` that picks from all furniture,
    //       but `wealth` is promoted from `uint8` there, so it's never taken.

    auto candidates = m_Furnitures | rngv::filter([wealth](const Furniture_c& f) {
        return f.m_nWealthMin <= wealth && wealth <= f.m_nWealthMax;
    });
    const auto numCandidates = (int32)rng::distance(candidates);
    if (!numCandidates) {
        return nullptr;
    }
    return &*rng::next(candidates.begin(), CGeneral::GetRandomNumberInRange(0, numCandidates));
}

// 0x590FD0
int32 FurnitureSubGroup_c::GetRandomId(uint8 wealth) {
    // NOTE: The original duplicates the code of `GetFurniture` here
    const auto f = GetFurniture(-1, wealth);
    return f ? f->m_nId : -1;
}

void FurnitureSubGroup_c::Exit() {
    m_Furnitures.RemoveAll();
}
