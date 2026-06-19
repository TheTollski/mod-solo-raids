#ifndef MOD_SOLO_RAIDS_SOLO_RAID_CONFIG_H
#define MOD_SOLO_RAIDS_SOLO_RAID_CONFIG_H

#include "Define.h"

namespace SoloRaids::Config
{
bool DisableTemporusHasten();
bool DisableAeonusThrash();

bool ClearRazorgoreDestroyEggCooldown();
float RazorgoreDestroyEggCastSpeedPct();

bool DisableVaelastraszBurningAdrenaline();
uint32 EbonrocShadowOfEbonrocDurationMs();
bool DisableChromaggusTimeLapse();

bool DisableTwinEmperorsHealBrother();

float PatchwerkHatefulStrikeDamagePct();
bool DisableGluthZombieHealing();
uint8 GluthMortalWoundMaxStacks();
float GluthEnrageDurationPct();
bool DisableThaddiusStaticFieldManaDrain();
bool DisableRazuviousManaBurn();
float RazuviousUnbalancingStrikeDamagePct();
uint32 KelThuzadGuardianOfIcecrownMaxActive();

bool DisableNetherspitePortalBuffs();
bool MagtheridonOneCubeBanish();
bool DisableMagtheridonBurningAbyssal();

float HighKingMaulgarBlindeyeHealingPct();
uint32 HighKingMaulgarOlmMaxFelhounds();

bool DisableKarathressLeechingThrow();
bool DisableKarathressSummonCyclone();
float MorogrimTidalWaveDurationPct();
bool DisableLadyVashjTaintedCoreParalyze();
bool PreventLadyVashjTaintedElementalDespawn();
float LadyVashjTaintedElementalPoisonBoltRange();
uint32 LadyVashjEnchantedElementalMaxActive();
uint32 LadyVashjSporebatMinSpawnIntervalMs();

bool DisableKaelthasRemoteToy();
bool DisableKaelthasInfinityBladesThrash();
bool DisableKaelthasDevastationWhirlwind();
bool DisableSunbladeCabalistIgniteMana();
bool CompleteKalecgosSoloOnBanish();
uint8 BrutallusMeteorSlashMaxStacks();
float BrutallusMeleeDamagePct();
float BrutallusMaxHealthPct();
float MuruShadowswordAddsMaxHealthPct();
float MuruShadowswordAddsDamagePct();
bool PreventMuruDarkFiendSpawns();

float MountHyjalTrashDamagePct();
bool DisableMountHyjalRaiseDead();
bool DisableMountHyjalManaBurn();
bool PreventMountHyjalGargoyleSpawns();
float KazrogalMarkDurationPct();
float KazrogalCrippleDurationPct();

float NajentusTidalShieldDurationPct();
uint8 GurtoggAcidicWoundMaxStacks();
bool DisableGurtoggBewilderingStrike();
bool DisableEssenceOfDesireSpiritShock();
float EssenceOfDesireRuneShieldDurationPct();
float FlameOfAzzinothDamagePct();
uint8 IllidanAuraOfDreadMaxStacks();
bool DisableIllidanSummonShadowDemon();
}

#endif
