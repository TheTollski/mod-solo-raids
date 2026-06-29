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

#include <set>

namespace
{
constexpr uint32 NPC_KOLOGARN = 32930;
constexpr uint32 SPELL_STONE_GRIP = 62166;
constexpr uint32 SPELL_STONE_GRIP_25 = 63981;
constexpr uint32 SPELL_RIDE_RIGHT_ARM = 62056;
constexpr uint32 SPELL_RIDE_RIGHT_ARM_25 = 63985;
constexpr uint32 SOLO_RAIDS_MAP_ULDUAR = 603;

std::set<ObjectGuid> kologarnSoloAnnouncementSent;

bool IsStoneGripSpell(uint32 spellId)
{
    return spellId == SPELL_STONE_GRIP || spellId == SPELL_STONE_GRIP_25;
}

void AnnounceKologarnSoloTweaks(Creature* kologarn)
{
    if (!kologarn)
        return;

    ObjectGuid const guid = kologarn->GetGUID();
    if (kologarnSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(kologarn->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    if (!player || !SoloRaids::Config::DisableKologarnStoneGrip())
        return;

    player->SendSystemMessage("mod-solo-raids active: Ulduar solo tweaks enabled for Kologarn. Stone Grip disabled.");
    kologarnSoloAnnouncementSent.insert(guid);
}

void RemoveStoneGripAuras(Unit* unit)
{
    if (!SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_ULDUAR) ||
        !SoloRaids::Config::DisableKologarnStoneGrip())
        return;

    unit->RemoveAurasDueToSpell(SPELL_RIDE_RIGHT_ARM);
    unit->RemoveAurasDueToSpell(SPELL_RIDE_RIGHT_ARM_25);
}
}

class KologarnSoloRaidCreatureScript : public AllCreatureScript
{
public:
    KologarnSoloRaidCreatureScript() : AllCreatureScript("KologarnSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_KOLOGARN && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            AnnounceKologarnSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_KOLOGARN)
            kologarnSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class KologarnSoloRaidSpellScript : public AllSpellScript
{
public:
    KologarnSoloRaidSpellScript() : AllSpellScript("KologarnSoloRaidSpellScript", { ALLSPELLHOOK_CAN_PREPARE, ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!spell || !IsStoneGripSpell(spell->GetSpellInfo()->Id) ||
            !SoloRaids::Config::DisableKologarnStoneGrip())
            return true;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_KOLOGARN &&
            SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            return false;

        return true;
    }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || !IsStoneGripSpell(spell->GetSpellInfo()->Id) ||
            !SoloRaids::Config::DisableKologarnStoneGrip())
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_KOLOGARN &&
            SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

class KologarnSoloRaidUnitScript : public UnitScript
{
public:
    KologarnSoloRaidUnitScript() : UnitScript("KologarnSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY, UNITHOOK_ON_UNIT_UPDATE }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!aura || (aura->GetId() != SPELL_RIDE_RIGHT_ARM && aura->GetId() != SPELL_RIDE_RIGHT_ARM_25))
            return;

        RemoveStoneGripAuras(unit);
    }

    void OnUnitUpdate(Unit* unit, uint32 /*diff*/) override
    {
        RemoveStoneGripAuras(unit);
    }
};

void AddKologarnSoloRaidScripts()
{
    new KologarnSoloRaidCreatureScript();
    new KologarnSoloRaidSpellScript();
    new KologarnSoloRaidUnitScript();
}
