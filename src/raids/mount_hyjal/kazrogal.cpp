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
constexpr uint32 NPC_KAZROGAL = 17888;
constexpr uint32 SPELL_CRIPPLE = 31477;
constexpr uint32 SPELL_MARK_OF_KAZROGAL = 31447;
constexpr uint32 SOLO_RAIDS_MAP_MOUNT_HYJAL = 534;

std::set<ObjectGuid> kazrogalSoloAnnouncementSent;

void AnnounceKazrogalSoloTweaks(Creature* kazrogal)
{
    if (!kazrogal)
        return;

    ObjectGuid const guid = kazrogal->GetGUID();
    if (kazrogalSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(kazrogal->GetMap(), SOLO_RAIDS_MAP_MOUNT_HYJAL);
    if (!player)
        return;

    std::string message = "mod-solo-raids active: Mount Hyjal solo tweaks enabled for Kaz'rogal.";
    bool hasTweaks = false;

    float const markDurationPct = SoloRaids::Config::KazrogalMarkDurationPct();
    if (markDurationPct != 1.0f)
    {
        message += " Mark of Kaz'rogal duration set to " + std::to_string(uint32(markDurationPct * 100.0f)) + "%.";
        hasTweaks = true;
    }

    float const crippleDurationPct = SoloRaids::Config::KazrogalCrippleDurationPct();
    if (crippleDurationPct != 1.0f)
    {
        message += " Cripple duration set to " + std::to_string(uint32(crippleDurationPct * 100.0f)) + "%.";
        hasTweaks = true;
    }

    if (!hasTweaks)
        return;

    player->SendSystemMessage(message.c_str());
    kazrogalSoloAnnouncementSent.insert(guid);
}
}

class KazrogalSoloRaidCreatureScript : public AllCreatureScript
{
public:
    KazrogalSoloRaidCreatureScript() : AllCreatureScript("KazrogalSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_KAZROGAL && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_MOUNT_HYJAL))
            AnnounceKazrogalSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_KAZROGAL)
            kazrogalSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class KazrogalSoloRaidSpellScript : public AllSpellScript
{
public:
    KazrogalSoloRaidSpellScript() : AllSpellScript("KazrogalSoloRaidSpellScript", { ALLSPELLHOOK_ON_CALC_MAX_DURATION }) { }

    void OnCalcMaxDuration(Aura const* aura, int32& maxDuration) override
    {
        if (!aura || aura->GetType() != UNIT_AURA_TYPE || maxDuration <= 0 ||
            (aura->GetId() != SPELL_MARK_OF_KAZROGAL && aura->GetId() != SPELL_CRIPPLE) ||
            !SoloRaids::IsSoloPlayer(aura->GetUnitOwner(), SOLO_RAIDS_MAP_MOUNT_HYJAL))
            return;

        Unit const* caster = aura->GetCaster();
        if (!caster || caster->GetEntry() != NPC_KAZROGAL)
            return;

        if (aura->GetId() == SPELL_MARK_OF_KAZROGAL)
            maxDuration = int32(float(maxDuration) * SoloRaids::Config::KazrogalMarkDurationPct());
        else if (aura->GetId() == SPELL_CRIPPLE)
            maxDuration = int32(float(maxDuration) * SoloRaids::Config::KazrogalCrippleDurationPct());
    }
};

void AddKazrogalSoloRaidScripts()
{
    new KazrogalSoloRaidCreatureScript();
    new KazrogalSoloRaidSpellScript();
}
