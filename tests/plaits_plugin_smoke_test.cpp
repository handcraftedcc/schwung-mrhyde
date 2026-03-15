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

int main() {
    plugin_api_v2_t *api = move_plugin_init_v2(NULL);
    if (!api) fail("move_plugin_init_v2 returned null");
    if (!api->create_instance || !api->on_midi || !api->set_param || !api->get_param || !api->render_block) {
        fail("plugin api missing required function pointers");
    }

    void *inst = api->create_instance("src", "{}");
    if (!inst) fail("create_instance failed");

    api->set_param(inst, "model", "fm_2op");
    api->set_param(inst, "harmonics", "0.6");
    api->set_param(inst, "timbre", "0.4");
    api->set_param(inst, "morph", "0.7");
    api->set_param(inst, "fm_amount", "0.2");
    api->set_param(inst, "voice_mode", "poly");
    api->set_param(inst, "lfo_shape", "saw");
    api->set_param(inst, "filter_mode", "bp");
    api->set_param(inst, "filter_cutoff", "0.37");
    api->set_param(inst, "filter_resonance", "0.61");
    api->set_param(inst, "glide_ms", "127");
    api->set_param(inst, "pitch_mod_lfo_amt", "12");
    api->set_param(inst, "cutoff_mod_lfo_amt", "0.5");
    api->set_param(inst, "assign1_target", "morph");
    api->set_param(inst, "assign1_mod_env_amt", "0.2");
    api->set_param(inst, "env_attack_ms", "123.7");
    api->set_param(inst, "cycle_attack_ms", "0");

    char model_buf[32];
    memset(model_buf, 0, sizeof(model_buf));
    if (api->get_param(inst, "model", model_buf, (int)sizeof(model_buf)) < 0) {
        fail("get_param(model) failed");
    }
    if (strcmp(model_buf, "fm_2op") != 0) {
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

    char assign_target_buf[32];
    memset(assign_target_buf, 0, sizeof(assign_target_buf));
    if (api->get_param(inst, "assign1_target", assign_target_buf, (int)sizeof(assign_target_buf)) < 0) {
        fail("get_param(assign1_target) failed");
    }
    if (strcmp(assign_target_buf, "morph") != 0) {
        fail("assign1_target should return enum text");
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

    api->set_param(inst, "lfo_sync", "on");
    api->set_param(inst, "lfo_rate", "0");
    char lfo_rate_buf[32];
    memset(lfo_rate_buf, 0, sizeof(lfo_rate_buf));
    if (api->get_param(inst, "lfo_rate", lfo_rate_buf, (int)sizeof(lfo_rate_buf)) < 0) {
        fail("get_param(lfo_rate) failed");
    }
    if (strcmp(lfo_rate_buf, "16 bars") != 0) {
        fail("lfo_rate should show synced division label when sync is on");
    }

    api->set_param(inst, "lfo_rate", "1/64");
    memset(lfo_rate_buf, 0, sizeof(lfo_rate_buf));
    if (api->get_param(inst, "lfo_rate", lfo_rate_buf, (int)sizeof(lfo_rate_buf)) < 0) {
        fail("get_param(lfo_rate) failed after enum text set");
    }
    if (strcmp(lfo_rate_buf, "1/64") != 0) {
        fail("lfo_rate should accept and return synced fraction labels");
    }

    char hierarchy_buf[8192];
    memset(hierarchy_buf, 0, sizeof(hierarchy_buf));
    if (api->get_param(inst, "ui_hierarchy", hierarchy_buf, (int)sizeof(hierarchy_buf)) <= 0) {
        fail("ui_hierarchy get_param returned empty");
    }
    if (hierarchy_buf[0] != '{') {
        fail("ui_hierarchy should be a JSON object");
    }
    if (!has_json_label(hierarchy_buf, "Assign 1*")) {
        fail("ui_hierarchy should mark active assign modulation page with star");
    }
    if (!has_json_label(hierarchy_buf, "Cutoff*")) {
        fail("ui_hierarchy should mark active cutoff modulation page with star");
    }
    if (has_json_label(hierarchy_buf, "Assign 2*")) {
        fail("ui_hierarchy should not mark inactive assign2 page");
    }

    char chain_params_buf[16384];
    memset(chain_params_buf, 0, sizeof(chain_params_buf));
    if (api->get_param(inst, "chain_params", chain_params_buf, (int)sizeof(chain_params_buf)) <= 0) {
        fail("chain_params get_param returned empty");
    }
    if (chain_params_buf[0] != '[') {
        fail("chain_params should be a JSON array");
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

    api->destroy_instance(inst);
    printf("PASS: plaits plugin smoke test\n");
    return 0;
}
