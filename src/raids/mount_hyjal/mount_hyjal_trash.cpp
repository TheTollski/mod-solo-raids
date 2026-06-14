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
#include <string>

namespace
{
constexpr uint32 NPC_GHOUL = 17895;
constexpr uint32 NPC_CRYPT_FIEND = 17897;
constexpr uint32 NPC_ABOMINATION = 17898;
constexpr uint32 NPC_SHADOWY_NECROMANCER = 17899;
constexpr uint32 NPC_SKELETON_INVADER = 17902;
constexpr uint32 NPC_SKELETON_MAGE = 17903;
constexpr uint32 NPC_BANSHEE = 17905;
constexpr uint32 NPC_GARGOYLE = 17906;
constexpr uint32 NPC_FROST_WYRM = 17907;
constexpr uint32 NPC_GIANT_INFERNAL = 17908;
constexpr uint32 NPC_FEL_STALKER = 17916;
constexpr uint32 SPELL_RAISE_DEAD_1 = 31617;
constexpr uint32 SPELL_RAISE_DEAD_2 = 31624;
constexpr uint32 SPELL_RAISE_DEAD_3 = 31625;
constexpr uint32 SPELL_MANA_BURN = 31729;
constexpr uint32 SOLO_RAIDS_MAP_MOUNT_HYJAL = 534;

std::set<uint32> mountHyjalTrashSoloAnnouncementMaps;
std::set<ObjectGuid> removedSoloGargoyles;

bool IsMountHyjalWaveTrash(Unit const* unit)
{
    if (!unit)
        return false;

    switch (unit->GetEntry())
    {
        case NPC_GHOUL:
        case NPC_CRYPT_FIEND:
        case NPC_ABOMINATION:
        case NPC_SHADOWY_NECROMANCER:
        case NPC_SKELETON_INVADER:
        case NPC_SKELETON_MAGE:
        case NPC_BANSHEE:
        case NPC_GARGOYLE:
        case NPC_FROST_WYRM:
        case NPC_GIANT_INFERNAL:
        case NPC_FEL_STALKER:
            return true;
        default:
            return false;
    }
}

bool IsRaiseDead(uint32 spellId)
{
    return spellId == SPELL_RAISE_DEAD_1 || spellId == SPELL_RAISE_DEAD_2 || spellId == SPELL_RAISE_DEAD_3;
}

void AnnounceMountHyjalTrashSoloTweaks(Creature* trash)
{
    if (!trash)
        return;

    Player* player = SoloRaids::GetSoloPlayer(trash->GetMap(), SOLO_RAIDS_MAP_MOUNT_HYJAL);
    if (!player)
        return;

    uint32 const instanceId = trash->GetInstanceId();
    if (mountHyjalTrashSoloAnnouncementMaps.count(instanceId) != 0)
        return;

    std::string message = "mod-solo-raids active: Mount Hyjal solo tweaks enabled for wave trash.";
    bool hasTweaks = false;

    float const damagePct = SoloRaids::Config::MountHyjalTrashDamagePct();
    if (damagePct != 1.0f)
    {
        message += " Damage against all targets set to " + std::to_string(uint32(damagePct * 100.0f)) + "%.";
        hasTweaks = true;
    }
    if (SoloRaids::Config::DisableMountHyjalRaiseDead())
    {
        message += " Raise Dead disabled.";
        hasTweaks = true;
    }
    if (SoloRaids::Config::DisableMountHyjalManaBurn())
    {
        message += " Mana Burn disabled.";
        hasTweaks = true;
    }
    if (SoloRaids::Config::PreventMountHyjalGargoyleSpawns())
    {
        message += " Gargoyle spawns prevented.";
        hasTweaks = true;
    }

    if (!hasTweaks)
        return;

    player->SendSystemMessage(message.c_str());
    mountHyjalTrashSoloAnnouncementMaps.insert(instanceId);
}
}

class MountHyjalTrashSoloRaidCreatureScript : public AllCreatureScript
{
public:
    MountHyjalTrashSoloRaidCreatureScript() : AllCreatureScript("MountHyjalTrashSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !IsMountHyjalWaveTrash(creature) ||
            !SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_MOUNT_HYJAL))
            return;

        if (creature->GetEntry() == NPC_GARGOYLE && creature->IsAlive() &&
            SoloRaids::Config::PreventMountHyjalGargoyleSpawns() &&
            removedSoloGargoyles.insert(creature->GetGUID()).second)
        {
            creature->KillSelf();
            creature->DespawnOrUnsummon(Milliseconds(100));
            return;
        }

        if (creature->IsInCombat())
            AnnounceMountHyjalTrashSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (!creature)
            return;

        removedSoloGargoyles.erase(creature->GetGUID());
    }
};

class MountHyjalTrashSoloRaidSpellScript : public AllSpellScript
{
public:
    MountHyjalTrashSoloRaidSpellScript() : AllSpellScript("MountHyjalTrashSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK)
            return;

        Unit* caster = spell->GetCaster();
        if (!caster || !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_MOUNT_HYJAL))
            return;

        uint32 const spellId = spell->GetSpellInfo()->Id;
        if ((caster->GetEntry() == NPC_SHADOWY_NECROMANCER && IsRaiseDead(spellId) &&
             SoloRaids::Config::DisableMountHyjalRaiseDead()) ||
            (caster->GetEntry() == NPC_FEL_STALKER && spellId == SPELL_MANA_BURN &&
             SoloRaids::Config::DisableMountHyjalManaBurn()))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

class MountHyjalTrashSoloRaidUnitScript : public UnitScript
{
public:
    MountHyjalTrashSoloRaidUnitScript() : UnitScript("MountHyjalTrashSoloRaidUnitScript", true, { UNITHOOK_ON_DAMAGE }) { }

    void OnDamage(Unit* attacker, Unit* /*victim*/, uint32& damage) override
    {
        if (!IsMountHyjalWaveTrash(attacker) ||
            !SoloRaids::IsSoloMap(attacker->GetMap(), SOLO_RAIDS_MAP_MOUNT_HYJAL))
            return;

        damage = uint32(float(damage) * SoloRaids::Config::MountHyjalTrashDamagePct());
    }
};

void AddMountHyjalTrashSoloRaidScripts()
{
    new MountHyjalTrashSoloRaidCreatureScript();
    new MountHyjalTrashSoloRaidSpellScript();
    new MountHyjalTrashSoloRaidUnitScript();
}
