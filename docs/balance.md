# Gameplay balance

`balance.cfg` beside the executable is the shared source for gameplay tuning.
Edit it, save it, and restart the game. No recompilation is needed. The supplied
file reproduces the previous gameplay values exactly.

The application loads it before constructing gameplay objects. Configuration is
fixed for that process: changing the file during play, restarting a checkpoint,
or changing levels does not change the rules midway through a run.

## Editing the file

Use `key = number`, with one assignment per line. Blank lines and `#` comments
are accepted, including comments after a value. Decimal numbers use a dot.
Every shipped setting has a comment giving its units and allowed range. Missing
keys retain their original defaults, so a small override file is valid.

For example, this increases sprint endurance and slightly reduces pursuit speed:

```ini
stamina.drain_per_second = 10
stamina.recovery_delay = 1.2
aranea.chase_speed = 3.2
```

The available groups are:

| Group | Parameters |
| --- | --- |
| `player` | Walking, sprinting and crouching speed; acceleration and braking |
| `stamina` | Drain, recovery, recovery delay and exhaustion recovery threshold |
| `aranea` | Patrol/chase speed, key-triggered multiplier, spawn grace/range, sight, search memory, contact distance, footstep/door/wall hearing, local alarm range/lifetime and turn rates |
| `mouse` | Approach and escape speeds |
| `partygoer` | Ordinary/alarm speeds and alarm duration |
| `darkness` | Pursuit speed, flashlight repulsion range and contact distance |

Stamina remains measured on the existing 0–100 scale. Camera preferences,
brightness, volume and frame limits remain in `settings.cfg`; jumpscare timing
and camera treatment remain in `jumpscare.cfg`. Collider sizes, room dimensions,
pathfinding budgets, streaming limits, visual animation constants and procedural
seed rules are not balance settings.

## Validation and fallback

Unknown or repeated keys, malformed numbers, NaN/infinity, trailing junk,
out-of-range values, oversized lines/files and contradictory values reject the
whole candidate. The log reports the file, line and reason, then the game uses
all built-in defaults. A missing file also uses defaults. No valid-looking
subset of a rejected file is applied.

Cross-field rules require crouch ≤ walk ≤ sprint, braking ≥ acceleration,
patrol ≤ chase, minimum spawn distance ≤ maximum, the hearing cap ≥ its base,
chase turning ≥ patrol turning, mouse escape ≥ approach, and Partygoer alarm
speed ≥ ordinary speed. Enraged chase speed is capped at 12 m/s. Sight remains
within the existing 24 m visibility query. Mouse and Parking-eye speeds have
conservative caps for their existing collision steps. A relation involving an omitted key
uses that key's default; diagnostics may label it line 0.

A file may contain at most 256 lines and 4096 characters per line. Fix an invalid
file and start a new process to try it again. Configuration is never silently
clamped or reloaded while a chase is in progress.

## Code and checks

`common/balance.h` contains the typed defaults, schema, pure parser and startup
loader. `balance()` exposes only a const reference during simulation. Existing
movement and actor constants are read-only aliases into that fixed configuration,
so all levels use the same validated values. No new library is required.

`tests/balance_tests.cpp` checks the shipped defaults, malformed and cross-field
validation, atomic fallback, and actual changes to movement, stamina, enemy
perception and chase displacement under an alternate configuration. Its
`--invalid-startup` and `--missing-startup` modes exercise independent process
startup fallback. Test execution should use the project directory so the shipped
`balance.cfg` can be checked.
