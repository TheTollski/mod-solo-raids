#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "Map.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_THE_LICH_KING = 36597;
constexpr uint32 NPC_THE_LICH_KING_10H = 39166;
constexpr uint32 NPC_THE_LICH_KING_25N = 39167;
constexpr uint32 NPC_THE_LICH_KING_25H = 39168;
constexpr uint32 NPC_DRUDGE_GHOUL = 37695;
constexpr uint32 NPC_DRUDGE_GHOUL_10H = 39309;
constexpr uint32 NPC_DRUDGE_GHOUL_25N = 39310;
constexpr uint32 NPC_DRUDGE_GHOUL_25H = 39311;
constexpr uint32 NPC_SHAMBLING_HORROR = 37698;
constexpr uint32 NPC_SHAMBLING_HORROR_10H = 39299;
constexpr uint32 NPC_SHAMBLING_HORROR_25N = 39300;
constexpr uint32 NPC_SHAMBLING_HORROR_25H = 39301;
constexpr uint32 NPC_ICE_SPHERE = 36633;
constexpr uint32 SPELL_REMORSELESS_WINTER_10N = 68981;
constexpr uint32 SPELL_REMORSELESS_WINTER_25N = 74270;
constexpr uint32 SPELL_REMORSELESS_WINTER_10H = 74271;
constexpr uint32 SPELL_REMORSELESS_WINTER_25H = 74272;
constexpr uint32 SPELL_REMORSELESS_WINTER_PHASE_TWO = 72259;
constexpr uint32 SPELL_REMORSELESS_WINTER_PHASE_TWO_25N = 74273;
constexpr uint32 SPELL_REMORSELESS_WINTER_PHASE_TWO_10H = 74274;
constexpr uint32 SPELL_REMORSELESS_WINTER_PHASE_TWO_25H = 74275;
constexpr uint32 SPELL_REMORSELESS_WINTER_DAMAGE = 68983;
constexpr uint32 SOLO_RAIDS_MAP_ICECROWN_CITADEL = 631;

struct HealthScaleState
{
    uint32 baselineMaxHealth;
    uint32 appliedMaxHealth;
};

std::map<ObjectGuid, HealthScaleState> lichKingAddHealthScaleStates;
std::set<uint32> lichKingSoloAnnouncementInstances;

bool IsTheLichKing(uint32 entry)
{
    return entry == NPC_THE_LICH_KING ||
        entry == NPC_THE_LICH_KING_10H ||
        entry == NPC_THE_LICH_KING_25N ||
        entry == NPC_THE_LICH_KING_25H;
}

bool IsDrudgeGhoul(uint32 entry)
{
    return entry == NPC_DRUDGE_GHOUL ||
        entry == NPC_DRUDGE_GHOUL_10H ||
        entry == NPC_DRUDGE_GHOUL_25N ||
        entry == NPC_DRUDGE_GHOUL_25H;
}

bool IsShamblingHorror(uint32 entry)
{
    return entry == NPC_SHAMBLING_HORROR ||
        entry == NPC_SHAMBLING_HORROR_10H ||
        entry == NPC_SHAMBLING_HORROR_25N ||
        entry == NPC_SHAMBLING_HORROR_25H;
}

bool IsScaledLichKingAdd(Creature const* creature)
{
    return creature && (IsDrudgeGhoul(creature->GetEntry()) || IsShamblingHorror(creature->GetEntry()));
}

bool IsIceSphere(Creature const* creature)
{
    return creature && creature->GetEntry() == NPC_ICE_SPHERE;
}

bool IsRemorselessWinter(uint32 spellId)
{
    return spellId == SPELL_REMORSELESS_WINTER_10N ||
        spellId == SPELL_REMORSELESS_WINTER_25N ||
        spellId == SPELL_REMORSELESS_WINTER_10H ||
        spellId == SPELL_REMORSELESS_WINTER_25H ||
        spellId == SPELL_REMORSELESS_WINTER_PHASE_TWO ||
        spellId == SPELL_REMORSELESS_WINTER_PHASE_TWO_25N ||
        spellId == SPELL_REMORSELESS_WINTER_PHASE_TWO_10H ||
        spellId == SPELL_REMORSELESS_WINTER_PHASE_TWO_25H ||
        spellId == SPELL_REMORSELESS_WINTER_DAMAGE;
}

uint32 CountRaidPlayers(Map const* map)
{
    if (!map || !map->IsRaid())
        return 0;

    uint32 count = 0;
    for (Map::PlayerList::const_iterator itr = map->GetPlayers().begin(); itr != map->GetPlayers().end(); ++itr)
    {
        Player* player = itr->GetSource();
        if (player && !player->IsGameMaster())
            ++count;
    }

    return count;
}

float GetRaidSizeDamageMultiplier(Map const* map)
{
    uint32 const playerCount = CountRaidPlayers(map);
    if (playerCount == 0)
        return 1.0f;

    uint32 const raidSize = map->Is25ManRaid() ? 25 : 10;
    return float(raidSize) / float(playerCount);
}

float GetAddMaxHealthPct(Creature const* creature)
{
    if (!creature)
        return 1.0f;

    if (IsDrudgeGhoul(creature->GetEntry()))
        return SoloRaids::Config::LichKingDrudgeGhoulMaxHealthPct();

    if (IsShamblingHorror(creature->GetEntry()))
        return SoloRaids::Config::LichKingShamblingHorrorMaxHealthPct();

    return 1.0f;
}

void SetMaxHealthPreservingPct(Creature* creature, uint32 maxHealth)
{
    if (!creature || creature->GetMaxHealth() == maxHealth)
        return;

    float const healthPct = creature->GetMaxHealth() > 0
        ? float(creature->GetHealth()) / float(creature->GetMaxHealth())
        : 1.0f;

    creature->SetMaxHealth(maxHealth);
    uint32 newHealth = uint32(float(maxHealth) * healthPct);
    if (creature->IsAlive())
        newHealth = std::max<uint32>(1, newHealth);
    creature->SetHealth(std::min(maxHealth, newHealth));
}

void ScaleLichKingAddMaxHealth(Creature* creature)
{
    if (!IsScaledLichKingAdd(creature))
        return;

    ObjectGuid const guid = creature->GetGUID();
    auto stateItr = lichKingAddHealthScaleStates.find(guid);

    if (!SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
    {
        if (stateItr != lichKingAddHealthScaleStates.end())
        {
            SetMaxHealthPreservingPct(creature, stateItr->second.baselineMaxHealth);
            lichKingAddHealthScaleStates.erase(stateItr);
        }
        return;
    }

    if (stateItr == lichKingAddHealthScaleStates.end())
    {
        uint32 const currentMaxHealth = creature->GetMaxHealth();
        stateItr = lichKingAddHealthScaleStates.emplace(
            guid, HealthScaleState{ currentMaxHealth, currentMaxHealth }).first;
    }
    else if (creature->GetMaxHealth() != stateItr->second.appliedMaxHealth &&
        creature->GetMaxHealth() != stateItr->second.baselineMaxHealth)
    {
        stateItr->second.baselineMaxHealth = creature->GetMaxHealth();
    }

    uint32 const scaledMaxHealth = std::max<uint32>(
        1, uint32(float(stateItr->second.baselineMaxHealth) * GetAddMaxHealthPct(creature)));
    stateItr->second.appliedMaxHealth = scaledMaxHealth;
    SetMaxHealthPreservingPct(creature, scaledMaxHealth);
}

void AnnounceLichKingSoloTweaks(Creature* lichKing)
{
    if (!lichKing)
        return;

    uint32 const instanceId = lichKing->GetInstanceId();
    if (lichKingSoloAnnouncementInstances.count(instanceId) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(lichKing->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL);
    if (!player)
        return;

    std::string message = "mod-solo-raids active: Icecrown Citadel solo tweaks enabled for The Lich King. Drudge Ghoul max health set to " +
        std::to_string(uint32(SoloRaids::Config::LichKingDrudgeGhoulMaxHealthPct() * 100.0f)) +
        "% and Shambling Horror max health set to " +
        std::to_string(uint32(SoloRaids::Config::LichKingShamblingHorrorMaxHealthPct() * 100.0f)) +
        "% after other scaling.";

    if (SoloRaids::Config::LichKingRestoreRemorselessWinterDamage())
        message += " Remorseless Winter damage restored using raid size divided by active raid players.";

    if (SoloRaids::Config::DisableLichKingIceSpheres())
        message += " Ice Spheres disabled.";

    player->SendSystemMessage(message.c_str());
    lichKingSoloAnnouncementInstances.insert(instanceId);
}
}

class LichKingSoloRaidCreatureScript : public AllCreatureScript
{
public:
    LichKingSoloRaidCreatureScript() : AllCreatureScript("LichKingSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature)
            return;

        if (SoloRaids::Config::DisableLichKingIceSpheres() &&
            IsIceSphere(creature) &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
        {
            creature->DespawnOrUnsummon();
            return;
        }

        if (IsScaledLichKingAdd(creature))
        {
            ScaleLichKingAddMaxHealth(creature);
            return;
        }

        if (IsTheLichKing(creature->GetEntry()) && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            AnnounceLichKingSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature)
            return;

        lichKingAddHealthScaleStates.erase(creature->GetGUID());

        if (IsTheLichKing(creature->GetEntry()))
            lichKingSoloAnnouncementInstances.erase(creature->GetInstanceId());
    }
};

class LichKingSoloRaidUnitScript : public UnitScript
{
public:
    LichKingSoloRaidUnitScript() : UnitScript("LichKingSoloRaidUnitScript", true,
        { UNITHOOK_MODIFY_PERIODIC_DAMAGE_AURAS_TICK, UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifyPeriodicDamageAurasTick(Unit* target, Unit* /*attacker*/, uint32& damage, SpellInfo const* spellInfo) override
    {
        if (!target || !spellInfo || damage == 0 ||
            !SoloRaids::Config::LichKingRestoreRemorselessWinterDamage() ||
            !IsRemorselessWinter(spellInfo->Id) ||
            !target->ToPlayer() ||
            !target->GetMap() ||
            target->GetMap()->GetId() != SOLO_RAIDS_MAP_ICECROWN_CITADEL)
            return;

        damage = uint32(float(damage) * GetRaidSizeDamageMultiplier(target->GetMap()));
    }

    void ModifySpellDamageTaken(Unit* target, Unit* /*attacker*/, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!target || !spellInfo || damage <= 0 ||
            !SoloRaids::Config::LichKingRestoreRemorselessWinterDamage() ||
            !IsRemorselessWinter(spellInfo->Id) ||
            !target->ToPlayer() ||
            !target->GetMap() ||
            target->GetMap()->GetId() != SOLO_RAIDS_MAP_ICECROWN_CITADEL)
            return;

        damage = int32(float(damage) * GetRaidSizeDamageMultiplier(target->GetMap()));
    }
};

void AddLichKingSoloRaidScripts()
{
    new LichKingSoloRaidCreatureScript();
    new LichKingSoloRaidUnitScript();
}
