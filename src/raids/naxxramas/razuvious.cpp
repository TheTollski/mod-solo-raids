#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <set>

namespace
{
constexpr uint32 NPC_RAZUVIOUS = 16061;
constexpr uint32 NPC_RAZUVIOUS_25 = 29940;
constexpr uint32 NPC_RAZUVIOUS_40 = 351036;
constexpr uint32 SPELL_UNBALANCING_STRIKE = 26613;
constexpr uint32 SPELL_MANA_BURN = 26046;
constexpr uint32 SOLO_RAIDS_MAP_NAXXRAMAS = 533;

std::set<uint32> razuviousSoloAnnouncementMaps;

bool IsRazuvious(Unit const* unit)
{
    if (!unit)
        return false;

    uint32 const entry = unit->GetEntry();
    return entry == NPC_RAZUVIOUS || entry == NPC_RAZUVIOUS_25 || entry == NPC_RAZUVIOUS_40;
}

void AnnounceRazuviousSoloTweaks(Creature* razuvious)
{
    if (!razuvious)
        return;

    Player* player = SoloRaids::GetSoloPlayer(razuvious->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS);
    if (!player)
        return;

    uint32 const instanceId = razuvious->GetInstanceId();
    if (razuviousSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    player->SendSystemMessage("mod-solo-raids active: Naxxramas solo tweaks enabled for Instructor Razuvious. Mana Burn disabled, Unbalancing Strike damage reduced by 50%.");
    razuviousSoloAnnouncementMaps.insert(instanceId);
}
}

class RazuviousSoloRaidCreatureScript : public AllCreatureScript
{
public:
    RazuviousSoloRaidCreatureScript() : AllCreatureScript("RazuviousSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !IsRazuvious(creature))
            return;

        if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            AnnounceRazuviousSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || !IsRazuvious(creature))
            return;

        razuviousSoloAnnouncementMaps.erase(creature->GetInstanceId());
    }
};

class RazuviousSoloRaidSpellScript : public AllSpellScript
{
public:
    RazuviousSoloRaidSpellScript() : AllSpellScript("RazuviousSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_MANA_BURN)
            return;

        Unit* caster = spell->GetCaster();
        if (IsRazuvious(caster) && SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_NAXXRAMAS))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

class RazuviousSoloRaidUnitScript : public UnitScript
{
public:
    RazuviousSoloRaidUnitScript() : UnitScript("RazuviousSoloRaidUnitScript", true, { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifySpellDamageTaken(Unit* target, Unit* source, int32& amount, SpellInfo const* spellInfo) override
    {
        if (!target || !source || !spellInfo || spellInfo->Id != SPELL_UNBALANCING_STRIKE || !IsRazuvious(source) || !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_NAXXRAMAS))
            return;

        amount /= 2;
    }
};

void AddRazuviousSoloRaidScripts()
{
    new RazuviousSoloRaidCreatureScript();
    new RazuviousSoloRaidSpellScript();
    new RazuviousSoloRaidUnitScript();
}
