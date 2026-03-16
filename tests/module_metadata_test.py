#!/usr/bin/env python3
import json
import sys
from pathlib import Path


def fail(msg: str) -> None:
    print(f"FAIL: {msg}", file=sys.stderr)
    raise SystemExit(1)


module_json_path = Path(__file__).resolve().parents[1] / "src" / "module.json"
module = json.loads(module_json_path.read_text())

if module.get("id") != "freak":
    fail("module id must be 'freak'")

if module.get("name") != "Freak":
    fail("module name must be 'Freak'")

cap = module.get("capabilities", {})
if cap.get("component_type") != "sound_generator":
    fail("component_type must be sound_generator")

chain_params = {p.get("key"): p for p in cap.get("chain_params", []) if isinstance(p, dict)}

required_keys = [
    "model",
    "pitch",
    "harmonics",
    "timbre",
    "morph",
    "fm_amount",
    "filter_mode",
    "filter_cutoff",
    "filter_resonance",
    "lpg_decay",
    "lpg_color",
    "assign1_target",
    "assign2_target",
    "voice_mode",
    "polyphony",
    "unison",
    "detune",
    "spread",
    "glide_ms",
]
for key in required_keys:
    if key not in chain_params:
        fail(f"missing chain param: {key}")

model_meta = chain_params.get("model", {})
if model_meta.get("type") != "enum":
    fail("model param must be enum")
model_opts = model_meta.get("options", [])
if not isinstance(model_opts, list) or len(model_opts) != 24:
    fail("model enum must define exactly 24 engine names")
if "FM 2-Op" not in model_opts:
    fail("model enum options missing expected engine name 'FM 2-Op'")
for opt in model_opts:
    if not isinstance(opt, str):
        fail("model enum options must be strings")
    if "_" in opt:
        fail("model enum options should use clean labels without underscores")

assign_target_opts = chain_params.get("assign1_target", {}).get("options", [])
if len(assign_target_opts) != 10:
    fail("assign target enum must expose 10 options")
for option in ["off", "morph", "fm_amount", "lpg_decay", "lpg_color", "filter_resonance", "filter_cutoff"]:
    if option not in assign_target_opts:
        fail(f"assign target enum missing option: {option}")

dests = ["pitch", "harmonics", "timbre", "cutoff", "assign1", "assign2"]
sources = ["lfo", "env", "cycle_env", "random", "velocity", "poly_aftertouch"]
for d in dests:
    for s in sources:
        key = f"{d}_mod_{s}_amt"
        if key not in chain_params:
            fail(f"missing modulation amount key: {key}")

for old_prefix in ["morph_mod_", "fm_mod_", "color_mod_"]:
    for key in chain_params.keys():
        if key.startswith(old_prefix):
            fail(f"old modulation key should not exist: {key}")

levels = cap.get("ui_hierarchy", {}).get("levels", {})
root = levels.get("root", {})
entries = root.get("params", [])

main_level = levels.get("main", {})
expected_main_params = ["model", "pitch", "harmonics", "timbre", "morph", "fm_amount", "lpg_decay", "lpg_color"]
if main_level.get("params") != expected_main_params:
    fail(f"main level params must be {expected_main_params}")

for key in expected_main_params:
    if key in entries:
        fail(f"{key} should not be a direct root parameter when Main submenu is present")

labels = [e.get("label") for e in entries if isinstance(e, dict)]
if "Main" not in labels:
    fail("missing root submenu: Main")
if "Mod" not in labels:
    fail("missing root submenu: Mod")
if "Filter" not in labels:
    fail("missing root submenu: Filter")
if "Mod Sources" not in labels:
    fail("missing root submenu: Mod Sources")
if "Voice" not in labels:
    fail("missing root submenu: Voice")

for forbidden in ["Pitch Mod", "Harmonics Mod", "Timbre Mod", "Morph Mod", "FM Mod", "Color Mod", "Macros", "Init / Randomize"]:
    if forbidden in labels:
        fail(f"forbidden root submenu present: {forbidden}")

if labels.index("Filter") > labels.index("Mod"):
    fail("Filter submenu must appear above Mod submenu")
if labels.index("Mod") > labels.index("Mod Sources"):
    fail("Mod submenu must appear above Mod Sources")
if labels.index("Main") > labels.index("Filter"):
    fail("Main submenu must appear above Filter submenu")

root_knobs = root.get("knobs", [])
expected_root_knobs = [
    "model",
    "harmonics",
    "timbre",
    "morph",
    "fm_amount",
    "lpg_decay",
    "lpg_color",
    "filter_cutoff",
]
if root_knobs != expected_root_knobs:
    fail(f"root knobs must be {expected_root_knobs}, got {root_knobs}")

mod_level = levels.get("mod", {})
mod_entries = mod_level.get("params", [])
mod_labels = [e.get("label") for e in mod_entries if isinstance(e, dict)]
expected_mod_labels = ["Pitch", "Harmonics", "Timbre", "Cutoff", "Assign 1", "Assign 2"]
if mod_labels != expected_mod_labels:
    fail(f"mod submenu order must be {expected_mod_labels}, got {mod_labels}")

expected_mod_level_params = {
    "assign1_mod": ["assign1_target"] + [f"assign1_mod_{s}_amt" for s in sources],
    "assign2_mod": ["assign2_target"] + [f"assign2_mod_{s}_amt" for s in sources],
    "pitch_mod": [f"pitch_mod_{s}_amt" for s in sources],
    "harmonics_mod": [f"harmonics_mod_{s}_amt" for s in sources],
    "timbre_mod": [f"timbre_mod_{s}_amt" for s in sources],
    "cutoff_mod": [f"cutoff_mod_{s}_amt" for s in sources],
}
for level_name, expected_params in expected_mod_level_params.items():
    got = levels.get(level_name, {}).get("params", [])
    if got != expected_params:
        fail(f"{level_name} params mismatch, expected {expected_params}, got {got}")

expected_mod_names = {
    "lfo": "LFO",
    "env": "Env",
    "cycle_env": "Cycle Env",
    "random": "Random",
    "velocity": "Velocity",
    "poly_aftertouch": "Poly Aftertouch",
}

for dest in ["pitch", "harmonics", "timbre", "cutoff", "assign1", "assign2"]:
    for src_key, expected_name in expected_mod_names.items():
        key = f"{dest}_mod_{src_key}_amt"
        meta = chain_params.get(key, {})
        if meta.get("name") != expected_name:
            fail(f"{key} should have name '{expected_name}'")

for src_key in sources:
    key = f"pitch_mod_{src_key}_amt"
    meta = chain_params.get(key, {})
    if meta.get("min") != -48.0 or meta.get("max") != 48.0:
        fail(f"{key} pitch modulation range should be -48..48")
    if meta.get("step") != 0.1:
        fail(f"{key} pitch modulation step should be 0.1")

for dest in ["harmonics", "timbre", "cutoff", "assign1", "assign2"]:
    for src_key in sources:
        key = f"{dest}_mod_{src_key}_amt"
        meta = chain_params.get(key, {})
        if meta.get("min") != -1.0 or meta.get("max") != 1.0:
            fail(f"{key} range should be -1..1")

glide_meta = chain_params.get("glide_ms", {})
if glide_meta.get("type") != "int":
    fail("glide_ms must be int")
if glide_meta.get("step") != 5:
    fail("glide_ms step must be 5ms")

for key, min_value in [
    ("env_attack_ms", 0),
    ("env_decay_ms", 0),
    ("env_release_ms", 0),
    ("cycle_attack_ms", 1),
    ("cycle_decay_ms", 1),
]:
    meta = chain_params.get(key, {})
    if meta.get("type") != "int":
        fail(f"{key} must be int")
    if meta.get("step") != 1:
        fail(f"{key} step must be 1ms")
    if meta.get("min") != min_value:
        fail(f"{key} min must be {min_value}")

lfo_rate_meta = chain_params.get("lfo_rate", {})
if lfo_rate_meta.get("type") != "enum":
    fail("lfo_rate type must be enum for stable synced division editing")
random_rate_meta = chain_params.get("random_rate", {})
if random_rate_meta.get("type") != "enum":
    fail("random_rate type must be enum for stable synced division editing")

expected_rate_options = ["16 bars", "8 bars", "4 bars", "2 bars", "1 bar", "1/2", "1/4", "1/8", "1/16", "1/32", "1/64"]
if lfo_rate_meta.get("options") != expected_rate_options:
    fail(f"lfo_rate options must be {expected_rate_options}")
if random_rate_meta.get("options") != expected_rate_options:
    fail(f"random_rate options must be {expected_rate_options}")

poly_at_curve_meta = chain_params.get("poly_aftertouch_curve", {})
if poly_at_curve_meta.get("type") != "float":
    fail("poly_aftertouch_curve must be float")
if poly_at_curve_meta.get("min") != -1.0 or poly_at_curve_meta.get("max") != 1.0:
    fail("poly_aftertouch_curve must use bipolar range -1..1")
if "poly_aftertouch_smoothing" in chain_params:
    fail("poly_aftertouch_smoothing must be removed")

poly_at_level = levels.get("poly_aftertouch", {})
if poly_at_level.get("params") != ["poly_aftertouch_curve"]:
    fail("poly_aftertouch level must only expose poly_aftertouch_curve")

filter_mode_meta = chain_params.get("filter_mode", {})
if filter_mode_meta.get("type") != "enum":
    fail("filter_mode must be enum")
if set(filter_mode_meta.get("options", [])) != {"lp", "bp", "hp"}:
    fail("filter_mode enum must expose lp/bp/hp")

for filter_key in ["filter_cutoff", "filter_resonance"]:
    meta = chain_params.get(filter_key, {})
    if meta.get("type") != "float":
        fail(f"{filter_key} must be float")
    if meta.get("min") != 0.0 or meta.get("max") != 1.0:
        fail(f"{filter_key} range must be 0..1")

filter_level = levels.get("filter", {})
if filter_level.get("params") != ["filter_mode", "filter_cutoff", "filter_resonance"]:
    fail("filter level must expose mode, cutoff, resonance")

expected_submenu_knobs = {
    "main": ["model", "pitch", "harmonics", "timbre", "morph", "fm_amount", "lpg_decay", "lpg_color"],
    "filter": ["filter_mode", "filter_cutoff", "filter_resonance"],
    "assign1_mod": ["assign1_target", "assign1_mod_lfo_amt", "assign1_mod_env_amt", "assign1_mod_cycle_env_amt", "assign1_mod_random_amt", "assign1_mod_velocity_amt", "assign1_mod_poly_aftertouch_amt"],
    "assign2_mod": ["assign2_target", "assign2_mod_lfo_amt", "assign2_mod_env_amt", "assign2_mod_cycle_env_amt", "assign2_mod_random_amt", "assign2_mod_velocity_amt", "assign2_mod_poly_aftertouch_amt"],
    "pitch_mod": ["pitch_mod_lfo_amt", "pitch_mod_env_amt", "pitch_mod_cycle_env_amt", "pitch_mod_random_amt", "pitch_mod_velocity_amt", "pitch_mod_poly_aftertouch_amt"],
    "harmonics_mod": ["harmonics_mod_lfo_amt", "harmonics_mod_env_amt", "harmonics_mod_cycle_env_amt", "harmonics_mod_random_amt", "harmonics_mod_velocity_amt", "harmonics_mod_poly_aftertouch_amt"],
    "timbre_mod": ["timbre_mod_lfo_amt", "timbre_mod_env_amt", "timbre_mod_cycle_env_amt", "timbre_mod_random_amt", "timbre_mod_velocity_amt", "timbre_mod_poly_aftertouch_amt"],
    "cutoff_mod": ["cutoff_mod_lfo_amt", "cutoff_mod_env_amt", "cutoff_mod_cycle_env_amt", "cutoff_mod_random_amt", "cutoff_mod_velocity_amt", "cutoff_mod_poly_aftertouch_amt"],
    "lfo": ["lfo_shape", "lfo_rate", "lfo_sync", "lfo_retrig", "lfo_phase"],
    "envelope": ["env_attack_ms", "env_decay_ms", "env_sustain", "env_release_ms", "env_retrig"],
    "cycle_env": ["cycle_attack_ms", "cycle_decay_ms", "cycle_shape", "cycle_sync", "cycle_retrig", "cycle_bipolar"],
    "random": ["random_mode", "random_rate", "random_sync", "random_slew", "random_retrig"],
    "velocity": ["velocity_curve"],
    "poly_aftertouch": ["poly_aftertouch_curve"],
    "voice": ["voice_mode", "polyphony", "unison", "detune", "spread", "glide_ms"],
}

for level_name, expected_knobs in expected_submenu_knobs.items():
    level = levels.get(level_name, {})
    knobs = level.get("knobs")
    if knobs != expected_knobs:
        fail(f"{level_name} knobs must be {expected_knobs}, got {knobs}")

print("PASS: module metadata checks")
