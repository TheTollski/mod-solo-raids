#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_LORD_JARAXXUS = 34780;
constexpr uint32 SPELL_INCINERATE_FLESH = 66237;
constexpr uint32 SPELL_INCINERATE_FLESH_10H = 67049;
constexpr uint32 SPELL_INCINERATE_FLESH_25N = 67050;
constexpr uint32 SPELL_INCINERATE_FLESH_25H = 67051;
constexpr uint32 SPELL_NETHER_POWER = 66228;
constexpr uint32 SPELL_NETHER_POWER_10H = 67106;
constexpr uint32 SPELL_NETHER_POWER_25N = 67107;
constexpr uint32 SPELL_NETHER_POWER_25H = 67108;
constexpr uint32 SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER = 649;

std::set<ObjectGuid> jaraxxusSoloAnnouncementSent;

bool IsNetherPower(uint32 spellId)
{
    return spellId == SPELL_NETHER_POWER ||
        spellId == SPELL_NETHER_POWER_10H ||
        spellId == SPELL_NETHER_POWER_25N ||
        spellId == SPELL_NETHER_POWER_25H;
}

bool IsIncinerateFlesh(uint32 spellId)
{
    return spellId == SPELL_INCINERATE_FLESH ||
        spellId == SPELL_INCINERATE_FLESH_10H ||
        spellId == SPELL_INCINERATE_FLESH_25N ||
        spellId == SPELL_INCINERATE_FLESH_25H;
}

bool ShouldBlockSpell(Spell* spell)
{
    if (!spell)
        return false;

    Unit* caster = spell->GetCaster();
    if (!caster || caster->GetEntry() != NPC_LORD_JARAXXUS ||
        !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
        return false;

    uint32 const spellId = spell->GetSpellInfo()->Id;
    if (IsIncinerateFlesh(spellId))
        return SoloRaids::Config::DisableLordJaraxxusIncinerateFlesh();

    if (IsNetherPower(spellId))
        return SoloRaids::Config::DisableLordJaraxxusNetherPower();

    return false;
}

void RemoveIncinerateFlesh(Unit* unit)
{
    if (!SoloRaids::Config::DisableLordJaraxxusIncinerateFlesh() ||
        !SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
        return;

    unit->RemoveAurasDueToSpell(SPELL_INCINERATE_FLESH);
    unit->RemoveAurasDueToSpell(SPELL_INCINERATE_FLESH_10H);
    unit->RemoveAurasDueToSpell(SPELL_INCINERATE_FLESH_25N);
    unit->RemoveAurasDueToSpell(SPELL_INCINERATE_FLESH_25H);
}

void AnnounceJaraxxusSoloTweaks(Creature* jaraxxus)
{
    if (!jaraxxus)
        return;

    ObjectGuid const guid = jaraxxus->GetGUID();
    if (jaraxxusSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(jaraxxus->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER);
    if (!player)
        return;

    std::string message = "mod-solo-raids active: Trial of the Crusader solo tweaks enabled for Lord Jaraxxus.";
    if (SoloRaids::Config::DisableLordJaraxxusIncinerateFlesh())
        message += " Incinerate Flesh disabled.";
    if (SoloRaids::Config::DisableLordJaraxxusNetherPower())
        message += " Nether Power disabled.";

    player->SendSystemMessage(message.c_str());
    jaraxxusSoloAnnouncementSent.insert(guid);
}
}

class LordJaraxxusSoloRaidCreatureScript : public AllCreatureScript
{
public:
    LordJaraxxusSoloRaidCreatureScript() : AllCreatureScript("LordJaraxxusSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_LORD_JARAXXUS && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
            AnnounceJaraxxusSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_LORD_JARAXXUS)
            jaraxxusSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class LordJaraxxusSoloRaidSpellScript : public AllSpellScript
{
public:
    LordJaraxxusSoloRaidSpellScript() : AllSpellScript("LordJaraxxusSoloRaidSpellScript", { ALLSPELLHOOK_CAN_PREPARE, ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        return !ShouldBlockSpell(spell);
    }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (result == SPELL_CAST_OK && ShouldBlockSpell(spell))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

class LordJaraxxusSoloRaidUnitScript : public UnitScript
{
public:
    LordJaraxxusSoloRaidUnitScript() : UnitScript("LordJaraxxusSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura || !IsIncinerateFlesh(aura->GetId()))
            return;

        RemoveIncinerateFlesh(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        RemoveIncinerateFlesh(unit);
    }
};

void AddLordJaraxxusSoloRaidScripts()
{
    new LordJaraxxusSoloRaidCreatureScript();
    new LordJaraxxusSoloRaidSpellScript();
    new LordJaraxxusSoloRaidUnitScript();
}
