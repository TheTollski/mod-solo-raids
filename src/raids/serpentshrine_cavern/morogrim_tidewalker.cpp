#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_MOROGRIM_TIDEWALKER = 21213;
constexpr uint32 SPELL_TIDAL_WAVE = 37730;
constexpr uint32 SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN = 548;

std::set<ObjectGuid> morogrimSoloAnnouncementSent;

void AnnounceMorogrimSoloTweaks(Creature* morogrim)
{
    if (!morogrim)
        return;

    ObjectGuid const guid = morogrim->GetGUID();
    if (morogrimSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(morogrim->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN);
    if (!player)
        return;

    uint32 const durationPct = uint32(SoloRaids::Config::MorogrimTidalWaveDurationPct() * 100.0f);
    player->SendSystemMessage(("mod-solo-raids active: Serpentshrine Cavern solo tweaks enabled for Morogrim Tidewalker. Tidal Wave debuff duration set to " + std::to_string(durationPct) + "%.").c_str());
    morogrimSoloAnnouncementSent.insert(guid);
}
}

class MorogrimTidewalkerSoloRaidCreatureScript : public AllCreatureScript
{
public:
    MorogrimTidewalkerSoloRaidCreatureScript() : AllCreatureScript("MorogrimTidewalkerSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_MOROGRIM_TIDEWALKER || !creature->IsInCombat() ||
            !SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
            return;

        AnnounceMorogrimSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_MOROGRIM_TIDEWALKER)
            morogrimSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class MorogrimTidewalkerSoloRaidSpellScript : public AllSpellScript
{
public:
    MorogrimTidewalkerSoloRaidSpellScript() : AllSpellScript("MorogrimTidewalkerSoloRaidSpellScript", { ALLSPELLHOOK_ON_CALC_MAX_DURATION }) { }

    void OnCalcMaxDuration(Aura const* aura, int32& maxDuration) override
    {
        if (!aura || aura->GetType() != UNIT_AURA_TYPE || aura->GetId() != SPELL_TIDAL_WAVE || maxDuration <= 0 ||
            !SoloRaids::IsSoloPlayer(aura->GetUnitOwner(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
            return;

        maxDuration = int32(float(maxDuration) * SoloRaids::Config::MorogrimTidalWaveDurationPct());
    }
};

void AddMorogrimTidewalkerSoloRaidScripts()
{
    new MorogrimTidewalkerSoloRaidCreatureScript();
    new MorogrimTidewalkerSoloRaidSpellScript();
}
