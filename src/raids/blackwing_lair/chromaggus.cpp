#include "solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "Spell.h"
#include "SpellInfo.h"
#include "SharedDefines.h"

#include <set>

namespace
{
constexpr uint32 NPC_CHROMAGGUS = 14020;
constexpr uint32 SPELL_TIME_LAPSE = 23310;

std::set<ObjectGuid> chromaggusSoloAnnouncementSent;

void AnnounceChromaggusSoloTweaks(Creature* chromaggus)
{
    if (!chromaggus)
        return;

    ObjectGuid const guid = chromaggus->GetGUID();
    if (chromaggusSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::BlackwingLair::GetSoloRaidPlayer(chromaggus->GetMap());
    if (!player)
        return;

    player->SendSystemMessage("mod-solo-raids active: Blackwing Lair solo tweaks enabled for Chromaggus. Time Lapse disabled.");
    chromaggusSoloAnnouncementSent.insert(guid);
}
}

class ChromaggusSoloRaidCreatureScript : public AllCreatureScript
{
public:
    ChromaggusSoloRaidCreatureScript() : AllCreatureScript("ChromaggusSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_CHROMAGGUS)
            return;

        if (creature->IsInCombat() && SoloRaids::BlackwingLair::IsSoloRaidMap(creature->GetMap()))
            AnnounceChromaggusSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_CHROMAGGUS)
            return;

        chromaggusSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class ChromaggusSoloRaidSpellScript : public AllSpellScript
{
public:
    ChromaggusSoloRaidSpellScript() : AllSpellScript("ChromaggusSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_TIME_LAPSE)
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_CHROMAGGUS && SoloRaids::BlackwingLair::IsSoloRaidMap(caster->GetMap()))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddChromaggusSoloRaidScripts()
{
    new ChromaggusSoloRaidCreatureScript();
    new ChromaggusSoloRaidSpellScript();
}
