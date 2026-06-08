#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "CreatureAI.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SharedDefines.h"
#include "Spell.h"
#include "SpellAuras.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <set>

namespace
{
constexpr uint32 NPC_MAGTHERIDON = 17257;
constexpr uint32 NPC_HELLFIRE_CHANNELER = 17256;
constexpr uint32 SPELL_SHADOW_GRASP = 30410;
constexpr uint32 SPELL_BURNING_ABYSSAL = 30511;
constexpr uint32 SOLO_RAIDS_MAP_MAGTHERIDONS_LAIR = 544;
constexpr int32 ACTION_BANISH_SELF = 2;

std::set<ObjectGuid> magtheridonSoloAnnouncementSent;

void AnnounceMagtheridonSoloTweaks(Creature* magtheridon)
{
    if (!magtheridon)
        return;

    ObjectGuid const guid = magtheridon->GetGUID();
    if (magtheridonSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(magtheridon->GetMap(), SOLO_RAIDS_MAP_MAGTHERIDONS_LAIR);
    if (!player)
        return;

    bool const oneCubeBanish = SoloRaids::Config::MagtheridonOneCubeBanish();
    bool const disableBurningAbyssal = SoloRaids::Config::DisableMagtheridonBurningAbyssal();
    if (!oneCubeBanish && !disableBurningAbyssal)
        return;

    if (oneCubeBanish && disableBurningAbyssal)
        player->SendSystemMessage("mod-solo-raids active: Magtheridon's Lair solo tweaks enabled for Magtheridon. One Manticron Cube is sufficient to banish Magtheridon, and Hellfire Channelers cannot summon Burning Abyssals.");
    else if (oneCubeBanish)
        player->SendSystemMessage("mod-solo-raids active: Magtheridon's Lair solo tweaks enabled for Magtheridon. One Manticron Cube is sufficient to banish Magtheridon.");
    else
        player->SendSystemMessage("mod-solo-raids active: Magtheridon's Lair solo tweaks enabled for Magtheridon. Hellfire Channelers cannot summon Burning Abyssals.");

    magtheridonSoloAnnouncementSent.insert(guid);
}
}

class MagtheridonSoloRaidCreatureScript : public AllCreatureScript
{
public:
    MagtheridonSoloRaidCreatureScript() : AllCreatureScript("MagtheridonSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_MAGTHERIDON || !creature->IsInCombat() ||
            !SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_MAGTHERIDONS_LAIR))
            return;

        AnnounceMagtheridonSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature || creature->GetEntry() != NPC_MAGTHERIDON)
            return;

        magtheridonSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class MagtheridonSoloRaidUnitScript : public UnitScript
{
public:
    MagtheridonSoloRaidUnitScript() : UnitScript("MagtheridonSoloRaidUnitScript", true, { UNITHOOK_ON_AURA_APPLY }) { }

    void OnAuraApply(Unit* unit, Aura* aura) override
    {
        if (!unit || !aura || aura->GetId() != SPELL_SHADOW_GRASP ||
            !SoloRaids::Config::MagtheridonOneCubeBanish() ||
            !SoloRaids::IsSoloPlayer(unit, SOLO_RAIDS_MAP_MAGTHERIDONS_LAIR))
            return;

        Creature* magtheridon = unit->FindNearestCreature(NPC_MAGTHERIDON, 200.0f, true);
        if (magtheridon && magtheridon->IsInCombat())
            magtheridon->AI()->DoAction(ACTION_BANISH_SELF);
    }
};

class MagtheridonSoloRaidSpellScript : public AllSpellScript
{
public:
    MagtheridonSoloRaidSpellScript() : AllSpellScript("MagtheridonSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK || spell->GetSpellInfo()->Id != SPELL_BURNING_ABYSSAL ||
            !SoloRaids::Config::DisableMagtheridonBurningAbyssal())
            return;

        Unit* caster = spell->GetCaster();
        if (caster && caster->GetEntry() == NPC_HELLFIRE_CHANNELER &&
            SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_MAGTHERIDONS_LAIR))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddMagtheridonSoloRaidScripts()
{
    new MagtheridonSoloRaidCreatureScript();
    new MagtheridonSoloRaidUnitScript();
    new MagtheridonSoloRaidSpellScript();
}
