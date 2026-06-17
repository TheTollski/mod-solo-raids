#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "UnitScript.h"

#include <set>

namespace
{
constexpr uint32 NPC_KALECGOS_DRAGON = 24850;
constexpr uint32 NPC_SATHROVARR = 24892;
constexpr uint32 SPELL_BANISH = 44836;
constexpr uint32 SOLO_RAIDS_MAP_SUNWELL_PLATEAU = 580;
constexpr int32 ACTION_SATH_BANISH = 3;

std::set<ObjectGuid> kalecgosSoloAnnouncementSent;
std::set<ObjectGuid> kalecgosSoloBanishCompleted;

Player* GetSoloSunwellPlayer(Unit const* unit)
{
    return unit ? SoloRaids::GetSoloPlayer(unit->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU) : nullptr;
}

bool ShouldCompleteSoloOnBanish(Unit const* unit)
{
    return SoloRaids::Config::CompleteKalecgosSoloOnBanish() && GetSoloSunwellPlayer(unit);
}

void CompleteSoloKalecgosBanish(Unit* unit)
{
    Creature* kalecgos = unit ? unit->ToCreature() : nullptr;
    if (!kalecgos || !ShouldCompleteSoloOnBanish(kalecgos) ||
        !kalecgos->HasAura(SPELL_BANISH) ||
        kalecgosSoloBanishCompleted.count(kalecgos->GetGUID()) != 0)
        return;

    if (Creature* sathrovarr = kalecgos->FindNearestCreature(NPC_SATHROVARR, 200.0f, true))
        sathrovarr->CastSpell(sathrovarr, SPELL_BANISH, true);

    kalecgosSoloBanishCompleted.insert(kalecgos->GetGUID());
    kalecgos->AI()->DoAction(ACTION_SATH_BANISH);
}

void AnnounceKalecgosSoloTweaks(Creature* kalecgos)
{
    if (!kalecgos || !SoloRaids::Config::CompleteKalecgosSoloOnBanish())
        return;

    ObjectGuid const guid = kalecgos->GetGUID();
    if (kalecgosSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = GetSoloSunwellPlayer(kalecgos);
    if (!player)
        return;

    player->SendSystemMessage("mod-solo-raids active: Sunwell Plateau solo tweaks enabled for Kalecgos. Split-realm completion handled when Kalecgos reaches banish.");
    kalecgosSoloAnnouncementSent.insert(guid);
}
}

class KalecgosSoloRaidCreatureScript : public AllCreatureScript
{
public:
    KalecgosSoloRaidCreatureScript() : AllCreatureScript("KalecgosSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_KALECGOS_DRAGON && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_SUNWELL_PLATEAU))
            AnnounceKalecgosSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_KALECGOS_DRAGON)
        {
            kalecgosSoloAnnouncementSent.erase(creature->GetGUID());
            kalecgosSoloBanishCompleted.erase(creature->GetGUID());
        }
    }
};

class KalecgosSoloRaidUnitScript : public UnitScript
{
public:
    KalecgosSoloRaidUnitScript() : UnitScript("KalecgosSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || unit->GetEntry() != NPC_KALECGOS_DRAGON || !aura || aura->GetId() != SPELL_BANISH)
            return;

        CompleteSoloKalecgosBanish(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        if (unit && unit->GetEntry() == NPC_KALECGOS_DRAGON)
            CompleteSoloKalecgosBanish(unit);
    }
};

void AddKalecgosSoloRaidScripts()
{
    new KalecgosSoloRaidCreatureScript();
    new KalecgosSoloRaidUnitScript();
}
