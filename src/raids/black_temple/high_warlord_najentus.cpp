#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"
#include "SpellAuras.h"

#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_HIGH_WARLORD_NAJENTUS = 22887;
constexpr uint32 SPELL_TIDAL_SHIELD = 39872;
constexpr uint32 SOLO_RAIDS_MAP_BLACK_TEMPLE = 564;

std::set<ObjectGuid> najentusSoloAnnouncementSent;

void AnnounceNajentusSoloTweaks(Creature* najentus)
{
    if (!najentus)
        return;

    ObjectGuid const guid = najentus->GetGUID();
    if (najentusSoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(najentus->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE);
    if (!player)
        return;

    float const durationPct = SoloRaids::Config::NajentusTidalShieldDurationPct();
    if (durationPct == 1.0f)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Black Temple solo tweaks enabled for High Warlord Naj'entus. Tidal Shield duration set to " +
        std::to_string(uint32(durationPct * 100.0f)) + "%.").c_str());
    najentusSoloAnnouncementSent.insert(guid);
}
}

class HighWarlordNajentusSoloRaidCreatureScript : public AllCreatureScript
{
public:
    HighWarlordNajentusSoloRaidCreatureScript() : AllCreatureScript("HighWarlordNajentusSoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (creature && creature->GetEntry() == NPC_HIGH_WARLORD_NAJENTUS && creature->IsInCombat() &&
            SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE))
            AnnounceNajentusSoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_HIGH_WARLORD_NAJENTUS)
            najentusSoloAnnouncementSent.erase(creature->GetGUID());
    }
};

class HighWarlordNajentusSoloRaidSpellScript : public AllSpellScript
{
public:
    HighWarlordNajentusSoloRaidSpellScript() : AllSpellScript("HighWarlordNajentusSoloRaidSpellScript", { ALLSPELLHOOK_ON_CALC_MAX_DURATION }) { }

    void OnCalcMaxDuration(Aura const* aura, int32& maxDuration) override
    {
        if (!aura || aura->GetType() != UNIT_AURA_TYPE || aura->GetId() != SPELL_TIDAL_SHIELD || maxDuration <= 0)
            return;

        Unit const* owner = aura->GetUnitOwner();
        Unit const* caster = aura->GetCaster();
        if (!owner || !caster || owner->GetEntry() != NPC_HIGH_WARLORD_NAJENTUS ||
            caster->GetEntry() != NPC_HIGH_WARLORD_NAJENTUS ||
            !SoloRaids::IsSoloMap(owner->GetMap(), SOLO_RAIDS_MAP_BLACK_TEMPLE))
            return;

        maxDuration = int32(float(maxDuration) * SoloRaids::Config::NajentusTidalShieldDurationPct());
    }
};

void AddHighWarlordNajentusSoloRaidScripts()
{
    new HighWarlordNajentusSoloRaidCreatureScript();
    new HighWarlordNajentusSoloRaidSpellScript();
}
