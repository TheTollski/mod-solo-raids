#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "UnitScript.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_STEELBREAKER = 32867;
constexpr uint32 NPC_MOLGEIM = 32927;
constexpr uint32 NPC_BRUNDIR = 32857;
constexpr uint32 SPELL_SHIELD_OF_RUNES = 62274;
constexpr uint32 SPELL_SHIELD_OF_RUNES_25 = 63489;
constexpr uint32 SPELL_SHIELD_OF_RUNES_BUFF = 62277;
constexpr uint32 SPELL_SHIELD_OF_RUNES_BUFF_25 = 63967;
constexpr uint32 SOLO_RAIDS_MAP_ULDUAR = 603;

std::set<uint32> assemblySoloAnnouncementMaps;

bool IsAssemblyBoss(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_STEELBREAKER || entry == NPC_MOLGEIM || entry == NPC_BRUNDIR;
}

bool IsShieldOfRunesAura(uint32 spellId)
{
    return spellId == SPELL_SHIELD_OF_RUNES || spellId == SPELL_SHIELD_OF_RUNES_25 ||
        spellId == SPELL_SHIELD_OF_RUNES_BUFF || spellId == SPELL_SHIELD_OF_RUNES_BUFF_25;
}

void ScaleAuraDuration(Aura* aura)
{
    if (!aura || !IsShieldOfRunesAura(aura->GetId()))
        return;

    Unit* owner = aura->GetOwner()->ToUnit();
    if (!owner || !SoloRaids::IsSoloMap(owner->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
        return;

    float const durationPct = SoloRaids::Config::AssemblyOfIronShieldOfRunesDurationPct();
    if (durationPct == 1.0f || aura->GetMaxDuration() <= 0)
        return;

    int32 const duration = int32(float(aura->GetMaxDuration()) * durationPct);
    aura->SetMaxDuration(duration);
    aura->SetDuration(duration);
}

void AnnounceAssemblySoloTweaks(Creature* boss)
{
    if (!boss)
        return;

    Player* player = SoloRaids::GetSoloPlayer(boss->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    if (!player)
        return;

    uint32 const instanceId = boss->GetInstanceId();
    if (assemblySoloAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Ulduar solo tweaks enabled for Assembly of Iron. Shield of Runes durations set to " +
        std::to_string(uint32(SoloRaids::Config::AssemblyOfIronShieldOfRunesDurationPct() * 100.0f)) + "%.").c_str());
    assemblySoloAnnouncementMaps.insert(instanceId);
}
}

class AssemblyOfIronSoloRaidCreatureScript : public AllCreatureScript
{
public:
    AssemblyOfIronSoloRaidCreatureScript() : AllCreatureScript("AssemblyOfIronSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && IsAssemblyBoss(creature) && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            AnnounceAssemblySoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && IsAssemblyBoss(creature))
            assemblySoloAnnouncementMaps.erase(creature->GetInstanceId());
    }
};

class AssemblyOfIronSoloRaidUnitScript : public UnitScript
{
public:
    AssemblyOfIronSoloRaidUnitScript() : UnitScript("AssemblyOfIronSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY }) { }

    void OnAuraApply(Unit* /*unit*/, Aura* aura) override
    {
        ScaleAuraDuration(aura);
    }
};

void AddAssemblyOfIronSoloRaidScripts()
{
    new AssemblyOfIronSoloRaidCreatureScript();
    new AssemblyOfIronSoloRaidUnitScript();
}
