#include "../../solo_raid_utils.h"
#include "../../solo_raid_config.h"

#include "Creature.h"
#include "GlobalScript.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SpellMgr.h"
#include "SharedDefines.h"
#include "Unit.h"

#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_RAZORGORE = 12435;
constexpr uint32 SPELL_DESTROY_EGG = 19873;
constexpr uint32 SOLO_RAIDS_MAP_BLACKWING_LAIR = 469;

std::map<ObjectGuid, float> razorgoreHasteApplied;
std::set<ObjectGuid> razorgoreSoloAnnouncementSent;

bool IsSoloControlledRazorgore(Unit const* unit)
{
    if (!unit || unit->GetEntry() != NPC_RAZORGORE || !unit->IsCharmed())
        return false;

    return SoloRaids::IsSoloMap(unit->GetMap(), SOLO_RAIDS_MAP_BLACKWING_LAIR);
}

float GetDestroyEggCastHasteBonusPct()
{
    float const castTimePct = SoloRaids::Config::RazorgoreDestroyEggCastSpeedPct();
    if (castTimePct >= 1.0f)
        return 0.0f;

    return ((1.0f / castTimePct) - 1.0f) * 100.0f;
}

void AnnounceRazorgoreSoloTweaks(Creature* razorgore)
{
    if (!razorgore)
        return;

    ObjectGuid const guid = razorgore->GetGUID();
    if (razorgoreSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = razorgore->GetCharmerOrOwnerPlayerOrPlayerItself();
    if (!player)
        player = SoloRaids::GetSoloPlayer(razorgore->GetMap(), SOLO_RAIDS_MAP_BLACKWING_LAIR);

    if (!player)
        return;

    std::string message = "mod-solo-raids active: Blackwing Lair solo tweaks enabled for Razorgore.";
    bool hasTweaks = false;

    if (SoloRaids::Config::ClearRazorgoreDestroyEggCooldown())
    {
        message += " Destroy Egg cooldown removed.";
        hasTweaks = true;
    }

    float const castTimePct = SoloRaids::Config::RazorgoreDestroyEggCastSpeedPct();
    if (castTimePct != 1.0f)
    {
        message += " Destroy Egg cast time set to " + std::to_string(uint32(castTimePct * 100.0f)) + "% of normal.";
        hasTweaks = true;
    }

    if (!hasTweaks)
        return;

    player->SendSystemMessage(message.c_str());
    razorgoreSoloAnnouncementSent.insert(guid);
}

void SetRazorgoreSoloHaste(Creature* creature, bool apply)
{
    if (!creature)
        return;

    ObjectGuid const guid = creature->GetGUID();
    auto itr = razorgoreHasteApplied.find(guid);
    bool const alreadyApplied = itr != razorgoreHasteApplied.end();
    float const castSpeedBonus = GetDestroyEggCastHasteBonusPct();

    if (!apply || castSpeedBonus <= 0.0f)
    {
        if (alreadyApplied)
        {
            creature->ApplyCastTimePercentMod(itr->second, false);
            razorgoreHasteApplied.erase(itr);
        }
        return;
    }

    if (alreadyApplied && itr->second == castSpeedBonus)
        return;

    if (alreadyApplied)
        creature->ApplyCastTimePercentMod(itr->second, false);

    creature->ApplyCastTimePercentMod(castSpeedBonus, true);
    razorgoreHasteApplied[guid] = castSpeedBonus;
}

void ClearDestroyEggCooldown(Creature* razorgore)
{
    if (!razorgore || !SoloRaids::Config::ClearRazorgoreDestroyEggCooldown())
        return;

    SpellInfo const* spellInfo = sSpellMgr->GetSpellInfo(SPELL_DESTROY_EGG);
    uint32 const category = spellInfo ? spellInfo->GetCategory() : 0;

    for (CreatureSpellCooldowns::iterator itr = razorgore->m_CreatureSpellCooldowns.begin(); itr != razorgore->m_CreatureSpellCooldowns.end();)
    {
        if (itr->first == SPELL_DESTROY_EGG || (category && itr->second.category == category))
            itr = razorgore->m_CreatureSpellCooldowns.erase(itr);
        else
            ++itr;
    }

    if (Player* controller = razorgore->GetCharmerOrOwnerPlayerOrPlayerItself())
        controller->SendClearCooldown(SPELL_DESTROY_EGG, razorgore);
}
}

class RazorgoreTheUntamedSoloRaidGlobalScript : public GlobalScript
{
public:
    RazorgoreTheUntamedSoloRaidGlobalScript() : GlobalScript("RazorgoreTheUntamedSoloRaidGlobalScript", { GLOBALHOOK_ON_LOAD_SPELL_CUSTOM_ATTR }) { }

    void OnLoadSpellCustomAttr(SpellInfo* spellInfo) override
    {
        if (!spellInfo || spellInfo->Id != SPELL_DESTROY_EGG)
            return;

        // Destroy Egg is SPELL_DAMAGE_CLASS_NONE, so this flag lets Razorgore's solo haste affect its real server cast timer.
        spellInfo->AttributesEx5 |= SPELL_ATTR5_SPELL_HASTE_AFFECTS_PERIODIC;
    }
};

class RazorgoreTheUntamedSoloRaidCreatureScript : public AllCreatureScript
{
public:
    RazorgoreTheUntamedSoloRaidCreatureScript() : AllCreatureScript("RazorgoreTheUntamedSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_RAZORGORE)
            return;

        bool const applySoloChanges = IsSoloControlledRazorgore(creature);
        if (applySoloChanges)
            AnnounceRazorgoreSoloTweaks(creature);

        SetRazorgoreSoloHaste(creature, applySoloChanges);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_RAZORGORE)
            return;

        SetRazorgoreSoloHaste(creature, false);
        razorgoreSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class RazorgoreTheUntamedSoloRaidSpellScript : public AllSpellScript
{
public:
    RazorgoreTheUntamedSoloRaidSpellScript() : AllSpellScript("RazorgoreTheUntamedSoloRaidSpellScript", { ALLSPELLHOOK_CAN_PREPARE, ALLSPELLHOOK_ON_PREPARE, ALLSPELLHOOK_ON_CAST }) { }

    bool CanPrepare(Spell* spell, SpellCastTargets const* /*targets*/, AuraEffect const* /*triggeredByAura*/) override
    {
        if (!spell || spell->GetSpellInfo()->Id != SPELL_DESTROY_EGG || !IsSoloControlledRazorgore(spell->GetCaster()))
            return true;

        Creature* razorgore = spell->GetCaster()->ToCreature();
        if (!razorgore)
            return true;

        SetRazorgoreSoloHaste(razorgore, true);

        return true;
    }

    void OnSpellPrepare(Spell* spell, Unit* caster, SpellInfo const* spellInfo) override
    {
        if (!spell || !spellInfo || spellInfo->Id != SPELL_DESTROY_EGG || !IsSoloControlledRazorgore(caster))
            return;

        Creature* razorgore = caster->ToCreature();
        if (!razorgore)
            return;

        ClearDestroyEggCooldown(razorgore);
    }

    void OnSpellCast(Spell* /*spell*/, Unit* caster, SpellInfo const* spellInfo, bool /*skipCheck*/) override
    {
        if (!spellInfo || spellInfo->Id != SPELL_DESTROY_EGG || !IsSoloControlledRazorgore(caster))
            return;

        ClearDestroyEggCooldown(caster->ToCreature());
    }
};

void AddRazorgoreTheUntamedSoloRaidScripts()
{
    new RazorgoreTheUntamedSoloRaidGlobalScript();
    new RazorgoreTheUntamedSoloRaidCreatureScript();
    new RazorgoreTheUntamedSoloRaidSpellScript();
}
