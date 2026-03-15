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
    "lpg_decay",
    "lpg_color",
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
if "fm_2op" not in model_opts:
    fail("model enum options missing expected engine name 'fm_2op'")

dests = ["pitch", "harmonics", "timbre", "morph", "fm", "color"]
sources = ["lfo", "env", "cycle_env", "random", "velocity", "poly_aftertouch"]
for d in dests:
    for s in sources:
        key = f"{d}_mod_{s}_amt"
        if key not in chain_params:
            fail(f"missing modulation amount key: {key}")

levels = cap.get("ui_hierarchy", {}).get("levels", {})
root = levels.get("root", {})
entries = root.get("params", [])

if "timbre" not in entries:
    fail("timbre must be direct root parameter")
if "harmonics" not in entries:
    fail("harmonics must be direct root parameter")
if "morph" not in entries:
    fail("morph must be direct root parameter")
if "fm_amount" not in entries:
    fail("fm_amount must be direct root parameter")
if "pitch" not in entries:
    fail("pitch must be direct root parameter")
if "lpg_decay" not in entries or "lpg_color" not in entries:
    fail("lpg_decay and lpg_color must be direct root parameters")

labels = [e.get("label") for e in entries if isinstance(e, dict)]
for label in ["Pitch Mod", "Harmonics Mod", "Timbre Mod", "Morph Mod", "FM Mod", "Color Mod"]:
    if label not in labels:
        fail(f"missing root submenu: {label}")
if "LPG" in labels:
    fail("LPG should not be a submenu")

root_knobs = root.get("knobs", [])
expected_root_knobs = [
    "model",
    "harmonics",
    "timbre",
    "morph",
    "fm_amount",
    "lpg_decay",
    "lpg_color",
]
if root_knobs != expected_root_knobs:
    fail(f"root knobs must be {expected_root_knobs}, got {root_knobs}")

for forbidden in ["Macros", "Init / Randomize", "LPG Mod"]:
    if forbidden in labels:
        fail(f"forbidden root submenu present: {forbidden}")

level_names = set(levels.keys())
forbidden_levels = ["macros", "init_randomize", "lpg_mod"]
for forbidden_level in forbidden_levels:
    if forbidden_level in level_names:
        fail(f"forbidden level present: {forbidden_level}")

expected_mod_names = {
    "lfo": "LFO Amt",
    "env": "Env Amt",
    "cycle_env": "Cycle Env Amt",
    "random": "Random Amt",
    "velocity": "Velocity Amt",
    "poly_aftertouch": "Poly Aftertouch Amt",
}

for dest in dests:
    for src_key, expected_name in expected_mod_names.items():
        key = f"{dest}_mod_{src_key}_amt"
        meta = chain_params.get(key, {})
        name = meta.get("name")
        if name != expected_name:
            fail(f"{key} should have name '{expected_name}', got '{name}'")
        if dest == "pitch":
            if meta.get("min") != -48.0 or meta.get("max") != 48.0:
                fail(f"{key} pitch modulation range should be -48..48")
            if meta.get("step") != 1.0:
                fail(f"{key} pitch modulation step should be 1.0")
        if dest == "color":
            if meta.get("min") != -1.0 or meta.get("max") != 1.0:
                fail(f"{key} color modulation range should be -1..1")

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
if lfo_rate_meta.get("type") != "float":
    fail("lfo_rate type must stay float for continuous unsynced use")
random_rate_meta = chain_params.get("random_rate", {})
if random_rate_meta.get("type") != "float":
    fail("random_rate type must stay float for continuous unsynced use")

def level_param_keys(level_name: str):
    level = levels.get(level_name, {})
    out = []
    for item in level.get("params", []):
        if isinstance(item, str):
            out.append(item)
    return out

for level_name in [
    "pitch_mod",
    "harmonics_mod",
    "timbre_mod",
    "morph_mod",
    "fm_mod",
    "color_mod",
    "lfo",
    "envelope",
    "cycle_env",
    "random",
    "velocity",
    "poly_aftertouch",
    "voice",
]:
    expected = level_param_keys(level_name)
    knobs = levels.get(level_name, {}).get("knobs", [])
    if knobs != expected:
        fail(f"{level_name} knobs should follow param order")

print("PASS: module metadata checks")
