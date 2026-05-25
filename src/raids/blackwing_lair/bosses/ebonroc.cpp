#include "../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"
#include "SpellInfo.h"

#include <set>

namespace
{
constexpr uint32 NPC_EBONROC = 14601;
constexpr uint32 SPELL_SHADOW_OF_EBONROC = 23340;
constexpr int32 SHADOW_OF_EBONROC_SOLO_DURATION = 2000;

std::set<ObjectGuid> ebonrocSoloAnnouncementSent;

void AnnounceEbonrocSoloTweaks(Creature* ebonroc)
{
    if (!ebonroc)
        return;

    ObjectGuid const guid = ebonroc->GetGUID();
    if (ebonrocSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::BlackwingLair::GetSoloRaidPlayer(ebonroc->GetMap());
    if (!player)
        return;

    player->SendSystemMessage("mod-solo-raids active: Blackwing Lair solo tweaks enabled for Ebonroc. Shadow of Ebonroc duration reduced to 2 seconds.");
    ebonrocSoloAnnouncementSent.insert(guid);
}
}

class EbonrocSoloRaidCreatureScript : public AllCreatureScript
{
public:
    EbonrocSoloRaidCreatureScript() : AllCreatureScript("EbonrocSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_EBONROC)
            return;

        if (creature->IsInCombat() && SoloRaids::BlackwingLair::IsSoloRaidMap(creature->GetMap()))
            AnnounceEbonrocSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_EBONROC)
            return;

        ebonrocSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class EbonrocSoloRaidSpellScript : public AllSpellScript
{
public:
    EbonrocSoloRaidSpellScript() : AllSpellScript("EbonrocSoloRaidSpellScript", { ALLSPELLHOOK_ON_CALC_MAX_DURATION }) { }

    void OnCalcMaxDuration(Aura const* aura, int32& maxDuration) override
    {
        if (!aura || aura->GetId() != SPELL_SHADOW_OF_EBONROC)
            return;

        Unit const* owner = aura->GetUnitOwner();
        if (!SoloRaids::BlackwingLair::IsSoloRaidPlayer(owner))
            return;

        maxDuration = SHADOW_OF_EBONROC_SOLO_DURATION;
    }
};

void AddEbonrocSoloRaidScripts()
{
    new EbonrocSoloRaidCreatureScript();
    new EbonrocSoloRaidSpellScript();
}
