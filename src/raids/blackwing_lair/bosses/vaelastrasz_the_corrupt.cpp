#include "../solo_raid_utils.h"

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
constexpr uint32 NPC_VAELASTRAZ = 13020;
constexpr uint32 SPELL_BURNING_ADRENALINE = 18173;

std::set<ObjectGuid> vaelastraszSoloAnnouncementSent;

void AnnounceVaelastraszSoloTweaks(Creature* vaelastrasz)
{
    if (!vaelastrasz)
        return;

    ObjectGuid const guid = vaelastrasz->GetGUID();
    if (vaelastraszSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::BlackwingLair::GetSoloRaidPlayer(vaelastrasz->GetMap());
    if (!player)
        return;

    player->SendSystemMessage("mod-solo-raids active: Blackwing Lair solo tweaks enabled for Vaelastrasz. Burning Adrenaline disabled.");
    vaelastraszSoloAnnouncementSent.insert(guid);
}
}

class VaelastraszTheCorruptSoloRaidCreatureScript : public AllCreatureScript
{
public:
    VaelastraszTheCorruptSoloRaidCreatureScript() : AllCreatureScript("VaelastraszTheCorruptSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_VAELASTRAZ)
            return;

        if (creature->IsInCombat() && SoloRaids::BlackwingLair::IsSoloRaidMap(creature->GetMap()))
            AnnounceVaelastraszSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_VAELASTRAZ)
            return;

        vaelastraszSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class VaelastraszTheCorruptSoloRaidSpellScript : public AllSpellScript
{
public:
    VaelastraszTheCorruptSoloRaidSpellScript() : AllSpellScript("VaelastraszTheCorruptSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_BURNING_ADRENALINE)
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_VAELASTRAZ && SoloRaids::BlackwingLair::IsSoloRaidMap(caster->GetMap()))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddVaelastraszTheCorruptSoloRaidScripts()
{
    new VaelastraszTheCorruptSoloRaidCreatureScript();
    new VaelastraszTheCorruptSoloRaidSpellScript();
}
