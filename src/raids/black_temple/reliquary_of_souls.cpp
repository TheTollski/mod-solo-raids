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

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_ESSENCE_OF_DESIRE = 23419;
constexpr uint32 SPELL_RUNE_SHIELD = 41431;
constexpr uint32 SPELL_SPIRIT_SHOCK = 41426;
constexpr uint32 SOLO_RAIDS_MAP_BLACK_TEMPLE = 564;

std::set<ObjectGuid> essenceOfDesireSoloAnnouncementSent;

void AnnounceEssenceOfDesireSoloTweaks(Creature* essence)
{
    if (!essence)
        return;

    ObjectGuid const guid = essence->GetGUID();
    if (essenceOfDesireSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(essence->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE);
    if (!player)
        return;

    std::string message = "mod-solo-raids active: Black Temple solo tweaks enabled for Reliquary of Souls.";
    bool hasTweaks = false;

    if (SoloRaids::Config::DisableEssenceOfDesireSpiritShock())
    {
        message += " Essence of Desire's Spirit Shock disabled.";
        hasTweaks = true;
    }

    float const runeShieldDurationPct = SoloRaids::Config::EssenceOfDesireRuneShieldDurationPct();
    if (runeShieldDurationPct != 1.0f)
    {
        message += " Essence of Desire's Rune Shield duration set to " +
            std::to_string(uint32(runeShieldDurationPct * 100.0f)) + "%.";
        hasTweaks = true;
    }

    if (!hasTweaks)
        return;

    player->SendSystemMessage(message.c_str());
    essenceOfDesireSoloAnnouncementSent.insert(guid);
}
}

class ReliquaryOfSoulsSoloRaidCreatureScript : public AllCreatureScript
{
public:
    ReliquaryOfSoulsSoloRaidCreatureScript() : AllCreatureScript("ReliquaryOfSoulsSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_ESSENCE_OF_DESIRE && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE))
            AnnounceEssenceOfDesireSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_ESSENCE_OF_DESIRE)
            essenceOfDesireSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class ReliquaryOfSoulsSoloRaidSpellScript : public AllSpellScript
{
public:
    ReliquaryOfSoulsSoloRaidSpellScript() : AllSpellScript("ReliquaryOfSoulsSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST, ALLSPELLHOOK_ON_CALC_MAX_DURATION }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_SPIRIT_SHOCK ||
            !SoloRaids::Config::DisableEssenceOfDesireSpiritShock())
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_ESSENCE_OF_DESIRE &&
            SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE))
            result = SPELL_FAILED_DONT_REPORT;
    }

    void OnCalcMaxDuration(Aura const* aura, int32& maxDuration) override
    {
        if (!aura || aura->GetType() != UNIT_AURA_TYPE || aura->GetId() != SPELL_RUNE_SHIELD || maxDuration <= 0)
            return;

        Unit const* owner = aura->GetUnitOwner();
        Unit const* caster = aura->GetCaster();
        if (!owner || !caster || owner->GetEntry() != NPC_ESSENCE_OF_DESIRE ||
            caster->GetEntry() != NPC_ESSENCE_OF_DESIRE ||
            !SoloRaids::IsSoloMap(owner->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE))
            return;

        maxDuration = int32(float(maxDuration) * SoloRaids::Config::EssenceOfDesireRuneShieldDurationPct());
    }
};

void AddReliquaryOfSoulsSoloRaidScripts()
{
    new ReliquaryOfSoulsSoloRaidCreatureScript();
    new ReliquaryOfSoulsSoloRaidSpellScript();
}
