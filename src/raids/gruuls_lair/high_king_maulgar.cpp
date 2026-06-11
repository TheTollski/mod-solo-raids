#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_HIGH_KING_MAULGAR = 18831;
constexpr uint32 NPC_OLM_THE_SUMMONER = 18834;
constexpr uint32 NPC_BLINDEYE_THE_SEER = 18836;
constexpr uint32 NPC_WILD_FEL_STALKER = 18847;
constexpr uint32 SPELL_SUMMON_WILD_FELHUNTER = 33131;
constexpr uint32 SOLO_RAIDS_MAP_GRUULS_LAIR = 565;

std::set<uint32> highKingMaulgarSoloAnnouncementMaps;
std::map<ObjectGuid, uint32> activeSoloOlmFelhoundInstances;

uint32 CountActiveOlmFelhounds(uint32 instanceId)
{
    uint32 count = 0;
    for (auto const& felhound : activeSoloOlmFelhoundInstances)
        if (felhound.second == instanceId)
            ++count;

    return count;
}

void LimitSoloOlmFelhounds(Creature* felhound)
{
    if (!felhound || felhound->GetEntry() != NPC_WILD_FEL_STALKER ||
        !SoloRaids::IsSoloMap(felhound->GetMap(), SOLO_RAIDS_MAP_GRUULS_LAIR))
        return;

    ObjectGuid const guid = felhound->GetGUID();
    if (activeSoloOlmFelhoundInstances.count(guid) != 0)
        return;

    uint32 const instanceId = felhound->GetInstanceId();
    if (CountActiveOlmFelhounds(instanceId) >= SoloRaids::Config::HighKingMaulgarOlmMaxFelhounds())
    {
        felhound->DespawnOrUnsummon(Milliseconds(100));
        return;
    }

    activeSoloOlmFelhoundInstances[guid] = instanceId;
}

void AnnounceHighKingMaulgarSoloTweaks(Creature* maulgar)
{
    if (!maulgar)
        return;

    Player* player = SoloRaids::GetSoloPlayer(maulgar->GetMap(), SOLO_RAIDS_MAP_GRUULS_LAIR);
    if (!player)
        return;

    uint32 const instanceId = maulgar->GetInstanceId();
    if (highKingMaulgarSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    uint32 const healingPct = uint32(SoloRaids::Config::HighKingMaulgarBlindeyeHealingPct() * 100.0f);
    uint32 const maxFelhounds = SoloRaids::Config::HighKingMaulgarOlmMaxFelhounds();
    std::string message = "mod-solo-raids active: Gruul's Lair solo tweaks enabled for High King Maulgar. Blindeye healing set to " +
        std::to_string(healingPct) + "%. Olm's active felhounds capped at " + std::to_string(maxFelhounds) + ".";
    player->SendSystemMessage(message.c_str());
    highKingMaulgarSoloAnnouncementMaps.insert(instanceId);
}
}

class HighKingMaulgarSoloRaidCreatureScript : public AllCreatureScript
{
public:
    HighKingMaulgarSoloRaidCreatureScript() : AllCreatureScript("HighKingMaulgarSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (creature->GetEntry() == NPC_HIGH_KING_MAULGAR && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_GRUULS_LAIR))
            AnnounceHighKingMaulgarSoloTweaks(creature);

        if (creature->GetEntry() == NPC_WILD_FEL_STALKER)
            LimitSoloOlmFelhounds(creature);
    }

    void OnCreatureAddWorld(Creature* creature) override
    {
        LimitSoloOlmFelhounds(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature)
            return;

        if (creature->GetEntry() == NPC_HIGH_KING_MAULGAR)
            highKingMaulgarSoloAnnouncementMaps.erase(creature->GetInstanceId());

        activeSoloOlmFelhoundInstances.erase(creature->GetGUID());
    }
};

class HighKingMaulgarSoloRaidUnitScript : public UnitScript
{
public:
    HighKingMaulgarSoloRaidUnitScript() : UnitScript("HighKingMaulgarSoloRaidUnitScript", true, { UNITHOOK_ON_HEAL }) { }

    void OnHeal(Unit* healer, Unit* /*receiver*/, uint32& gain) override
    {
        if (!healer || healer->GetEntry() != NPC_BLINDEYE_THE_SEER ||
            !SoloRaids::IsSoloMap(healer->GetMap(), SOLO_RAIDS_MAP_GRUULS_LAIR))
            return;

        gain = uint32(float(gain) * SoloRaids::Config::HighKingMaulgarBlindeyeHealingPct());
    }
};

class HighKingMaulgarSoloRaidSpellScript : public AllSpellScript
{
public:
    HighKingMaulgarSoloRaidSpellScript() : AllSpellScript("HighKingMaulgarSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_SUMMON_WILD_FELHUNTER)
            return;

        Unit* caster = spell->GetCaster();
        if (!caster || caster->GetEntry() != NPC_OLM_THE_SUMMONER ||
            !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_GRUULS_LAIR))
            return;

        if (CountActiveOlmFelhounds(caster->GetInstanceId()) >= SoloRaids::Config::HighKingMaulgarOlmMaxFelhounds())
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddHighKingMaulgarSoloRaidScripts()
{
    new HighKingMaulgarSoloRaidCreatureScript();
    new HighKingMaulgarSoloRaidUnitScript();
    new HighKingMaulgarSoloRaidSpellScript();
}
