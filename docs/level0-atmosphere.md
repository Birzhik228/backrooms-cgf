# Level 0 atmosphere — 19 October 2026

Level 0 now adds sparse room sounds, muffled footsteps behind walls and a quiet
hint from Aranea's actual position. The yellow interior and music settings are
preserved. These events do not summon or move the creature.

## Events and placement

- Soft ventilation/settling, an occasional creak and a water drop come from
  nearby loaded rooms, 4–10 metres away. The first event is due after 9–15
  active seconds; subsequent attempts are 16–28 seconds apart, with no
  immediate repetition of the same ambient type.
- A short sequence of two or three soft soil contacts comes from walkable
  floor 7–16 metres away, behind one or two real walls or closed doors. Its
  first attempt is after 28–42 seconds, then 48–78 seconds after a successful
  placement. It is atmosphere rather than a second simulated enemy.
- A brief breath/drag comes from the real Aranea only when it is 10–22 metres
  away behind one barrier. No cue is fabricated for an absent creature. After
  a successful hint, another cannot occur for 35–60 seconds.

All events share at least seven seconds of separation. Placement failure is
skipped and retried later; each attempt checks at most 16 candidate positions.
No extra world chunks are generated. Double-precision world positions survive
render-origin rebasing. The selected source has its own wall transmission;
it does not borrow Aranea's occlusion value.

Chase, search, a visible or very close creature, and hiding suppress atmosphere.
After these conditions end, eight quiet seconds pass before timers resume.
Existing atmosphere tails fade down when danger begins. Actual creature steps,
sighting cries and capture audio keep their original priority and levels.
Menus pause the timers; death and other levels prevent events. Returning to the
entrance or starting a fresh run resets the seeded schedule.

The old random ambient/stinger scheduler is disabled only on Level 0 so that
two systems cannot pile sounds together. Fluorescent hum and the existing
tension drone remain. Level Fun and Parking retain their existing scheduler.

## Spawn distance

Ordinary Aranea spawns now select hidden, reachable positions **24–32 metres**
from the player, previously 16–22 metres. Candidates also stay at least 24
metres from the initial player position, recorded by the first valid update
or an earlier alarm. This prevents moving away from the entrance from allowing
a new spawn beside it. The eight-second grace period, key/mouse alarms, sight,
movement speeds, eight candidate attempts and 4096-node path budget remain.
The entrance is not permanently safe: a creature can walk back into it.
Explicit developer `--threat-x/--threat-z` fixtures keep their requested pose.

## Audio and checks

Ventilation and organic rustle are synthesized once when constructing the
mixer. Distant contacts reuse the credited Aranea soil recordings with filtering
and irregular gaps. No new download or runtime audio library is required.
All cues obey stereo position, master volume, mute, pause and exclusive capture.

`build.ps1 -Test` includes `tests/atmosphere_tests.cpp` alongside the expanded
audio and spawn tests. `tools/verify-atmosphere.ps1` runs room sounds, natural
spawning, paused play, both later levels and capture/restart in the actual game.
`tests/audio_tests.exe --level0-preview tests/artifacts/atmosphere` writes the
software mix for listening, with real distance and wall gains.

Automated verification checks timing, collision barriers, isolation and PCM
levels; it is not a physical listening test or a full manual playthrough.

Verified on 19 October 2026: the rebuilt game and the atmosphere, audio, threat
and progression native suites passed. The six rendered scenarios completed
with zero OpenGL errors: a 60-second room fixture emitted two ambient events
and one behind-wall step sequence; the first natural/alarm spawn was 30.64 m
away; pause, Fun and Parking emitted no Level 0 events; capture/restart worked.
The spawn tests cover eight seeds and moving away from the initial position.
An AI/world probe also produced real occluded-creature hints before pursuit.

At master 0.75, the unnormalized software-mixer preview measures about 234 PCM
RMS for ventilation at 6 m, 110 for soil steps at 10 m through one wall, and
99 for the rustle at 14 m through one wall. Real creature and capture PCM remain
unchanged by atmosphere ducking. The hidden staging runs could not initialize
Windows MCI music (error 277); music playback is not counted as a pass there.
