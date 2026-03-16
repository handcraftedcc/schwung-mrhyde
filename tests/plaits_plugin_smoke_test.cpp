#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

extern "C" {
typedef struct host_api_v1 {
    uint32_t api_version;
    int sample_rate;
    int frames_per_block;
    uint8_t *mapped_memory;
    int audio_out_offset;
    int audio_in_offset;
    void (*log)(const char *msg);
    int (*midi_send_internal)(const uint8_t *msg, int len);
    int (*midi_send_external)(const uint8_t *msg, int len);
} host_api_v1_t;

typedef struct plugin_api_v2 {
    uint32_t api_version;
    void *(*create_instance)(const char *module_dir, const char *json_defaults);
    void (*destroy_instance)(void *instance);
    void (*on_midi)(void *instance, const uint8_t *msg, int len, int source);
    void (*set_param)(void *instance, const char *key, const char *val);
    int (*get_param)(void *instance, const char *key, char *buf, int buf_len);
    int (*get_error)(void *instance, char *buf, int buf_len);
    void (*render_block)(void *instance, int16_t *out_interleaved_lr, int frames);
} plugin_api_v2_t;

plugin_api_v2_t *move_plugin_init_v2(const host_api_v1_t *host);
}

static void fail(const char *msg) {
    fprintf(stderr, "FAIL: %s\n", msg);
    exit(1);
}

static int has_nonzero_audio(const int16_t *audio, int n) {
    for (int i = 0; i < n; ++i) {
        if (audio[i] != 0) return 1;
    }
    return 0;
}

static int has_json_label(const char *json, const char *label_text) {
    if (!json || !label_text) return 0;
    char compact[128];
    char spaced[128];
    snprintf(compact, sizeof(compact), "\"label\":\"%s\"", label_text);
    snprintf(spaced, sizeof(spaced), "\"label\": \"%s\"", label_text);
    return strstr(json, compact) != NULL || strstr(json, spaced) != NULL;
}

static int chain_params_has_key_type(const char *json, const char *key, const char *type) {
    if (!json || !key || !type) return 0;
    char key_compact[128];
    char key_spaced[128];
    char type_compact[64];
    char type_spaced[64];
    snprintf(key_compact, sizeof(key_compact), "\"key\":\"%s\"", key);
    snprintf(key_spaced, sizeof(key_spaced), "\"key\": \"%s\"", key);
    snprintf(type_compact, sizeof(type_compact), "\"type\":\"%s\"", type);
    snprintf(type_spaced, sizeof(type_spaced), "\"type\": \"%s\"", type);

    const char *k = strstr(json, key_compact);
    if (!k) k = strstr(json, key_spaced);
    if (!k) return 0;

    const char *end = strchr(k, '}');
    if (!end) return 0;
    const char *t = strstr(k, type_compact);
    if (!t) t = strstr(k, type_spaced);
    return t && t < end;
}

int main() {
    plugin_api_v2_t *api = move_plugin_init_v2(NULL);
    if (!api) fail("move_plugin_init_v2 returned null");
    if (!api->create_instance || !api->on_midi || !api->set_param || !api->get_param || !api->render_block) {
        fail("plugin api missing required function pointers");
    }

    void *inst = api->create_instance("src", "{}");
    if (!inst) fail("create_instance failed");

    api->set_param(inst, "model", "FM 2-Op");
    api->set_param(inst, "harmonics", "0.6");
    api->set_param(inst, "timbre", "0.4");
    api->set_param(inst, "morph", "0.7");
    api->set_param(inst, "fm_amount", "0.2");
    api->set_param(inst, "aux_mix", "0.5");
    api->set_param(inst, "volume", "0.82");
    api->set_param(inst, "pan", "-0.31");
    api->set_param(inst, "voice_mode", "poly");
    api->set_param(inst, "lfo_shape", "saw");
    api->set_param(inst, "filter_mode", "bp");
    api->set_param(inst, "filter_cutoff", "0.37");
    api->set_param(inst, "filter_resonance", "0.61");
    api->set_param(inst, "glide_ms", "127");
    api->set_param(inst, "pitch_mod_lfo_amt", "12");
    api->set_param(inst, "harmonics_mod_env_amt", "0.2");
    api->set_param(inst, "timbre_mod_velocity_amt", "0.3");
    api->set_param(inst, "cutoff_mod_lfo_amt", "0.5");
    api->set_param(inst, "assign1_target", "morph");
    api->set_param(inst, "assign2_target", "pan");
    api->set_param(inst, "assign1_mod_env_amt", "0.2");
    api->set_param(inst, "env_attack_ms", "123.7");
    api->set_param(inst, "cycle_attack_ms", "0");

    char model_buf[32];
    memset(model_buf, 0, sizeof(model_buf));
    if (api->get_param(inst, "model", model_buf, (int)sizeof(model_buf)) < 0) {
        fail("get_param(model) failed");
    }
    if (strcmp(model_buf, "FM 2-Op") != 0) {
        fail("model value mismatch after set/get");
    }

    char enum_buf[64];
    memset(enum_buf, 0, sizeof(enum_buf));
    if (api->get_param(inst, "voice_mode", enum_buf, (int)sizeof(enum_buf)) < 0) {
        fail("get_param(voice_mode) failed");
    }
    if (strcmp(enum_buf, "poly") != 0) {
        fail("voice_mode should return enum text");
    }

    memset(enum_buf, 0, sizeof(enum_buf));
    if (api->get_param(inst, "lfo_shape", enum_buf, (int)sizeof(enum_buf)) < 0) {
        fail("get_param(lfo_shape) failed");
    }
    if (strcmp(enum_buf, "saw") != 0) {
        fail("lfo_shape should return enum text");
    }

    memset(enum_buf, 0, sizeof(enum_buf));
    if (api->get_param(inst, "filter_mode", enum_buf, (int)sizeof(enum_buf)) < 0) {
        fail("get_param(filter_mode) failed");
    }
    if (strcmp(enum_buf, "bp") != 0) {
        fail("filter_mode should return enum text");
    }

    char filter_cutoff_buf[32];
    memset(filter_cutoff_buf, 0, sizeof(filter_cutoff_buf));
    if (api->get_param(inst, "filter_cutoff", filter_cutoff_buf, (int)sizeof(filter_cutoff_buf)) < 0) {
        fail("get_param(filter_cutoff) failed");
    }
    if (strcmp(filter_cutoff_buf, "0.37") != 0) {
        fail("filter_cutoff should roundtrip as float");
    }

    char filter_res_buf[32];
    memset(filter_res_buf, 0, sizeof(filter_res_buf));
    if (api->get_param(inst, "filter_resonance", filter_res_buf, (int)sizeof(filter_res_buf)) < 0) {
        fail("get_param(filter_resonance) failed");
    }
    if (strcmp(filter_res_buf, "0.61") != 0) {
        fail("filter_resonance should roundtrip as float");
    }

    memset(enum_buf, 0, sizeof(enum_buf));
    if (api->get_param(inst, "lpg_retrig", enum_buf, (int)sizeof(enum_buf)) >= 0) {
        fail("lpg_retrig should not exist");
    }

    char glide_buf[32];
    memset(glide_buf, 0, sizeof(glide_buf));
    if (api->get_param(inst, "glide_ms", glide_buf, (int)sizeof(glide_buf)) < 0) {
        fail("get_param(glide_ms) failed");
    }
    if (strcmp(glide_buf, "125") != 0) {
        fail("glide_ms should quantize to 5ms integer steps");
    }

    char pitch_mod_buf[32];
    memset(pitch_mod_buf, 0, sizeof(pitch_mod_buf));
    if (api->get_param(inst, "pitch_mod_lfo_amt", pitch_mod_buf, (int)sizeof(pitch_mod_buf)) < 0) {
        fail("get_param(pitch_mod_lfo_amt) failed");
    }
    if (strcmp(pitch_mod_buf, "12") != 0) {
        fail("pitch_mod_lfo_amt should support wider pitch range values");
    }

    char cutoff_mod_buf[32];
    memset(cutoff_mod_buf, 0, sizeof(cutoff_mod_buf));
    if (api->get_param(inst, "cutoff_mod_lfo_amt", cutoff_mod_buf, (int)sizeof(cutoff_mod_buf)) < 0) {
        fail("get_param(cutoff_mod_lfo_amt) failed");
    }
    if (strcmp(cutoff_mod_buf, "0.5") != 0) {
        fail("cutoff_mod_lfo_amt should roundtrip as float amount");
    }

    char aux_mix_buf[32];
    memset(aux_mix_buf, 0, sizeof(aux_mix_buf));
    if (api->get_param(inst, "aux_mix", aux_mix_buf, (int)sizeof(aux_mix_buf)) < 0) {
        fail("get_param(aux_mix) failed");
    }
    if (strcmp(aux_mix_buf, "0.5") != 0) {
        fail("aux_mix should roundtrip as float amount");
    }

    char volume_buf[32];
    memset(volume_buf, 0, sizeof(volume_buf));
    if (api->get_param(inst, "volume", volume_buf, (int)sizeof(volume_buf)) < 0) {
        fail("get_param(volume) failed");
    }
    if (strcmp(volume_buf, "0.82") != 0) {
        fail("volume should roundtrip as float amount");
    }

    char pan_buf[32];
    memset(pan_buf, 0, sizeof(pan_buf));
    if (api->get_param(inst, "pan", pan_buf, (int)sizeof(pan_buf)) < 0) {
        fail("get_param(pan) failed");
    }
    if (strcmp(pan_buf, "-0.31") != 0) {
        fail("pan should roundtrip as float amount");
    }

    char assign_target_buf[32];
    char assign2_target_buf[32];

    api->set_param(inst, "assign1_target", "detune");
    memset(assign_target_buf, 0, sizeof(assign_target_buf));
    if (api->get_param(inst, "assign1_target", assign_target_buf, (int)sizeof(assign_target_buf)) < 0) {
        fail("get_param(assign1_target) failed for detune");
    }
    if (strcmp(assign_target_buf, "detune") != 0) {
        fail("assign1_target should support detune target");
    }
    api->set_param(inst, "assign1_target", "morph");

    api->set_param(inst, "assign2_target", "spread");
    memset(assign2_target_buf, 0, sizeof(assign2_target_buf));
    if (api->get_param(inst, "assign2_target", assign2_target_buf, (int)sizeof(assign2_target_buf)) < 0) {
        fail("get_param(assign2_target) failed for spread");
    }
    if (strcmp(assign2_target_buf, "spread") != 0) {
        fail("assign2_target should support spread target");
    }
    api->set_param(inst, "assign2_target", "pan");

    memset(assign_target_buf, 0, sizeof(assign_target_buf));
    if (api->get_param(inst, "assign1_target", assign_target_buf, (int)sizeof(assign_target_buf)) < 0) {
        fail("get_param(assign1_target) failed");
    }
    if (strcmp(assign_target_buf, "morph") != 0) {
        fail("assign1_target should return enum text");
    }

    memset(assign2_target_buf, 0, sizeof(assign2_target_buf));
    if (api->get_param(inst, "assign2_target", assign2_target_buf, (int)sizeof(assign2_target_buf)) < 0) {
        fail("get_param(assign2_target) failed");
    }
    if (strcmp(assign2_target_buf, "pan") != 0) {
        fail("assign2_target should support pan target");
    }

    char attack_buf[32];
    memset(attack_buf, 0, sizeof(attack_buf));
    if (api->get_param(inst, "env_attack_ms", attack_buf, (int)sizeof(attack_buf)) < 0) {
        fail("get_param(env_attack_ms) failed");
    }
    if (strcmp(attack_buf, "124") != 0) {
        fail("env_attack_ms should be integer milliseconds");
    }

    char cycle_attack_buf[32];
    memset(cycle_attack_buf, 0, sizeof(cycle_attack_buf));
    if (api->get_param(inst, "cycle_attack_ms", cycle_attack_buf, (int)sizeof(cycle_attack_buf)) < 0) {
        fail("get_param(cycle_attack_ms) failed");
    }
    if (strcmp(cycle_attack_buf, "1") != 0) {
        fail("cycle_attack_ms should clamp to integer minimum of 1ms");
    }

    api->set_param(inst, "lfo_sync", "free");
    api->set_param(inst, "lfo_rate", "3.5");
    char lfo_rate_buf[32];
    memset(lfo_rate_buf, 0, sizeof(lfo_rate_buf));
    if (api->get_param(inst, "lfo_rate", lfo_rate_buf, (int)sizeof(lfo_rate_buf)) < 0) {
        fail("get_param(lfo_rate) failed");
    }
    if (strcmp(lfo_rate_buf, "3.5") != 0) {
        fail("lfo_rate should return free numeric Hz value when sync is off");
    }

    api->set_param(inst, "lfo_sync", "sync");
    char err_buf[256];
    memset(err_buf, 0, sizeof(err_buf));
    if (api->get_error(inst, err_buf, (int)sizeof(err_buf)) < 0) {
        fail("get_error failed after lfo_sync=sync");
    }
    if (err_buf[0] != '\0') {
        fail("lfo_sync should accept paramlfo-style sync text");
    }

    api->set_param(inst, "lfo_sync", "sync");
    api->set_param(inst, "lfo_rate_sync", "1/8");
    char lfo_rate_sync_buf[32];
    memset(lfo_rate_sync_buf, 0, sizeof(lfo_rate_sync_buf));
    if (api->get_param(inst, "lfo_rate_sync", lfo_rate_sync_buf, (int)sizeof(lfo_rate_sync_buf)) < 0) {
        fail("get_param(lfo_rate_sync) failed");
    }
    if (strcmp(lfo_rate_sync_buf, "1/8") != 0) {
        fail("lfo_rate_sync should return synced division label when sync is on");
    }

    api->set_param(inst, "lfo_rate_sync", "1/64");
    memset(lfo_rate_sync_buf, 0, sizeof(lfo_rate_sync_buf));
    if (api->get_param(inst, "lfo_rate_sync", lfo_rate_sync_buf, (int)sizeof(lfo_rate_sync_buf)) < 0) {
        fail("get_param(lfo_rate_sync) failed after enum text set");
    }
    if (strcmp(lfo_rate_sync_buf, "1/64") != 0) {
        fail("lfo_rate_sync should accept and return synced fraction labels");
    }

    api->set_param(inst, "lfo_sync", "off");
    api->set_param(inst, "lfo_rate", "1/4");
    memset(err_buf, 0, sizeof(err_buf));
    if (api->get_error(inst, err_buf, (int)sizeof(err_buf)) < 0) {
        fail("get_error failed after lfo_rate enum write while sync off");
    }
    if (err_buf[0] != '\0') {
        fail("lfo_rate should accept synced label input even when sync is off");
    }
    memset(lfo_rate_buf, 0, sizeof(lfo_rate_buf));
    if (api->get_param(inst, "lfo_rate", lfo_rate_buf, (int)sizeof(lfo_rate_buf)) < 0) {
        fail("get_param(lfo_rate) failed after sync off stale enum write");
    }
    if (strchr(lfo_rate_buf, '/') != NULL) {
        fail("lfo_rate should report numeric Hz in free mode");
    }

    api->set_param(inst, "lfo_sync", "free");
    memset(err_buf, 0, sizeof(err_buf));
    if (api->get_error(inst, err_buf, (int)sizeof(err_buf)) < 0) {
        fail("get_error failed after lfo_sync=free");
    }
    if (err_buf[0] != '\0') {
        fail("lfo_sync should accept paramlfo-style free text");
    }

    api->set_param(inst, "random_sync", "free");
    api->set_param(inst, "random_rate", "5.25");
    char random_rate_buf[32];
    memset(random_rate_buf, 0, sizeof(random_rate_buf));
    if (api->get_param(inst, "random_rate", random_rate_buf, (int)sizeof(random_rate_buf)) < 0) {
        fail("get_param(random_rate) failed");
    }
    if (strcmp(random_rate_buf, "5.25") != 0) {
        fail("random_rate should return free numeric Hz value when sync is off");
    }

    api->set_param(inst, "random_sync", "sync");
    memset(err_buf, 0, sizeof(err_buf));
    if (api->get_error(inst, err_buf, (int)sizeof(err_buf)) < 0) {
        fail("get_error failed after random_sync=sync");
    }
    if (err_buf[0] != '\0') {
        fail("random_sync should accept paramlfo-style sync text");
    }

    api->set_param(inst, "random_sync", "sync");
    api->set_param(inst, "random_rate_sync", "1/8");
    char random_rate_sync_buf[32];
    memset(random_rate_sync_buf, 0, sizeof(random_rate_sync_buf));
    if (api->get_param(inst, "random_rate_sync", random_rate_sync_buf, (int)sizeof(random_rate_sync_buf)) < 0) {
        fail("get_param(random_rate_sync) failed");
    }
    if (strcmp(random_rate_sync_buf, "1/8") != 0) {
        fail("random_rate_sync should return synced division label when sync is on");
    }

    api->set_param(inst, "random_rate_sync", "1/64");
    memset(random_rate_sync_buf, 0, sizeof(random_rate_sync_buf));
    if (api->get_param(inst, "random_rate_sync", random_rate_sync_buf, (int)sizeof(random_rate_sync_buf)) < 0) {
        fail("get_param(random_rate_sync) failed after enum text set");
    }
    if (strcmp(random_rate_sync_buf, "1/64") != 0) {
        fail("random_rate_sync should accept and return synced fraction labels");
    }

    api->set_param(inst, "random_sync", "off");
    api->set_param(inst, "random_rate", "1/4");
    memset(err_buf, 0, sizeof(err_buf));
    if (api->get_error(inst, err_buf, (int)sizeof(err_buf)) < 0) {
        fail("get_error failed after random_rate enum write while sync off");
    }
    if (err_buf[0] != '\0') {
        fail("random_rate should accept synced label input even when sync is off");
    }
    memset(random_rate_buf, 0, sizeof(random_rate_buf));
    if (api->get_param(inst, "random_rate", random_rate_buf, (int)sizeof(random_rate_buf)) < 0) {
        fail("get_param(random_rate) failed after sync off stale enum write");
    }
    if (strchr(random_rate_buf, '/') != NULL) {
        fail("random_rate should report numeric Hz in free mode");
    }

    api->set_param(inst, "random_sync", "free");
    memset(err_buf, 0, sizeof(err_buf));
    if (api->get_error(inst, err_buf, (int)sizeof(err_buf)) < 0) {
        fail("get_error failed after random_sync=free");
    }
    if (err_buf[0] != '\0') {
        fail("random_sync should accept paramlfo-style free text");
    }

    api->set_param(inst, "lfo_sync", "sync");
    api->set_param(inst, "random_sync", "sync");
    char hierarchy_buf[32768];
    memset(hierarchy_buf, 0, sizeof(hierarchy_buf));
    if (api->get_param(inst, "ui_hierarchy", hierarchy_buf, (int)sizeof(hierarchy_buf)) <= 0) {
        fail("ui_hierarchy get_param returned empty");
    }
    if (hierarchy_buf[0] != '{') {
        fail("ui_hierarchy should be a JSON object");
    }
    if (strstr(hierarchy_buf, "\"lfo_rate\"") == NULL) {
        fail("ui_hierarchy should expose lfo_rate key");
    }
    if (strstr(hierarchy_buf, "\"lfo_rate_sync\"") == NULL) {
        fail("ui_hierarchy should expose lfo_rate_sync key");
    }
    if (strstr(hierarchy_buf, "\"random_rate\"") == NULL) {
        fail("ui_hierarchy should expose random_rate key");
    }
    if (strstr(hierarchy_buf, "\"random_rate_sync\"") == NULL) {
        fail("ui_hierarchy should expose random_rate_sync key");
    }
    if (!has_json_label(hierarchy_buf, "Assign 1*")) {
        fail("ui_hierarchy should mark active assign modulation page with star");
    }
    if (!has_json_label(hierarchy_buf, "Cutoff*")) {
        fail("ui_hierarchy should mark active cutoff modulation page with star");
    }
    if (!has_json_label(hierarchy_buf, "Pitch*")) {
        fail("ui_hierarchy should mark active pitch modulation page with star");
    }
    if (!has_json_label(hierarchy_buf, "Harmonics*")) {
        fail("ui_hierarchy should mark active harmonics modulation page with star");
    }
    if (!has_json_label(hierarchy_buf, "Timbre*")) {
        fail("ui_hierarchy should mark active timbre modulation page with star");
    }
    if (has_json_label(hierarchy_buf, "Assign 2*")) {
        fail("ui_hierarchy should not mark inactive assign2 page");
    }

    api->set_param(inst, "timbre_mod_velocity_amt", "0");
    memset(hierarchy_buf, 0, sizeof(hierarchy_buf));
    if (api->get_param(inst, "ui_hierarchy", hierarchy_buf, (int)sizeof(hierarchy_buf)) <= 0) {
        fail("ui_hierarchy get_param returned empty after timbre reset");
    }
    if (has_json_label(hierarchy_buf, "Timbre*")) {
        fail("ui_hierarchy should clear timbre modulation star when inactive");
    }

    char chain_params_buf[16384];
    memset(chain_params_buf, 0, sizeof(chain_params_buf));
    if (api->get_param(inst, "chain_params", chain_params_buf, (int)sizeof(chain_params_buf)) <= 0) {
        fail("chain_params get_param returned empty");
    }
    if (chain_params_buf[0] != '[') {
        fail("chain_params should be a JSON array");
    }
    if (!chain_params_has_key_type(chain_params_buf, "lfo_rate", "float")) {
        fail("chain_params should expose lfo_rate float parameter");
    }
    if (!chain_params_has_key_type(chain_params_buf, "lfo_rate_sync", "enum")) {
        fail("chain_params should expose lfo_rate_sync enum parameter");
    }
    if (!chain_params_has_key_type(chain_params_buf, "random_rate", "float")) {
        fail("chain_params should expose random_rate float parameter");
    }
    if (!chain_params_has_key_type(chain_params_buf, "random_rate_sync", "enum")) {
        fail("chain_params should expose random_rate_sync enum parameter");
    }

    char state_buf[16384];
    memset(state_buf, 0, sizeof(state_buf));
    if (api->get_param(inst, "state", state_buf, (int)sizeof(state_buf)) <= 0) {
        fail("state get_param returned empty");
    }
    if (state_buf[0] != '{') {
        fail("state should be a JSON object");
    }

    void *inst_from_state = api->create_instance("src", state_buf);
    if (!inst_from_state) fail("create_instance from state failed");

    char restored_model_buf[32];
    memset(restored_model_buf, 0, sizeof(restored_model_buf));
    if (api->get_param(inst_from_state, "model", restored_model_buf, (int)sizeof(restored_model_buf)) < 0) {
        fail("get_param(model) failed on restored instance");
    }
    if (strcmp(restored_model_buf, "FM 2-Op") != 0) {
        fail("state restore via json_defaults should restore model");
    }

    char restored_assign_buf[32];
    memset(restored_assign_buf, 0, sizeof(restored_assign_buf));
    if (api->get_param(inst_from_state, "assign1_target", restored_assign_buf, (int)sizeof(restored_assign_buf)) < 0) {
        fail("get_param(assign1_target) failed on restored instance");
    }
    if (strcmp(restored_assign_buf, "morph") != 0) {
        fail("state restore via json_defaults should restore assign target");
    }

    char restored_assign2_buf[32];
    memset(restored_assign2_buf, 0, sizeof(restored_assign2_buf));
    if (api->get_param(inst_from_state, "assign2_target", restored_assign2_buf, (int)sizeof(restored_assign2_buf)) < 0) {
        fail("get_param(assign2_target) failed on restored instance");
    }
    if (strcmp(restored_assign2_buf, "pan") != 0) {
        fail("state restore via json_defaults should restore assign2 target");
    }

    char restored_cutoff_mod_buf[32];
    memset(restored_cutoff_mod_buf, 0, sizeof(restored_cutoff_mod_buf));
    if (api->get_param(inst_from_state, "cutoff_mod_lfo_amt", restored_cutoff_mod_buf, (int)sizeof(restored_cutoff_mod_buf)) < 0) {
        fail("get_param(cutoff_mod_lfo_amt) failed on restored instance");
    }
    if (strcmp(restored_cutoff_mod_buf, "0.5") != 0) {
        fail("state restore via json_defaults should restore cutoff mod amount");
    }

    char restored_aux_mix_buf[32];
    memset(restored_aux_mix_buf, 0, sizeof(restored_aux_mix_buf));
    if (api->get_param(inst_from_state, "aux_mix", restored_aux_mix_buf, (int)sizeof(restored_aux_mix_buf)) < 0) {
        fail("get_param(aux_mix) failed on restored instance");
    }
    if (strcmp(restored_aux_mix_buf, "0.5") != 0) {
        fail("state restore via json_defaults should restore aux_mix");
    }

    char restored_volume_buf[32];
    memset(restored_volume_buf, 0, sizeof(restored_volume_buf));
    if (api->get_param(inst_from_state, "volume", restored_volume_buf, (int)sizeof(restored_volume_buf)) < 0) {
        fail("get_param(volume) failed on restored instance");
    }
    if (strcmp(restored_volume_buf, "0.82") != 0) {
        fail("state restore via json_defaults should restore volume");
    }

    char restored_pan_buf[32];
    memset(restored_pan_buf, 0, sizeof(restored_pan_buf));
    if (api->get_param(inst_from_state, "pan", restored_pan_buf, (int)sizeof(restored_pan_buf)) < 0) {
        fail("get_param(pan) failed on restored instance");
    }
    if (strcmp(restored_pan_buf, "-0.31") != 0) {
        fail("state restore via json_defaults should restore pan");
    }

    uint8_t note_on[] = {0x90, 60, 100};
    api->on_midi(inst, note_on, 3, 0);

    int16_t audio[128 * 2];
    memset(audio, 0, sizeof(audio));
    api->render_block(inst, audio, 128);
    api->render_block(inst, audio, 128);
    api->render_block(inst, audio, 128);

    if (!has_nonzero_audio(audio, 128 * 2)) {
        fail("rendered audio is silent after note on");
    }

    uint8_t note_off[] = {0x80, 60, 0};
    api->on_midi(inst, note_off, 3, 0);

    api->destroy_instance(inst_from_state);
    api->destroy_instance(inst);
    printf("PASS: plaits plugin smoke test\n");
    return 0;
}
