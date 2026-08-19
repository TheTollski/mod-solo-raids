#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellInfo.h"
#include "UnitScript.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_LORD_MARROWGAR = 36612;
constexpr uint32 SPELL_BONE_SLICE = 69055;
constexpr uint32 SPELL_BONE_SLICE_25 = 70814;
constexpr uint32 SOLO_RAIDS_MAP_ICECROWN_CITADEL = 631;

std::set<ObjectGuid> lordMarrowgarSoloAnnouncementSent;

bool IsBoneSlice(uint32 spellId)
{
    return spellId == SPELL_BONE_SLICE || spellId == SPELL_BONE_SLICE_25;
}

void AnnounceLordMarrowgarSoloTweaks(Creature* marrowgar)
{
    if (!marrowgar)
        return;

    ObjectGuid const guid = marrowgar->GetGUID();
    if (lordMarrowgarSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(marrowgar->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL);
    if (!player)
        return;

    float const damagePct = SoloRaids::Config::LordMarrowgarBoneSliceDamagePct();
    if (damagePct == 1.0f)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Icecrown Citadel solo tweaks enabled for Lord Marrowgar. Bone Slice damage set to " +
        std::to_string(uint32(damagePct * 100.0f)) + "%.").c_str());
    lordMarrowgarSoloAnnouncementSent.insert(guid);
}
}

class LordMarrowgarSoloRaidCreatureScript : public AllCreatureScript
{
public:
    LordMarrowgarSoloRaidCreatureScript() : AllCreatureScript("LordMarrowgarSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_LORD_MARROWGAR && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            AnnounceLordMarrowgarSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_LORD_MARROWGAR)
            lordMarrowgarSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class LordMarrowgarSoloRaidUnitScript : public UnitScript
{
public:
    LordMarrowgarSoloRaidUnitScript() : UnitScript("LordMarrowgarSoloRaidUnitScript", true, { UNITHOOK_MODIFY_SPELL_DAMAGE_TAKEN }) { }

    void ModifySpellDamageTaken(Unit* target, Unit* attacker, int32& damage, SpellInfo const* spellInfo) override
    {
        if (!target || !attacker || !spellInfo ||
            attacker->GetEntry() != NPC_LORD_MARROWGAR ||
            !IsBoneSlice(spellInfo->Id) ||
            !SoloRaids::IsSoloPlayer(target, SOLO_RAIDS_MAP_ICECROWN_CITADEL))
            return;

        damage = int32(float(damage) * SoloRaids::Config::LordMarrowgarBoneSliceDamagePct());
    }
};

void AddLordMarrowgarSoloRaidScripts()
{
    new LordMarrowgarSoloRaidCreatureScript();
    new LordMarrowgarSoloRaidUnitScript();
}
