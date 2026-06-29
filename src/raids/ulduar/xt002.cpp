#include "../../solo_raid_config.h"
#include "../../solo_raid_utils.h"

#include "Creature.h"
#include "ObjectGuid.h"
#include "Player.h"
#include "ScriptMgr.h"

#include <algorithm>
#include <map>
#include <set>
#include <string>

namespace
{
constexpr uint32 NPC_XT002 = 33293;
constexpr uint32 SPELL_HEARTBREAK = 65737;
constexpr uint32 SOLO_RAIDS_MAP_ULDUAR = 603;

std::set<ObjectGuid> xt002SoloAnnouncementSent;
std::map<ObjectGuid, uint32> xt002LastHealth;

void ReduceScrapbotHealing(Creature* xt002)
{
    if (!xt002 || xt002->GetEntry() != NPC_XT002)
        return;

    ObjectGuid const guid = xt002->GetGUID();

    if (!xt002->IsInCombat() || !SoloRaids::IsSoloMap(xt002->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
    {
        xt002LastHealth[guid] = xt002->GetHealth();
        return;
    }

    uint32 const currentHealth = xt002->GetHealth();
    auto stateItr = xt002LastHealth.find(guid);
    if (stateItr == xt002LastHealth.end())
    {
        xt002LastHealth[guid] = currentHealth;
        return;
    }

    uint32 const previousHealth = stateItr->second;
    if (currentHealth <= previousHealth || xt002->HasAura(SPELL_HEARTBREAK))
    {
        stateItr->second = currentHealth;
        return;
    }

    float const healingPct = SoloRaids::Config::XT002ScrapbotHealingPct();
    if (healingPct >= 1.0f)
    {
        stateItr->second = currentHealth;
        return;
    }

    uint32 const healthIncrease = currentHealth - previousHealth;
    uint32 const allowedIncrease = uint32(float(healthIncrease) * healingPct);
    uint32 const adjustedHealth = std::min(xt002->GetMaxHealth(), previousHealth + allowedIncrease);

    xt002->SetHealth(std::max<uint32>(1, adjustedHealth));
    stateItr->second = xt002->GetHealth();
}

void AnnounceXT002SoloTweaks(Creature* xt002)
{
    if (!xt002)
        return;

    ObjectGuid const guid = xt002->GetGUID();
    if (xt002SoloAnnouncementSent.count(guid) != 0)
        return;

    Player* player = SoloRaids::GetSoloPlayer(xt002->GetMap(), SOLO_RAIDS_MAP_ULDUAR);
    if (!player)
        return;

    player->SendSystemMessage(("mod-solo-raids active: Ulduar solo tweaks enabled for XT-002 Deconstructor. Scrapbot healing set to " +
        std::to_string(uint32(SoloRaids::Config::XT002ScrapbotHealingPct() * 100.0f)) + "%.").c_str());
    xt002SoloAnnouncementSent.insert(guid);
}
}

class XT002SoloRaidCreatureScript : public AllCreatureScript
{
public:
    XT002SoloRaidCreatureScript() : AllCreatureScript("XT002SoloRaidCreatureScript") { }

    void OnAllCreatureUpdate(Creature* creature, uint32 /*diff*/) override
    {
        if (!creature || creature->GetEntry() != NPC_XT002)
            return;

        ReduceScrapbotHealing(creature);

        if (creature->IsInCombat() && SoloRaids::IsSoloMap(creature->GetMap(), SOLO_RAIDS_MAP_ULDUAR))
            AnnounceXT002SoloTweaks(creature);
    }

    void OnCreatureRemoveWorld(Creature* creature) override
    {
        if (creature && creature->GetEntry() == NPC_XT002)
        {
            xt002SoloAnnouncementSent.erase(creature->GetGUID());
            xt002LastHealth.erase(creature->GetGUID());
        }
    }
};

void AddXT002SoloRaidScripts()
{
    new XT002SoloRaidCreatureScript();
}
