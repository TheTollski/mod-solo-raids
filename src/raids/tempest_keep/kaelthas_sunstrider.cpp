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
constexpr uint32 NPC_KAELTHAS_SUNSTRIDER = 19622;
constexpr uint32 NPC_MASTER_ENGINEER_TELONICUS = 20063;
constexpr uint32 NPC_DEVASTATION = 21269;
constexpr uint32 NPC_INFINITY_BLADES = 21271;
constexpr uint32 SPELL_REMOTE_TOY = 37027;
constexpr uint32 SPELL_DEVASTATION_WHIRLWIND = 36981;
constexpr uint32 SPELL_INFINITY_BLADES_THRASH = 12787;
constexpr uint32 SOLO_RAIDS_MAP_TEMPEST_KEEP = 550;

std::set<ObjectGuid> kaelthasSoloAnnouncementSent;

void AnnounceKaelthasSoloTweaks(Creature* kaelthas)
{
    if (!kaelthas)
        return;

    ObjectGuid const guid = kaelthas->GetGUID();
    if (kaelthasSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(kaelthas->GetMap(), SOLO_RAIDS_MAP_TEMPEST_KEEP);
    if (!player)
        return;

    bool const disableRemoteToy = SoloRaids::Config::DisableKaelthasRemoteToy();
    bool const disableInfinityBladesThrash = SoloRaids::Config::DisableKaelthasInfinityBladesThrash();
    bool const disableDevastationWhirlwind = SoloRaids::Config::DisableKaelthasDevastationWhirlwind();
    if (!disableRemoteToy && !disableInfinityBladesThrash && !disableDevastationWhirlwind)
        return;

    std::string message = "mod-solo-raids active: Tempest Keep solo tweaks enabled for Kael'thas Sunstrider.";
    if (disableRemoteToy)
        message += " Telonicus's Remote Toy disabled.";
    if (disableInfinityBladesThrash)
        message += " Infinity Blades' Thrash disabled.";
    if (disableDevastationWhirlwind)
        message += " Devastation's Whirlwind disabled.";

    player->SendSystemMessage(message.c_str());
    kaelthasSoloAnnouncementSent.insert(guid);
}
}

class KaelthasSunstriderSoloRaidCreatureScript : public AllCreatureScript
{
public:
    KaelthasSunstriderSoloRaidCreatureScript() : AllCreatureScript("KaelthasSunstriderSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || !SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_TEMPEST_KEEP))
            return;

        if (creature->GetEntry() == NPC_INFINITY_BLADES && SoloRaids::Config::DisableKaelthasInfinityBladesThrash())
            creature->RemoveAurasDueToSpell(SPELL_INFINITY_BLADES_THRASH);
        else if (creature->GetEntry() == NPC_KAELTHAS_SUNSTRIDER && creature->IsInCombat())
            AnnounceKaelthasSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_KAELTHAS_SUNSTRIDER)
            kaelthasSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class KaelthasSunstriderSoloRaidSpellScript : public AllSpellScript
{
public:
    KaelthasSunstriderSoloRaidSpellScript() : AllSpellScript("KaelthasSunstriderSoloRaidSpellScript", { ALLSPELLHOOK_ON_SPELL_CHECK_CAST }) { }

    void OnSpellCheckCast(Spell* spell, bool /*strict*/, SpellCastResult& result) override
    {
        if (!spell || result != SPELL_CAST_OK)
            return;

        Unit* caster = spell->GetCaster();
        if (!caster || !SoloRaids::IsSoloMap(caster->GetMap(), SOLO_RAIDS_MAP_TEMPEST_KEEP))
            return;

        uint32 const spellId = spell->GetSpellInfo()->Id;
        if ((caster->GetEntry() == NPC_MASTER_ENGINEER_TELONICUS && spellId == SPELL_REMOTE_TOY &&
             SoloRaids::Config::DisableKaelthasRemoteToy()) ||
            (caster->GetEntry() == NPC_DEVASTATION && spellId == SPELL_DEVASTATION_WHIRLWIND &&
             SoloRaids::Config::DisableKaelthasDevastationWhirlwind()))
            result = SPELL_FAILED_DONT_REPORT;
    }
};

void AddKaelthasSunstriderSoloRaidScripts()
{
    new KaelthasSunstriderSoloRaidCreatureScript();
    new KaelthasSunstriderSoloRaidSpellScript();
}
