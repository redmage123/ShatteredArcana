extends Node
## Global signal-based event bus replacing UE5 delegates.
## All subsystems emit and listen via this singleton.

# ---------------------------------------------------------------------------
# Turn lifecycle
# ---------------------------------------------------------------------------
signal turn_started(turn: int)
signal turn_ended(turn: int)

# ---------------------------------------------------------------------------
# Unit / army events
# ---------------------------------------------------------------------------
signal unit_spawned(unit_id: int)
signal unit_killed(unit_id: int)
signal army_moved(army_id: int, from_pos: Vector2i, to_pos: Vector2i)

# ---------------------------------------------------------------------------
# Combat
# ---------------------------------------------------------------------------
signal combat_resolved(result: Dictionary)

# ---------------------------------------------------------------------------
# City / production
# ---------------------------------------------------------------------------
signal city_production_complete(city_id: int, item: String)
signal city_founded(city_id: int)

# ---------------------------------------------------------------------------
# Magic
# ---------------------------------------------------------------------------
signal spell_cast(wizard_id: int, spell_id: String)
signal spell_researched(wizard_id: int, spell_id: String)
signal mana_node_claimed(wizard_id: int, plane: int, pos: Vector2i)

# ---------------------------------------------------------------------------
# Quests
# ---------------------------------------------------------------------------
signal quest_available(quest_id: int)
signal quest_completed(quest_id: int)
signal quest_failed(quest_id: int)

# ---------------------------------------------------------------------------
# Diplomacy
# ---------------------------------------------------------------------------
signal war_declared(attacker_id: int, defender_id: int)
signal peace_made(wizard_a: int, wizard_b: int)

# ---------------------------------------------------------------------------
# Game end
# ---------------------------------------------------------------------------
signal wizard_eliminated(wizard_id: int)
signal victory_achieved(wizard_id: int, victory_type: String)

# ---------------------------------------------------------------------------
# Navigation
# ---------------------------------------------------------------------------
signal plane_changed(plane_index: int)
