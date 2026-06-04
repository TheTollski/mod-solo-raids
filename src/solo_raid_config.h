#ifndef MOD_SOLO_RAIDS_SOLO_RAID_CONFIG_H
#define MOD_SOLO_RAIDS_SOLO_RAID_CONFIG_H

#include "Define.h"

namespace SoloRaids::Config
{
bool ClearRazorgoreDestroyEggCooldown();
float RazorgoreDestroyEggCastSpeedPct();

bool DisableVaelastraszBurningAdrenaline();
uint32 EbonrocShadowOfEbonrocDurationMs();
bool DisableChromaggusTimeLapse();

bool DisableTwinEmperorsHealBrother();

float PatchwerkHatefulStrikeDamagePct();
bool DisableGluthZombieHealing();
uint8 GluthMortalWoundMaxStacks();
bool DisableThaddiusStaticFieldManaDrain();
bool DisableRazuviousManaBurn();
float RazuviousUnbalancingStrikeDamagePct();
uint32 KelThuzadGuardianOfIcecrownMaxActive();
}

#endif
