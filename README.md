# mod-solo-raids

Small encounter-specific raid adjustments for solo play.

## Configuration

Copy `conf/solo_raids.conf.dist` to your worldserver module config directory as `solo_raids.conf`.
Each tweak below has a matching `SoloRaids.*` option in that file.
Percentage options use fractional values, so `0.5` means 50%.

## Blackwing Lair

### Razorgore the Untamed
- Razorgore the Untamed gets configurable Destroy Egg cast time while controlled in a solo raid instance.
- Razorgore's Destroy Egg spell is flagged so that Razorgore's solo casting speed changes its real server cast timer.
- Razorgore's Destroy Egg spell cooldown can be cleared after each successful cast in a solo raid instance.

### Vaelastrasz the Corrupt
- Vaelastrasz the Corrupt's Burning Adrenaline can be disabled in a solo raid instance.

### Ebonroc
- Ebonroc's Shadow of Ebonroc duration is configurable in a solo raid instance.

### Chromaggus
- Chromaggus's Time Lapse can be disabled in a solo raid instance.

## Temple of Ahn'Qiraj

### Twin Emperors
- Twin Emperors' Heal Brother can be disabled in a solo raid instance.

## Naxxramas

### Gluth
- Gluth's zombie healing can be disabled in a solo raid instance.
- Gluth's Mortal Wound stack cap is configurable on the solo player.
- Gluth's Enrage duration percentage is configurable in a solo raid instance.

### Patchwerk
- Patchwerk's Hateful Strike damage percentage is configurable against the solo player.

### Kel'Thuzad
- Kel'Thuzad's Guardian of Icecrown active spawn cap is configurable in a solo raid instance.

### Instructor Razuvious
- Instructor Razuvious's Mana Burn can be disabled in a solo raid instance.
- Instructor Razuvious's Unbalancing Strike damage percentage is configurable against the solo player.

### Thaddius
- Feugen's Static Field mana drain can be disabled for the solo player.
