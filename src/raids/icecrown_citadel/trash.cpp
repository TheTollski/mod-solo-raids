#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "UnitScript.h"

#include <algorithm>
#include <set>
#include <string>

namespace
{
constexpr uint32 SPELL_MORTAL_WOUND = 71127;
constexpr uint32 SPELL_COMBOBULATING_SPRAY = 71103;
constexpr uint32 SOLO_RAIDS_MAP_ICECROWN_CITADEL = 631;

std::set<uint32> icecrownCitadelTrashAnnouncementMaps;

void AnnounceIcecrownCitadelTrashTweaks(Player* player)
{
    if (!player)
        return;

    uint32 const instanceId = player->GetInstanceId();
    if (icecrownCitadelTrashAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Icecrown Citadel solo trash tweaks enabled. Mortal Wound capped at " +
        std::to_string(uint32(SoloRaids::Config::IcecrownCitadelMortalWoundMaxStacks())) +
        " stacks. Combobulating Spray duration set to " +
        std::to_string(uint32(SoloRaids::Config::IcecrownCitadelCombobulatingSprayDurationPct() * 100.0f)) + "%.").c_str());
    icecrownCitadelTrashAnnouncementMaps.insert(instanceId);
}

void CapMortalWound(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_ICECROWN_CITADEL))
        return;

    Aura* aura = unit->GetAura(SPELL_MORTAL_WOUND);
    if (!aura)
        return;

    uint8 const maxStacks = SoloRaids::Config::IcecrownCitadelMortalWoundMaxStacks();
    if (maxStacks == 0)
    {
        unit->RemoveAurasDueToSpell(SPELL_MORTAL_WOUND);
        return;
    }

    if (aura->GetStackAmount() > maxStacks)
        aura->SetStackAmount(maxStacks);
}

void ScaleCombobulatingSprayDuration(Unit* unit, Aura* aura)
{
    if (!unit || !aura || aura->GetId() != SPELL_COMBOBULATING_SPRAY ||
        !SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_ICECROWN_CITADEL) ||
        aura->GetMaxDuration() <= 0)
        return;

    float const durationPct = SoloRaids::Config::IcecrownCitadelCombobulatingSprayDurationPct();
    if (durationPct == 1.0f)
        return;

    int32 duration = int32(float(aura->GetMaxDuration()) * durationPct);
    if (durationPct > 0.0f)
        duration = std::max<int32>(duration, 1);

    aura->SetMaxDuration(duration);
    aura->SetDuration(duration);
}
}

class IcecrownCitadelTrashSoloRaidUnitScript : public UnitScript
{
public:
    IcecrownCitadelTrashSoloRaidUnitScript() : UnitScript("IcecrownCitadelTrashSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !aura)
            return;

        if (Player* player = unit->ToPlayer())
        {
            if (SoloRaids::IsSoloPlayer(player, SOLO_RAIDS_MAP_ICECROWN_CITADEL) &&
                (aura->GetId() == SPELL_MORTAL_WOUND || aura->GetId() == SPELL_COMBOBULATING_SPRAY))
                AnnounceIcecrownCitadelTrashTweaks(player);
        }

        if (aura->GetId() == SPELL_MORTAL_WOUND)
            CapMortalWound(unit);
        else if (aura->GetId() == SPELL_COMBOBULATING_SPRAY)
            ScaleCombobulatingSprayDuration(unit, aura);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        CapMortalWound(unit);
    }
};

void AddIcecrownCitadelTrashSoloRaidScripts()
{
    new IcecrownCitadelTrashSoloRaidUnitScript();
}
