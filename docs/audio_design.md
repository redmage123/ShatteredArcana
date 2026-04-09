# Shattered Arcana - Audio Design Document

## Overview

Audio is managed by `UCoMAudioSubsystem` (a `UGameInstanceSubsystem`) using UE5's
built-in audio system (MetaSounds + Sound Cues). The subsystem provides three
independent audio layers: Music, SFX, and Ambient. If Wwise middleware is
installed later, the public API remains unchanged -- only the internal
implementation switches from USoundBase playback to Wwise event posting.

Sound assets are registered via `TMap<FName, USoundBase*>` properties on the
subsystem. Populate these from a DataAsset, DataTable, or Blueprint defaults.

---

## File Path Conventions

All audio source files live under `Content/Audio/` with this structure:

```
Content/Audio/
  Music/<Category>/SA_<TrackName>          (e.g. SA_OverworldTheme)
  SFX/<Category>/SA_<SoundName>            (e.g. SA_SwordSlash_01)
  Ambient/Planes/<PlaneName>/SA_<Name>     (e.g. SA_AurelithAmbience)
  Ambient/Weather/SA_<WeatherType>         (e.g. SA_RainLoop)
  Ambient/Underdark/SA_<Name>
  Ambient/Underwater/SA_<Name>
```

Naming convention: `SA_` prefix for Sound Assets (UE convention).
Variations use `_01`, `_02` suffixes. Loops have `Loop` in the name.

---

## Music Tracks

| Sound ID                  | Category  | Description                                        | Path                                | Loop  | Duration  |
|---------------------------|-----------|----------------------------------------------------|-------------------------------------|-------|-----------|
| Music_MainMenu            | Menu      | Main menu orchestral theme                         | Music/Menu/SA_MainMenuTheme         | Yes   | ~3:00     |
| Music_Overworld           | Overworld | Default overworld exploration theme                | Music/Overworld/SA_OverworldTheme   | Yes   | ~4:00     |
| Music_Overworld_Aurelith  | Overworld | Aurelith-specific overworld variation              | Music/Overworld/SA_OverworldAurelith| Yes   | ~4:00     |
| Music_Overworld_Noctharion| Overworld | Noctharion dark/ominous overworld theme            | Music/Overworld/SA_OverworldNoctharion | Yes | ~4:00     |
| Music_Overworld_Verdantis | Overworld | Verdantis nature/primal overworld theme            | Music/Overworld/SA_OverworldVerdantis | Yes  | ~4:00     |
| Music_Overworld_Infernyx  | Overworld | Infernyx volcanic/industrial overworld theme       | Music/Overworld/SA_OverworldInfernyx | Yes  | ~4:00     |
| Music_Overworld_Aethermist| Overworld | Aethermist celestial overworld theme               | Music/Overworld/SA_OverworldAethermist | Yes | ~4:00     |
| Music_Overworld_Abyssal   | Overworld | Abyssal chaotic/demon overworld theme              | Music/Overworld/SA_OverworldAbyssal | Yes   | ~4:00     |
| Music_Overworld_Ethereal  | Overworld | Ethereal alien/spirit overworld theme              | Music/Overworld/SA_OverworldEthereal| Yes   | ~4:00     |
| Music_Overworld_Feywild   | Overworld | Feywild whimsical/enchanted overworld theme        | Music/Overworld/SA_OverworldFeywild | Yes   | ~4:00     |
| Music_Combat_Skirmish     | Combat    | Low-intensity combat (small encounters)            | Music/Combat/SA_CombatSkirmish      | Yes   | ~2:30     |
| Music_Combat_Battle       | Combat    | Mid-intensity combat (standard army battles)       | Music/Combat/SA_CombatBattle        | Yes   | ~3:00     |
| Music_Combat_Epic         | Combat    | High-intensity combat (boss, siege, dragon)        | Music/Combat/SA_CombatEpic          | Yes   | ~3:30     |
| Music_Combat_Naval        | Combat    | Naval battle theme                                 | Music/Combat/SA_CombatNaval         | Yes   | ~3:00     |
| Music_Victory             | Victory   | Victory fanfare (game won)                         | Music/Victory/SA_VictoryFanfare     | No    | ~0:45     |
| Music_Victory_Conquest    | Victory   | Conquest victory extended celebration              | Music/Victory/SA_VictoryConquest    | No    | ~1:30     |
| Music_Victory_Spell       | Victory   | Spell of Mastery victory theme                     | Music/Victory/SA_VictorySpell       | No    | ~1:30     |
| Music_Defeat              | Victory   | Defeat / elimination lament                        | Music/Victory/SA_DefeatLament       | No    | ~0:30     |
| Music_Diplomacy           | Overworld | Diplomacy screen / treaty negotiation background   | Music/Overworld/SA_DiplomacyTheme   | Yes   | ~2:00     |

---

## UI Sound Effects

| Sound ID                  | Category | Description                                    | Path                            | Loop | Duration |
|---------------------------|----------|------------------------------------------------|---------------------------------|------|----------|
| SFX_UI_TurnStart          | UI       | New turn notification chime                    | SFX/UI/SA_TurnStart             | No   | ~0.5s    |
| SFX_UI_TurnEnd            | UI       | Turn submission confirmation                   | SFX/UI/SA_TurnEnd               | No   | ~0.3s    |
| SFX_UI_ButtonClick        | UI       | Generic button click                           | SFX/UI/SA_ButtonClick           | No   | ~0.1s    |
| SFX_UI_ButtonHover        | UI       | Button hover highlight                         | SFX/UI/SA_ButtonHover           | No   | ~0.05s   |
| SFX_UI_TabSwitch          | UI       | Panel/tab switch                               | SFX/UI/SA_TabSwitch             | No   | ~0.15s   |
| SFX_UI_Notification       | UI       | Generic notification ping                      | SFX/UI/SA_Notification          | No   | ~0.4s    |
| SFX_UI_Error              | UI       | Error / invalid action buzz                    | SFX/UI/SA_Error                 | No   | ~0.3s    |
| SFX_UI_Victory            | UI       | Victory stinger (layered with music)           | SFX/UI/SA_VictoryStinger        | No   | ~2.0s    |
| SFX_UI_Defeat             | UI       | Defeat stinger                                 | SFX/UI/SA_DefeatStinger         | No   | ~1.5s    |
| SFX_UI_WizardEliminated   | UI       | Enemy wizard eliminated announcement           | SFX/UI/SA_WizardEliminated      | No   | ~1.0s    |
| SFX_UI_ResearchComplete   | UI       | Spell research completed                       | SFX/UI/SA_ResearchComplete      | No   | ~1.0s    |
| SFX_UI_BuildingComplete   | UI       | Building construction finished                 | SFX/UI/SA_BuildingComplete      | No   | ~0.8s    |
| SFX_UI_QuestComplete      | UI       | Quest objective completed                      | SFX/UI/SA_QuestComplete         | No   | ~1.2s    |
| SFX_UI_HeroLevelUp        | UI       | Hero level-up fanfare                          | SFX/UI/SA_HeroLevelUp           | No   | ~1.0s    |
| SFX_UI_DiplomacyOffer     | UI       | Diplomatic offer received                      | SFX/UI/SA_DiplomacyOffer        | No   | ~0.6s    |
| SFX_UI_TreatyBroken       | UI       | Treaty broken / war declared                   | SFX/UI/SA_TreatyBroken          | No   | ~1.0s    |
| SFX_UI_MapOpen            | UI       | World map / strategy overlay opened            | SFX/UI/SA_MapOpen               | No   | ~0.3s    |
| SFX_UI_SpellbookOpen      | UI       | Spellbook interface opened                     | SFX/UI/SA_SpellbookOpen         | No   | ~0.5s    |

---

## Combat SFX

| Sound ID                  | Category | Description                                    | Path                            | Loop | Duration |
|---------------------------|----------|------------------------------------------------|---------------------------------|------|----------|
| SFX_Combat_SwordSlash     | Combat   | Melee sword swing impact                       | SFX/Combat/SA_SwordSlash_01     | No   | ~0.3s    |
| SFX_Combat_BowShot        | Combat   | Arrow release + whoosh                         | SFX/Combat/SA_BowShot_01        | No   | ~0.4s    |
| SFX_Combat_ArrowImpact    | Combat   | Arrow hitting target                           | SFX/Combat/SA_ArrowImpact_01    | No   | ~0.2s    |
| SFX_Combat_ShieldBlock    | Combat   | Shield blocking a blow                         | SFX/Combat/SA_ShieldBlock       | No   | ~0.25s   |
| SFX_Combat_UnitDeath      | Combat   | Generic unit death cry                         | SFX/Combat/SA_UnitDeath_01      | No   | ~0.5s    |
| SFX_Combat_Charge         | Combat   | Cavalry/infantry charge shout                  | SFX/Combat/SA_Charge            | No   | ~1.0s    |
| SFX_Combat_Retreat        | Combat   | Army retreat horn                              | SFX/Combat/SA_Retreat           | No   | ~1.5s    |
| SFX_Combat_SiegeRam       | Combat   | Battering ram impact                           | SFX/Combat/SA_SiegeRam          | No   | ~0.8s    |
| SFX_Combat_SiegeBreak     | Combat   | Wall breach / gate broken                      | SFX/Combat/SA_SiegeBreak        | No   | ~1.2s    |
| SFX_Combat_CannonFire     | Combat   | Siege cannon / catapult fire                   | SFX/Combat/SA_CannonFire        | No   | ~0.6s    |

---

## Magic SFX

| Sound ID                  | Category | Description                                    | Path                            | Loop | Duration |
|---------------------------|----------|------------------------------------------------|---------------------------------|------|----------|
| SFX_Magic_CastGeneric     | Magic    | Generic spellcasting whoosh                    | SFX/Magic/SA_CastGeneric        | No   | ~0.8s    |
| SFX_Magic_CastFire        | Magic    | Fire spell cast (fireball, flame strike)       | SFX/Magic/SA_CastFire           | No   | ~1.0s    |
| SFX_Magic_CastIce         | Magic    | Ice spell cast (frost bolt, blizzard)          | SFX/Magic/SA_CastIce            | No   | ~0.9s    |
| SFX_Magic_CastLightning   | Magic    | Lightning spell cast                           | SFX/Magic/SA_CastLightning      | No   | ~0.6s    |
| SFX_Magic_CastNature      | Magic    | Nature spell cast (vines, growth)              | SFX/Magic/SA_CastNature         | No   | ~1.2s    |
| SFX_Magic_CastDeath       | Magic    | Death/dark spell cast (soul drain, blight)     | SFX/Magic/SA_CastDeath          | No   | ~1.0s    |
| SFX_Magic_CastLife        | Magic    | Life/holy spell cast (heal, bless)             | SFX/Magic/SA_CastLife           | No   | ~1.0s    |
| SFX_Magic_CastChaos       | Magic    | Chaos spell cast (random, wild magic)          | SFX/Magic/SA_CastChaos          | No   | ~0.8s    |
| SFX_Magic_CastSorcery     | Magic    | Sorcery/arcane spell cast (dispel, enchant)    | SFX/Magic/SA_CastSorcery        | No   | ~0.9s    |
| SFX_Magic_Ritual          | Magic    | Ritual channeling loop                         | SFX/Magic/SA_RitualChannel      | Yes  | ~4.0s    |
| SFX_Magic_RitualComplete  | Magic    | Ritual completed successfully                  | SFX/Magic/SA_RitualComplete     | No   | ~2.0s    |
| SFX_Magic_SpellFizzle     | Magic    | Spell fizzle / dispel / counter                | SFX/Magic/SA_SpellFizzle        | No   | ~0.5s    |
| SFX_Magic_Summon          | Magic    | Creature summoning whoosh                      | SFX/Magic/SA_Summon             | No   | ~1.5s    |
| SFX_Magic_Enchant         | Magic    | Item/unit enchantment applied                  | SFX/Magic/SA_Enchant            | No   | ~0.8s    |
| SFX_Magic_GlobalSpell     | Magic    | Global spell impact (world-shaking rumble)     | SFX/Magic/SA_GlobalSpell        | No   | ~2.5s    |
| SFX_Magic_PortalOpen      | Magic    | Portal / ley line activation                   | SFX/Magic/SA_PortalOpen         | No   | ~1.5s    |
| SFX_Magic_PortalTravel    | Magic    | Traveling through a portal whoosh              | SFX/Magic/SA_PortalTravel       | No   | ~1.0s    |

---

## Creature SFX

| Sound ID                  | Category  | Description                                   | Path                              | Loop | Duration |
|---------------------------|-----------|-----------------------------------------------|-----------------------------------|------|----------|
| SFX_Creature_DragonRoar   | Creatures | Dragon roar (attack / arrival)                | SFX/Creatures/SA_DragonRoar       | No   | ~2.0s    |
| SFX_Creature_DragonWings  | Creatures | Dragon wing flap loop                         | SFX/Creatures/SA_DragonWings      | Yes  | ~1.5s    |
| SFX_Creature_DragonBreath | Creatures | Dragon breath weapon                          | SFX/Creatures/SA_DragonBreath     | No   | ~1.5s    |
| SFX_Creature_UndeadMoan   | Creatures | Undead unit ambient moan                      | SFX/Creatures/SA_UndeadMoan       | No   | ~1.0s    |
| SFX_Creature_DemonGrowl   | Creatures | Demon growl / attack                          | SFX/Creatures/SA_DemonGrowl       | No   | ~0.8s    |
| SFX_Creature_ElementalHum | Creatures | Elemental ambient hum                         | SFX/Creatures/SA_ElementalHum     | Yes  | ~2.0s    |
| SFX_Creature_FeyLaugh     | Creatures | Fey creature ethereal laugh                   | SFX/Creatures/SA_FeyLaugh         | No   | ~1.0s    |
| SFX_Creature_GiantStep    | Creatures | Giant / titan footstep (ground shake)         | SFX/Creatures/SA_GiantStep        | No   | ~0.6s    |
| SFX_Creature_SerpentHiss  | Creatures | Sea serpent / snake hiss                      | SFX/Creatures/SA_SerpentHiss      | No   | ~0.8s    |
| SFX_Creature_WolfHowl     | Creatures | Wolf / warg howl                              | SFX/Creatures/SA_WolfHowl         | No   | ~1.5s    |

---

## Building / City SFX

| Sound ID                  | Category | Description                                    | Path                            | Loop | Duration |
|---------------------------|----------|------------------------------------------------|---------------------------------|------|----------|
| SFX_Build_PlaceBuilding   | Building | Building placement confirmation                | SFX/Building/SA_PlaceBuilding   | No   | ~0.5s    |
| SFX_Build_Construction    | Building | Construction in progress (hammering loop)      | SFX/Building/SA_Construction    | Yes  | ~3.0s    |
| SFX_Build_Demolish        | Building | Building demolished / razed                    | SFX/Building/SA_Demolish        | No   | ~1.0s    |
| SFX_Build_CityFounded     | Building | City founding ceremony                         | SFX/Building/SA_CityFounded     | No   | ~2.0s    |
| SFX_Build_CityRazed       | Building | City destroyed / conquered                     | SFX/Building/SA_CityRazed       | No   | ~2.0s    |
| SFX_Build_PopGrowth       | Building | Population growth chime                        | SFX/Building/SA_PopGrowth       | No   | ~0.5s    |

---

## Naval SFX

| Sound ID                  | Category | Description                                    | Path                            | Loop | Duration |
|---------------------------|----------|------------------------------------------------|---------------------------------|------|----------|
| SFX_Naval_ShipLaunch      | Naval    | Ship launch / set sail                         | SFX/Naval/SA_ShipLaunch         | No   | ~1.0s    |
| SFX_Naval_CannonBroadside | Naval    | Ship broadside cannon volley                   | SFX/Naval/SA_CannonBroadside    | No   | ~1.5s    |
| SFX_Naval_ShipSink        | Naval    | Ship sinking (wood cracking, water)            | SFX/Naval/SA_ShipSink           | No   | ~2.0s    |
| SFX_Naval_BoardingAction  | Naval    | Boarding action (grapples, swords on deck)     | SFX/Naval/SA_BoardingAction     | No   | ~1.5s    |
| SFX_Naval_WavesLoop       | Naval    | Ocean waves ambient loop for naval maps        | SFX/Naval/SA_WavesLoop          | Yes  | ~5.0s    |
| SFX_Naval_FogHorn         | Naval    | Fog horn / ship horn                           | SFX/Naval/SA_FogHorn            | No   | ~2.0s    |

---

## Ambient Loops (Per Plane)

| Sound ID                        | Category | Description                                  | Path                                       | Loop | Duration |
|---------------------------------|----------|----------------------------------------------|--------------------------------------------|------|----------|
| Ambient_Aurelith                | Ambient  | Golden sunlight, birdsong, gentle wind       | Ambient/Planes/Aurelith/SA_AurelithLoop    | Yes  | ~60s     |
| Ambient_Noctharion              | Ambient  | Dark whispers, distant thunder, eerie tones  | Ambient/Planes/Noctharion/SA_NoctharionLoop| Yes  | ~60s     |
| Ambient_Verdantis               | Ambient  | Dense forest, insects, animal calls          | Ambient/Planes/Verdantis/SA_VerdantisLoop  | Yes  | ~60s     |
| Ambient_Infernyx                | Ambient  | Lava bubbling, fire crackle, forge hammering | Ambient/Planes/Infernyx/SA_InfernyxLoop    | Yes  | ~60s     |
| Ambient_Aethermist              | Ambient  | Celestial chimes, wind chimes, ethereal hum  | Ambient/Planes/Aethermist/SA_AethermistLoop| Yes  | ~60s     |
| Ambient_Abyssal                 | Ambient  | Demonic drones, distant screams, tremors     | Ambient/Planes/Abyssal/SA_AbyssalLoop      | Yes  | ~60s     |
| Ambient_Ethereal                | Ambient  | Alien reverb, crystalline tones, void wind   | Ambient/Planes/Ethereal/SA_EtherealLoop    | Yes  | ~60s     |
| Ambient_Feywild                 | Ambient  | Tinkling bells, distant flutes, rustling     | Ambient/Planes/Feywild/SA_FeywildLoop      | Yes  | ~60s     |

---

## Weather Ambient Layers

| Sound ID                  | Category | Description                                    | Path                               | Loop | Duration |
|---------------------------|----------|------------------------------------------------|------------------------------------|------|----------|
| Ambient_Weather_Rain      | Weather  | Rain loop (light to medium)                    | Ambient/Weather/SA_RainLoop        | Yes  | ~30s     |
| Ambient_Weather_Storm     | Weather  | Thunderstorm loop (heavy rain + thunder)       | Ambient/Weather/SA_StormLoop       | Yes  | ~45s     |
| Ambient_Weather_Wind      | Weather  | Strong wind loop                               | Ambient/Weather/SA_WindLoop        | Yes  | ~30s     |
| Ambient_Weather_Snow      | Weather  | Gentle snowfall with muffled wind              | Ambient/Weather/SA_SnowLoop        | Yes  | ~30s     |
| Ambient_Weather_Blizzard  | Weather  | Blizzard howling wind                          | Ambient/Weather/SA_BlizzardLoop    | Yes  | ~30s     |
| Ambient_Weather_ArcaneStorm | Weather | Magical storm (crackling energy + thunder)   | Ambient/Weather/SA_ArcaneStormLoop | Yes  | ~30s     |

---

## Underdark / Underwater Ambient

| Sound ID                       | Category   | Description                                 | Path                                  | Loop | Duration |
|--------------------------------|------------|---------------------------------------------|---------------------------------------|------|----------|
| Ambient_Underdark_Caves        | Underdark  | Dripping water, echoing footsteps           | Ambient/Underdark/SA_CavesLoop        | Yes  | ~60s     |
| Ambient_Underdark_DwarvenHalls | Underdark  | Distant mining, forge ambience              | Ambient/Underdark/SA_DwarvenHallsLoop | Yes  | ~60s     |
| Ambient_Underwater_Depths      | Underwater | Muffled water currents, whale calls         | Ambient/Underwater/SA_DepthsLoop      | Yes  | ~60s     |
| Ambient_Underwater_Shallows    | Underwater | Near-surface bubbles, light water movement  | Ambient/Underwater/SA_ShallowsLoop    | Yes  | ~60s     |

---

## Integration Points

The `UCoMAudioSubsystem` is triggered from the following game systems:

1. **Turn Manager** (`CoMTurnManager.cpp`, `CoMTurnSubsystem.cpp`):
   - Turn start -> `PlayUISound("SFX_UI_TurnStart")`
   - Combat phase -> `SetCombatIntensity(2)` (battle music)
   - Post-combat -> `SetCombatIntensity(0)` (peace / overworld)
   - Victory detected -> `PlayMusic("Music_Victory")` + `PlayUISound("SFX_UI_Victory")`

2. **Plane transitions** (future integration):
   - Entering a new plane -> `SetAmbientPreset(ECoMPlane::X)`
   - Overworld music per plane -> `PlayMusic("Music_Overworld_<PlaneName>")`

3. **Combat Subsystem** (future integration):
   - Encounter begins -> `SetCombatIntensity(1-3)` based on army size
   - Encounter ends -> `SetCombatIntensity(0)`

4. **UI System** (future integration):
   - Button clicks, panel transitions -> `PlayUISound("SFX_UI_*")`

5. **Magic Subsystem** (future integration):
   - Spell cast -> `PlaySFX("SFX_Magic_Cast<School>", CasterLocation)`
   - Ritual complete -> `PlaySFX("SFX_Magic_RitualComplete", ...)`

6. **City / Building** (future integration):
   - Building placed -> `PlayUISound("SFX_Build_PlaceBuilding")`
   - City founded -> `PlayUISound("SFX_Build_CityFounded")`

---

## Volume Defaults

| Channel | Default | Description                        |
|---------|---------|------------------------------------|
| Master  | 1.0     | Global multiplier for all audio    |
| Music   | 0.7     | Background music level             |
| SFX     | 1.0     | Sound effects and UI sounds        |
| Ambient | 0.42    | Derived: Music * 0.6 (internal)    |

---

## Wwise Migration Path

If Wwise is installed later:
1. Add AkAudioEvent references instead of USoundBase in the TMap properties
2. Replace `UAudioComponent::SetSound/FadeIn` calls with `UAkComponent::PostAkEvent`
3. Replace `UGameplayStatics::PlaySoundAtLocation` with `UAkGameplayStatics::PostEventAtLocation`
4. Map combat intensity to Wwise RTPC parameters instead of track switching
5. Map ambient presets to Wwise switch groups for seamless layered transitions
