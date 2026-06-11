#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellInfo.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_FATHOM_LORD_KARATHRESS = 21214;
constexpr uint32 NPC_FATHOM_GUARD_SHARKKIS = 21966;
constexpr uint32 NPC_FATHOM_GUARD_CARIBDIS = 21964;
constexpr uint32 SPELL_LEECHING_THROW = 29436;
constexpr uint32 SPELL_SUMMON_CYCLONE = 38337;
constexpr uint32 SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN = 548;

std::set<ObjectGuid> karathressSoloAnnouncementSent;

void AnnounceKarathressSoloTweaks(Creature* karathress)
{
    if (!karathress)
        return;

    ObjectGuid const guid = karathress->GetGUID();
    if (karathressSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(karathress->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN);
    if (!player)
        return;

    bool const disableLeechingThrow = SoloRaids::Config::DisableKarathressLeechingThrow();
    bool const disableSummonCyclone = SoloRaids::Config::DisableKarathressSummonCyclone();
    if (!disableLeechingThrow && !disableSummonCyclone)
        return;

    std::string message = "mod-solo-raids active: Serpentshrine Cavern solo tweaks enabled for Fathom-Lord Karathress.";
    if (disableLeechingThrow)
        message += " Sharkkis's Leeching Throw disabled.";
    if (disableSummonCyclone)
        message += " Caribdis's Summon Cyclone disabled.";

    player->SendSystemMessage(message.c_str());
    karathressSoloAnnouncementSent.insert(guid);
}
}

class FathomLordKarathressSoloRaidCreatureScript : public AllCreatureScript
{
public:
    FathomLordKarathressSoloRaidCreatureScript() : AllCreatureScript("FathomLordKarathressSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_FATHOM_LORD_KARATHRESS || !creature->IsInCombat() ||
            !SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
            return;

        AnnounceKarathressSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_FATHOM_LORD_KARATHRESS)
            karathressSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class FathomLordKarathressSoloRaidSpellScript : public AllSpellScript
{
public:
    FathomLordKarathressSoloRaidSpellScript() : AllSpellScript("FathomLordKarathressSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK)
            return;

        Unit* caster = spell->GetCaster();
        if (!caster || !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_SERPENTSHRINE_CAVERN))
            return;

        uint32 const spellId = spell->GetSpellInfo()->Id;
        if ((caster->GetEntry() == NPC_FATHOM_GUARD_SHARKKIS && spellId == SPELL_LEECHING_THROW &&
             SoloRaids::Config::DisableKarathressLeechingThrow()) ||
            (caster->GetEntry() == NPC_FATHOM_GUARD_CARIBDIS && spellId == SPELL_SUMMON_CYCLONE &&
             SoloRaids::Config::DisableKarathressSummonCyclone()))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddFathomLordKarathressSoloRaidScripts()
{
    new FathomLordKarathressSoloRaidCreatureScript();
    new FathomLordKarathressSoloRaidSpellScript();
}
