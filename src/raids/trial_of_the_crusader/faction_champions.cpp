#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
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
constexpr uint32 SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER = 649;

constexpr uint32 NPC_ALLIANCE_DEATH_KNIGHT = 34461;
constexpr uint32 NPC_ALLIANCE_DRUID_BALANCE = 34460;
constexpr uint32 NPC_ALLIANCE_DRUID_RESTORATION = 34469;
constexpr uint32 NPC_ALLIANCE_HUNTER = 34467;
constexpr uint32 NPC_ALLIANCE_MAGE = 34468;
constexpr uint32 NPC_ALLIANCE_PALADIN_HOLY = 34465;
constexpr uint32 NPC_ALLIANCE_PALADIN_RETRIBUTION = 34471;
constexpr uint32 NPC_ALLIANCE_PRIEST_DISCIPLINE = 34466;
constexpr uint32 NPC_ALLIANCE_PRIEST_SHADOW = 34473;
constexpr uint32 NPC_ALLIANCE_ROGUE = 34472;
constexpr uint32 NPC_ALLIANCE_SHAMAN_ENHANCEMENT = 34463;
constexpr uint32 NPC_ALLIANCE_SHAMAN_RESTORATION = 34470;
constexpr uint32 NPC_ALLIANCE_WARLOCK = 34474;
constexpr uint32 NPC_ALLIANCE_WARRIOR = 34475;
constexpr uint32 NPC_HORDE_DEATH_KNIGHT = 34458;
constexpr uint32 NPC_HORDE_DRUID_BALANCE = 34451;
constexpr uint32 NPC_HORDE_DRUID_RESTORATION = 34459;
constexpr uint32 NPC_HORDE_HUNTER = 34448;
constexpr uint32 NPC_HORDE_MAGE = 34449;
constexpr uint32 NPC_HORDE_PALADIN_HOLY = 34445;
constexpr uint32 NPC_HORDE_PALADIN_RETRIBUTION = 34456;
constexpr uint32 NPC_HORDE_PRIEST_DISCIPLINE = 34447;
constexpr uint32 NPC_HORDE_PRIEST_SHADOW = 34441;
constexpr uint32 NPC_HORDE_ROGUE = 34454;
constexpr uint32 NPC_HORDE_SHAMAN_ENHANCEMENT = 34455;
constexpr uint32 NPC_HORDE_SHAMAN_RESTORATION = 34444;
constexpr uint32 NPC_HORDE_WARLOCK = 34450;
constexpr uint32 NPC_HORDE_WARRIOR = 34453;

constexpr uint32 SPELL_HEX = 66054;
constexpr uint32 SPELL_PSYCHIC_SCREAM = 65543;
constexpr uint32 SPELL_FEAR = 65809;
constexpr uint32 SPELL_CYCLONE = 65859;
constexpr uint32 SPELL_INTIMIDATING_SHOUT = 65930;
constexpr uint32 SPELL_BLIND = 65960;
constexpr uint32 SPELL_REPENTANCE = 66008;
constexpr uint32 SPELL_MANA_BURN = 66100;
constexpr uint32 SPELL_SILENCE = 65542;
constexpr uint32 SPELL_STRANGULATE = 66018;
constexpr uint32 SPELL_SPELL_LOCK = 67519;
constexpr uint32 SPELL_DIVINE_SHIELD = 66010;
constexpr uint32 SPELL_HAND_OF_PROTECTION = 66009;
constexpr uint32 SPELL_DISPERSION = 65544;
constexpr uint32 SPELL_CLOAK_OF_SHADOWS = 65961;
constexpr uint32 SPELL_RETALIATION = 65932;
constexpr uint32 SPELL_ICEBOUND_FORTITUDE = 66023;
constexpr uint32 SPELL_LIFEBLOOM = 66093;
constexpr uint32 SPELL_NOURISH = 66066;
constexpr uint32 SPELL_REGROWTH = 66067;
constexpr uint32 SPELL_REJUVENATION = 66065;
constexpr uint32 SPELL_TRANQUILITY = 66086;
constexpr uint32 SPELL_HEALING_WAVE = 66055;
constexpr uint32 SPELL_RIPTIDE = 66053;
constexpr uint32 SPELL_EARTH_SHIELD = 66063;
constexpr uint32 SPELL_FLASH_OF_LIGHT = 66113;
constexpr uint32 SPELL_HOLY_LIGHT = 66112;
constexpr uint32 SPELL_HOLY_SHOCK = 66114;
constexpr uint32 SPELL_RENEW = 66177;
constexpr uint32 SPELL_POWER_WORD_SHIELD = 66099;
constexpr uint32 SPELL_FLASH_HEAL = 66104;

std::set<uint32> factionChampionsSoloAnnouncementInstances;

bool IsFactionChampion(Unit const* unit)
{
    if (!unit)
        return false;

    switch (unit->GetEntry())
    {
        case NPC_ALLIANCE_DEATH_KNIGHT:
        case NPC_ALLIANCE_DRUID_BALANCE:
        case NPC_ALLIANCE_DRUID_RESTORATION:
        case NPC_ALLIANCE_HUNTER:
        case NPC_ALLIANCE_MAGE:
        case NPC_ALLIANCE_PALADIN_HOLY:
        case NPC_ALLIANCE_PALADIN_RETRIBUTION:
        case NPC_ALLIANCE_PRIEST_DISCIPLINE:
        case NPC_ALLIANCE_PRIEST_SHADOW:
        case NPC_ALLIANCE_ROGUE:
        case NPC_ALLIANCE_SHAMAN_ENHANCEMENT:
        case NPC_ALLIANCE_SHAMAN_RESTORATION:
        case NPC_ALLIANCE_WARLOCK:
        case NPC_ALLIANCE_WARRIOR:
        case NPC_HORDE_DEATH_KNIGHT:
        case NPC_HORDE_DRUID_BALANCE:
        case NPC_HORDE_DRUID_RESTORATION:
        case NPC_HORDE_HUNTER:
        case NPC_HORDE_MAGE:
        case NPC_HORDE_PALADIN_HOLY:
        case NPC_HORDE_PALADIN_RETRIBUTION:
        case NPC_HORDE_PRIEST_DISCIPLINE:
        case NPC_HORDE_PRIEST_SHADOW:
        case NPC_HORDE_ROGUE:
        case NPC_HORDE_SHAMAN_ENHANCEMENT:
        case NPC_HORDE_SHAMAN_RESTORATION:
        case NPC_HORDE_WARLOCK:
        case NPC_HORDE_WARRIOR:
            return true;
        default:
            return false;
    }
}

bool IsHardCC(uint32 spellId)
{
    return spellId == SPELL_HEX ||
        spellId == SPELL_PSYCHIC_SCREAM ||
        spellId == SPELL_FEAR ||
        spellId == SPELL_CYCLONE ||
        spellId == SPELL_INTIMIDATING_SHOUT ||
        spellId == SPELL_BLIND ||
        spellId == SPELL_REPENTANCE;
}

bool IsManaBurnOrInterrupt(uint32 spellId)
{
    return spellId == SPELL_MANA_BURN ||
        spellId == SPELL_SILENCE ||
        spellId == SPELL_STRANGULATE ||
        spellId == SPELL_SPELL_LOCK;
}

bool IsMajorDefensive(uint32 spellId)
{
    return spellId == SPELL_DIVINE_SHIELD ||
        spellId == SPELL_HAND_OF_PROTECTION ||
        spellId == SPELL_DISPERSION ||
        spellId == SPELL_CLOAK_OF_SHADOWS ||
        spellId == SPELL_RETALIATION ||
        spellId == SPELL_ICEBOUND_FORTITUDE;
}

bool IsHealOrAbsorb(uint32 spellId)
{
    return spellId == SPELL_LIFEBLOOM ||
        spellId == SPELL_NOURISH ||
        spellId == SPELL_REGROWTH ||
        spellId == SPELL_REJUVENATION ||
        spellId == SPELL_TRANQUILITY ||
        spellId == SPELL_HEALING_WAVE ||
        spellId == SPELL_RIPTIDE ||
        spellId == SPELL_EARTH_SHIELD ||
        spellId == SPELL_FLASH_OF_LIGHT ||
        spellId == SPELL_HOLY_LIGHT ||
        spellId == SPELL_HOLY_SHOCK ||
        spellId == SPELL_RENEW ||
        spellId == SPELL_POWER_WORD_SHIELD ||
        spellId == SPELL_FLASH_HEAL;
}

bool ShouldBlockSpell(Spell* spell)
{
    if (!spell)
        return false;

    Unit* caster = spell->GetCaster();
    if (!IsFactionChampion(caster) ||
        !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
        return false;

    SpellInfo const* spellInfo = spell->GetSpellInfo();
    uint32 const spellId = spellInfo->Id;
    if (IsHardCC(spellId))
        return SoloRaids::Config::DisableFactionChampionsHardCC();
    if (IsManaBurnOrInterrupt(spellId))
        return SoloRaids::Config::DisableFactionChampionsManaBurnAndInterrupts();
    if (IsMajorDefensive(spellId))
        return SoloRaids::Config::DisableFactionChampionsMajorDefensives();
    if (SoloRaids::Config::DisableFactionChampionsHealing() && (IsHealOrAbsorb(spellId) || spellInfo->IsPositive()))
        return SoloRaids::Config::DisableFactionChampionsHealing();

    return false;
}

void RemoveHealAndAbsorbAuras(Unit* unit)
{
    if (!IsFactionChampion(unit) ||
        !SoloRaids::Config::DisableFactionChampionsHealing() ||
        !SoloRaids::IsSoloMap(unit->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
        return;

    unit->RemoveAurasDueToSpell(SPELL_LIFEBLOOM);
    unit->RemoveAurasDueToSpell(SPELL_REGROWTH);
    unit->RemoveAurasDueToSpell(SPELL_REJUVENATION);
    unit->RemoveAurasDueToSpell(SPELL_RIPTIDE);
    unit->RemoveAurasDueToSpell(SPELL_EARTH_SHIELD);
    unit->RemoveAurasDueToSpell(SPELL_RENEW);
    unit->RemoveAurasDueToSpell(SPELL_POWER_WORD_SHIELD);
}

void AnnounceFactionChampionsSoloTweaks(Creature* champion)
{
    if (!champion)
        return;

    Player* player = SoloRaids::GetSoloPlayer(champion->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER);
    if (!player)
        return;

    uint32 const instanceId = champion->GetInstanceId();
    if (factionChampionsSoloAnnouncementInstances.count(instanceId) != 0)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Trial of the Crusader solo tweaks enabled for Faction Champions. Damage set to " +
        std::to_string(uint32(SoloRaids::Config::FactionChampionsDamagePct() * 100.0f)) +
        "%, healing/support spells disabled, hard CC disabled, Mana Burn and heavy interrupts disabled, major defensives disabled.").c_str());
    factionChampionsSoloAnnouncementInstances.insert(instanceId);
}
}

class FactionChampionsSoloRaidCreatureScript : public AllCreatureScript
{
public:
    FactionChampionsSoloRaidCreatureScript() : AllCreatureScript("FactionChampionsSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && IsFactionChampion(creature) && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
            AnnounceFactionChampionsSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && IsFactionChampion(creature))
            factionChampionsSoloAnnouncementInstances.erase(creature->GetInstanceId());
    }
};

class FactionChampionsSoloRaidSpellScript : public AllSpellScript
{
public:
    FactionChampionsSoloRaidSpellScript() : AllSpellScript("FactionChampionsSoloRaidSpellScript", { ALLSPELLHOOK_CAN_PREPARE, ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

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

class FactionChampionsSoloRaidUnitScript : public UnitScript
{
public:
    FactionChampionsSoloRaidUnitScript() : UnitScript("FactionChampionsSoloRaidUnitScript", true, { UNITHOOK_ON_DAMAGE, UNITHOOK_MODIFY_HEAL_RECEIVED, UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnDamage(Unit* attacker, Unit* victim, uint32& damage) override
    {
        if (IsFactionChampion(attacker) &&
            SoloRaids::IsSoloPlayer(victim, SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
            damage = uint32(float(damage) * SoloRaids::Config::FactionChampionsDamagePct());
    }

    void ModifyHealReceived(Unit* target, Unit* healer, uint32& heal, SpellInfo const* /*spellInfo*/) override
    {
        if (IsFactionChampion(target) && IsFactionChampion(healer) &&
            SoloRaids::IsSoloMap(target->GetMap(), SOLO_RAIDS_MAP_TRIAL_OF_THE_CRUSADER))
            if (SoloRaids::Config::DisableFactionChampionsHealing())
                heal = 0;
    }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura || !IsHealOrAbsorb(aura->GetId()))
            return;

        RemoveHealAndAbsorbAuras(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        RemoveHealAndAbsorbAuras(unit);
    }
};

void AddFactionChampionsSoloRaidScripts()
{
    new FactionChampionsSoloRaidCreatureScript();
    new FactionChampionsSoloRaidSpellScript();
    new FactionChampionsSoloRaidUnitScript();
}
