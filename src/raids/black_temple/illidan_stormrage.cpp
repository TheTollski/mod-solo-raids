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
constexpr uint32 NPC_ILLIDAN_STORMRAGE = 22917;
constexpr uint32 NPC_FLAME_OF_AZZINOTH = 22997;
constexpr uint32 NPC_BLAZE = 23259;
constexpr uint32 SPELL_SUMMON_SHADOW_DEMON = 41117;
constexpr uint32 SPELL_AURA_OF_DREAD = 41142;
constexpr uint32 SOLO_RAIDS_MAP_BLACK_TEMPLE = 564;

std::set<ObjectGuid> illidanSoloAnnouncementSent;

bool IsSoloBlackTemple(Unit const* unit)
{
    return unit && SoloRaids::IsSoloMap(unit->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE);
}

bool IsFlameOfAzzinothDamageSource(Unit const* unit)
{
    return unit && (unit->GetEntry() == NPC_FLAME_OF_AZZINOTH || unit->GetEntry() == NPC_BLAZE);
}

void CapAuraOfDreadStacks(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_BLACK_TEMPLE))
        return;

    Aura* aura = unit->GetAura(SPELL_AURA_OF_DREAD);
    if (!aura)
        return;

    uint8 const maxStacks = SoloRaids::Config::IllidanAuraOfDreadMaxStacks();
    if (aura->GetStackAmount() > maxStacks)
        aura->SetStackAmount(maxStacks);
}

void AnnounceIllidanSoloTweaks(Creature* illidan)
{
    if (!illidan)
        return;

    ObjectGuid const guid = illidan->GetGUID();
    if (illidanSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(illidan->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE);
    if (!player)
        return;

    std::string message = "mod-solo-raids active: Black Temple solo tweaks enabled for Illidan Stormrage.";
    bool hasTweaks = false;

    float const flameDamagePct = SoloRaids::Config::FlameOfAzzinothDamagePct();
    if (flameDamagePct != 1.0f)
    {
        message += " Flame of Azzinoth damage set to " + std::to_string(uint32(flameDamagePct * 100.0f)) + "%.";
        hasTweaks = true;
    }

    message += " Aura of Dread capped at " +
        std::to_string(uint32(SoloRaids::Config::IllidanAuraOfDreadMaxStacks())) + " stacks.";
    hasTweaks = true;

    if (SoloRaids::Config::DisableIllidanSummonShadowDemon())
    {
        message += " Summon Shadow Demon disabled.";
        hasTweaks = true;
    }

    if (!hasTweaks)
        return;

    player->SendSystemMessage(message.c_str());
    illidanSoloAnnouncementSent.insert(guid);
}
}

class IllidanStormrageSoloRaidCreatureScript : public AllCreatureScript
{
public:
    IllidanStormrageSoloRaidCreatureScript() : AllCreatureScript("IllidanStormrageSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_ILLIDAN_STORMRAGE && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE))
            AnnounceIllidanSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_ILLIDAN_STORMRAGE)
            illidanSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class IllidanStormrageSoloRaidSpellScript : public AllSpellScript
{
public:
    IllidanStormrageSoloRaidSpellScript() : AllSpellScript("IllidanStormrageSoloRaidSpellScript", { ALLSPELLHOOK_CAN_PREPARE, ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!spell || spell->GetSpellInfo()->Id != SPELL_SUMMON_SHADOW_DEMON ||
            !SoloRaids::Config::DisableIllidanSummonShadowDemon())
            return true;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_ILLIDAN_STORMRAGE && IsSoloBlackTemple(caster))
            return false;

        return true;
    }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_SUMMON_SHADOW_DEMON ||
            !SoloRaids::Config::DisableIllidanSummonShadowDemon())
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_ILLIDAN_STORMRAGE && IsSoloBlackTemple(caster))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

class IllidanStormrageSoloRaidUnitScript : public UnitScript
{
public:
    IllidanStormrageSoloRaidUnitScript() : UnitScript("IllidanStormrageSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_DAMAGE, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura || aura->GetId() != SPELL_AURA_OF_DREAD)
            return;

        CapAuraOfDreadStacks(unit);
    }

    void OnDamage(Unit* attacker, Unit* /*victim*/, uint32& damage) override
    {
        if (!IsFlameOfAzzinothDamageSource(attacker) || !IsSoloBlackTemple(attacker))
            return;

        damage = uint32(float(damage) * SoloRaids::Config::FlameOfAzzinothDamagePct());
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        CapAuraOfDreadStacks(unit);
    }
};

void AddIllidanStormrageSoloRaidScripts()
{
    new IllidanStormrageSoloRaidCreatureScript();
    new IllidanStormrageSoloRaidSpellScript();
    new IllidanStormrageSoloRaidUnitScript();
}
