# mod-solo-raids

Small encounter-specific raid adjustments for solo play.

## Configuration

Copy `conf/solo_raids.conf.dist` to your worldserver module config directory as `solo_raids.conf`.
Each tweak below has a matching `SoloRaids.*` option in that file.
Percentage options use fractional values, so `0.5` means 50%.

## Vanilla Dungeons

## TBC Dungeons

### Black Morass

#### Temporus
- Temporus's Hasten can be disabled in a solo dungeon instance.

#### Aeonus
- Aeonus's Thrash can be disabled in a solo dungeon instance.

## Raids

### Blackwing Lair

#### Razorgore the Untamed
- Razorgore the Untamed gets configurable Destroy Egg cast time while controlled in a solo raid instance.
- Razorgore's Destroy Egg spell is flagged so that Razorgore's solo casting speed changes its real server cast timer.
- Razorgore's Destroy Egg spell cooldown can be cleared after each successful cast in a solo raid instance.

#### Vaelastrasz the Corrupt
- Vaelastrasz the Corrupt's Burning Adrenaline can be disabled in a solo raid instance.

#### Ebonroc
- Ebonroc's Shadow of Ebonroc duration is configurable in a solo raid instance.

#### Chromaggus
- Chromaggus's Time Lapse can be disabled in a solo raid instance.

### Temple of Ahn'Qiraj

#### Twin Emperors
- Twin Emperors' Heal Brother can be disabled in a solo raid instance.

### Naxxramas

#### Gluth
- Gluth's zombie healing can be disabled in a solo raid instance.
- Gluth's Mortal Wound stack cap is configurable on the solo player.
- Gluth's Enrage duration percentage is configurable in a solo raid instance.

#### Patchwerk
- Patchwerk's Hateful Strike damage percentage is configurable against the solo player.

#### Kel'Thuzad
- Kel'Thuzad's Guardian of Icecrown active spawn cap is configurable in a solo raid instance.

#### Instructor Razuvious
- Instructor Razuvious's Mana Burn can be disabled in a solo raid instance.
- Instructor Razuvious's Unbalancing Strike damage percentage is configurable against the solo player.

#### Thaddius
- Feugen's Static Field mana drain can be disabled for the solo player.

### Karazhan

#### Netherspite
- Netherspite's portal beam buffs can be disabled while player beam buffs and exhaustion remain unchanged.

### Magtheridon's Lair

#### Magtheridon
- One Manticron Cube channel can be sufficient to banish Magtheridon in a solo raid instance.
- Hellfire Channelers' Burning Abyssal summon can be disabled in a solo raid instance.

### Gruul's Lair

#### High King Maulgar
- Blindeye the Seer's healing percentage is configurable in a solo raid instance.
- The number of active felhounds summoned by Olm is configurable in a solo raid instance.

### Serpentshrine Cavern

#### Fathom-Lord Karathress
- Fathom-Guard Sharkkis's Leeching Throw can be disabled in a solo raid instance.
- Fathom-Guard Caribdis's Summon Cyclone can be disabled in a solo raid instance.

#### Morogrim Tidewalker
- Tidal Wave's debuff duration percentage is configurable on the solo player.

#### Lady Vashj
- Tainted Core's Paralyze effect can be disabled in a solo raid instance.
- Tainted Elementals can be prevented from despawning, and their Poison Bolt range is configurable.
- The number of active Enchanted Elementals is configurable in a solo raid instance.
- Toxic Sporebats have a configurable minimum spawn interval in a solo raid instance.

### Tempest Keep

#### Kael'thas Sunstrider
- Master Engineer Telonicus's Remote Toy can be disabled in a solo raid instance.
- Infinity Blades' Thrash can be disabled in a solo raid instance.
- Devastation's Whirlwind can be disabled in a solo raid instance.

### Mount Hyjal

#### Wave Trash
- Damage dealt by wave trash against all targets is configurable in a solo raid instance.
- Shadowy Necromancers' Raise Dead spells can be disabled in a solo raid instance.
- Fel Stalkers' Mana Burn can be disabled in a solo raid instance.
- Gargoyles can be prevented from participating in waves in a solo raid instance.

#### Kaz'rogal
- Mark of Kaz'rogal's duration percentage is configurable on the solo player.
- Cripple's debuff duration percentage is configurable on the solo player.

### Black Temple

#### High Warlord Naj'entus
- Tidal Shield's duration percentage is configurable in a solo raid instance.
