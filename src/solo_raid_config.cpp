#include "solo_raid_config.h"

#include "Config.h"

#include <algorithm>

namespace
{
template <typename T>
T ClampConfig(T value, T minValue, T maxValue)
{
    return std::clamp(value, minValue, maxValue);
}
}

namespace SoloRaids::Config
{
bool DisableTemporusHasten()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.BlackMorass.Temporus.DisableHasten", true);
}

bool DisableAeonusThrash()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.BlackMorass.Aeonus.DisableThrash", true);
}

bool ClearRazorgoreDestroyEggCooldown()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.BlackwingLair.Razorgore.DestroyEgg.ClearCooldown", true);
}

float RazorgoreDestroyEggCastSpeedPct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.BlackwingLair.Razorgore.DestroyEgg.CastSpeedPct", 0.5f), 0.01f, 10.0f);
}

bool DisableVaelastraszBurningAdrenaline()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.BlackwingLair.Vaelastrasz.DisableBurningAdrenaline", true);
}

uint32 EbonrocShadowOfEbonrocDurationMs()
{
    return ClampConfig(sConfigMgr->GetOption<uint32>("SoloRaids.BlackwingLair.Ebonroc.ShadowOfEbonroc.DurationMs", 2000), uint32(0), uint32(600000));
}

bool DisableChromaggusTimeLapse()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.BlackwingLair.Chromaggus.DisableTimeLapse", true);
}

bool DisableTwinEmperorsHealBrother()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.TempleOfAhnQiraj.TwinEmperors.DisableHealBrother", true);
}

float PatchwerkHatefulStrikeDamagePct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.Naxxramas.Patchwerk.HatefulStrike.DamagePct", 0.5f), 0.0f, 10.0f);
}

bool DisableGluthZombieHealing()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.Naxxramas.Gluth.DisableZombieHealing", true);
}

uint8 GluthMortalWoundMaxStacks()
{
    return ClampConfig(sConfigMgr->GetOption<uint8>("SoloRaids.Naxxramas.Gluth.MortalWound.MaxStacks", 3), uint8(0), uint8(255));
}

float GluthEnrageDurationPct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.Naxxramas.Gluth.Enrage.DurationPct", 0.5f), 0.0f, 10.0f);
}

bool DisableThaddiusStaticFieldManaDrain()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.Naxxramas.Thaddius.DisableStaticFieldManaDrain", true);
}

bool DisableRazuviousManaBurn()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.Naxxramas.Razuvious.DisableManaBurn", true);
}

float RazuviousUnbalancingStrikeDamagePct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.Naxxramas.Razuvious.UnbalancingStrike.DamagePct", 0.5f), 0.0f, 10.0f);
}

uint32 KelThuzadGuardianOfIcecrownMaxActive()
{
    return ClampConfig(sConfigMgr->GetOption<uint32>("SoloRaids.Naxxramas.KelThuzad.GuardianOfIcecrown.MaxActive", 2), uint32(0), uint32(100));
}

bool DisableNetherspitePortalBuffs()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.Karazhan.Netherspite.DisablePortalBuffs", true);
}

bool MagtheridonOneCubeBanish()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.MagtheridonsLair.Magtheridon.OneCubeBanish", true);
}

bool DisableMagtheridonBurningAbyssal()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.MagtheridonsLair.Magtheridon.DisableBurningAbyssal", true);
}

float HighKingMaulgarBlindeyeHealingPct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.GruulsLair.HighKingMaulgar.Blindeye.HealingPct", 0.25f), 0.0f, 10.0f);
}

uint32 HighKingMaulgarOlmMaxFelhounds()
{
    return ClampConfig(sConfigMgr->GetOption<uint32>("SoloRaids.GruulsLair.HighKingMaulgar.Olm.MaxFelhounds", 1), uint32(0), uint32(100));
}

bool DisableKarathressLeechingThrow()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.SerpentshrineCavern.FathomLordKarathress.DisableLeechingThrow", true);
}

bool DisableKarathressSummonCyclone()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.SerpentshrineCavern.FathomLordKarathress.DisableSummonCyclone", true);
}

float MorogrimTidalWaveDurationPct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.SerpentshrineCavern.MorogrimTidewalker.TidalWave.DurationPct", 0.25f), 0.0f, 10.0f);
}

bool DisableLadyVashjTaintedCoreParalyze()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.SerpentshrineCavern.LadyVashj.DisableTaintedCoreParalyze", true);
}

bool PreventLadyVashjTaintedElementalDespawn()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.SerpentshrineCavern.LadyVashj.PreventTaintedElementalDespawn", true);
}

float LadyVashjTaintedElementalPoisonBoltRange()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.SerpentshrineCavern.LadyVashj.TaintedElemental.PoisonBoltRange", 60.0f), 0.0f, 50000.0f);
}

uint32 LadyVashjEnchantedElementalMaxActive()
{
    return ClampConfig(sConfigMgr->GetOption<uint32>("SoloRaids.SerpentshrineCavern.LadyVashj.EnchantedElemental.MaxActive", 3), uint32(0), uint32(100));
}

uint32 LadyVashjSporebatMinSpawnIntervalMs()
{
    return ClampConfig(sConfigMgr->GetOption<uint32>("SoloRaids.SerpentshrineCavern.LadyVashj.Sporebat.MinSpawnIntervalMs", 30000), uint32(0), uint32(600000));
}

bool DisableKaelthasRemoteToy()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.TempestKeep.KaelthasSunstrider.DisableRemoteToy", true);
}

bool DisableKaelthasInfinityBladesThrash()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.TempestKeep.KaelthasSunstrider.DisableInfinityBladesThrash", true);
}

bool DisableKaelthasDevastationWhirlwind()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.TempestKeep.KaelthasSunstrider.DisableDevastationWhirlwind", true);
}

float MountHyjalTrashDamagePct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.MountHyjal.Trash.DamagePct", 0.6f), 0.0f, 10.0f);
}

bool DisableMountHyjalRaiseDead()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.MountHyjal.Trash.DisableRaiseDead", true);
}

bool DisableMountHyjalManaBurn()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.MountHyjal.Trash.DisableManaBurn", true);
}

bool PreventMountHyjalGargoyleSpawns()
{
    return sConfigMgr->GetOption<bool>("SoloRaids.MountHyjal.Trash.PreventGargoyleSpawns", true);
}

float KazrogalMarkDurationPct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.MountHyjal.Kazrogal.Mark.DurationPct", 0.25f), 0.0f, 10.0f);
}

float KazrogalCrippleDurationPct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.MountHyjal.Kazrogal.Cripple.DurationPct", 0.5f), 0.0f, 10.0f);
}

float NajentusTidalShieldDurationPct()
{
    return ClampConfig(sConfigMgr->GetOption<float>("SoloRaids.BlackTemple.Najentus.TidalShield.DurationPct", 0.25f), 0.0f, 10.0f);
}
}
