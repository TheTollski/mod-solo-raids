#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "UnitScript.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_MALYGOS = 28859;
constexpr uint32 NPC_VORTEX = 30090;
constexpr uint32 SPELL_VORTEX_TICK = 56237;
constexpr uint32 SOLO_RAIDS_MAP_EYE_OF_ETERNITY = 616;
constexpr float MALYGOS_SEARCH_RANGE = 500.0f;

std::set<ObjectGuid> malygosSoloAnnouncementSent;

bool IsVortexDamageSource(Unit const* attacker)
{
    if (!attacker)
        return false;

    if (attacker->GetEntry() == NPC_VORTEX)
        return true;

    return attacker->GetEntry() == NPC_MALYGOS && attacker->HasAura(SPELL_VORTEX_TICK);
}

bool IsMalygosVortexActive(Unit* unit)
{
    if (!unit)
        return false;

    Creature* malygos = unit->FindNearestCreature(NPC_MALYGOS, MALYGOS_SEARCH_RANGE, true);
    return malygos && malygos->HasAura(SPELL_VORTEX_TICK);
}

void AnnounceMalygosSoloTweaks(Creature* malygos)
{
    if (!malygos)
        return;

    ObjectGuid const guid = malygos->GetGUID();
    if (malygosSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(malygos->GetMap(), SOLO_RAIDS_MAP_EYE_OF_ETERNITY);
    if (!player)
        return;

    float const vortexDamagePct = SoloRaids::Config::MalygosVortexDamagePct();
    if (vortexDamagePct == 1.0f)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Eye of Eternity solo tweaks enabled for Malygos. Vortex damage set to " +
        std::to_string(uint32(vortexDamagePct * 100.0f)) + "%.").c_str());
    malygosSoloAnnouncementSent.insert(guid);
}
}

class MalygosSoloRaidCreatureScript : public AllCreatureScript
{
public:
    MalygosSoloRaidCreatureScript() : AllCreatureScript("MalygosSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_MALYGOS && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_EYE_OF_ETERNITY))
            AnnounceMalygosSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_MALYGOS)
            malygosSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class MalygosSoloRaidUnitScript : public UnitScript
{
public:
    MalygosSoloRaidUnitScript() : UnitScript("MalygosSoloRaidUnitScript", true, { UNITHOOK_ON_DAMAGE }) { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (!SoloRaids::IsSoloPlayer(victim, SOLO_RAIDS_MAP_EYE_OF_ETERNITY) ||
            (!IsVortexDamageSource(attacker) && !IsMalygosVortexActive(victim)))
            return;

        damage = uint32(float(damage) * SoloRaids::Config::MalygosVortexDamagePct());
    }
};

void AddMalygosSoloRaidScripts()
{
    new MalygosSoloRaidCreatureScript();
    new MalygosSoloRaidUnitScript();
}
