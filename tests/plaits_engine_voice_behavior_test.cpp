#include <cstdio>
#include <cstdlib>

#include "plaits_move_engine.h"

static void fail(const char *msg) {
    std::fprintf(stderr, "FAIL: %s\n", msg);
    std::exit(1);
}

static void render_some(ppf_engine_t &engine, int frames, int block) {
    float l[PPF_MAX_RENDER];
    float r[PPF_MAX_RENDER];
    int remaining = frames;
    while (remaining > 0) {
        int n = remaining > block ? block : remaining;
        engine.render(l, r, n);
        remaining -= n;
    }
}

static float render_peak(ppf_engine_t &engine, int frames, int block) {
    float l[PPF_MAX_RENDER];
    float r[PPF_MAX_RENDER];
    float peak = 0.0f;
    int remaining = frames;
    while (remaining > 0) {
        int n = remaining > block ? block : remaining;
        engine.render(l, r, n);
        for (int i = 0; i < n; ++i) {
            float al = l[i] < 0.0f ? -l[i] : l[i];
            float ar = r[i] < 0.0f ? -r[i] : r[i];
            if (al > peak) peak = al;
            if (ar > peak) peak = ar;
        }
        remaining -= n;
    }
    return peak;
}

int main() {
    ppf_engine_t engine;
    ppf_params_t params;
    ppf_default_params(&params);
    params.voice_mode = 1;  // poly
    params.polyphony = 4;
    params.unison = 1;
    params.env_attack_ms = 0;
    params.env_decay_ms = 0;
    params.env_sustain = 0.0f;
    params.env_release_ms = 5;
    engine.set_params(params);

    engine.note_on(60, 1.0f);
    render_some(engine, 256, 64);
    if (engine.debug_active_voice_count() != 1) {
        fail("first note_on should allocate one voice");
    }
    if (engine.debug_active_note_count(60) != 1) {
        fail("first note_on should have one active C4 voice");
    }

    engine.note_on(60, 0.8f);
    render_some(engine, 256, 64);
    if (engine.debug_active_voice_count() != 1) {
        fail("retriggering same note in poly should steal/reuse, not duplicate");
    }
    if (engine.debug_active_note_count(60) != 1) {
        fail("same note should still have a single active voice");
    }

    ppf_engine_t mono_engine;
    ppf_params_t mono_params;
    ppf_default_params(&mono_params);
    mono_params.voice_mode = 0;  // mono
    mono_params.unison = 3;
    mono_engine.set_params(mono_params);
    mono_engine.note_on(60, 1.0f);
    render_some(mono_engine, 256, 64);
    if (mono_engine.debug_active_voice_count() != 3) {
        fail("mono with unison should allocate a full unison stack");
    }
    if (mono_engine.debug_active_note_count(60) != 3) {
        fail("mono unison should stack voices on the played note");
    }
    mono_engine.note_on(64, 1.0f);
    render_some(mono_engine, 256, 64);
    if (mono_engine.debug_active_voice_count() != 3) {
        fail("mono should keep unison stack size when changing notes");
    }
    if (mono_engine.debug_active_note_count(60) != 0) {
        fail("mono should not allow chords when changing notes");
    }
    if (mono_engine.debug_active_note_count(64) != 3) {
        fail("mono note changes should retrigger the full unison stack");
    }

    ppf_engine_t legato_engine;
    ppf_params_t legato_params;
    ppf_default_params(&legato_params);
    legato_params.voice_mode = 2;  // mono-legato
    legato_params.unison = 2;
    legato_engine.set_params(legato_params);
    legato_engine.note_on(67, 1.0f);
    render_some(legato_engine, 256, 64);
    if (legato_engine.debug_active_voice_count() != 2) {
        fail("mono-legato with unison should allocate all unison voices");
    }
    if (legato_engine.debug_active_note_count(67) != 2) {
        fail("mono-legato unison should stack voices on the played note");
    }
    legato_engine.note_on(71, 1.0f);
    render_some(legato_engine, 256, 64);
    if (legato_engine.debug_active_voice_count() != 2) {
        fail("mono-legato should keep unison stack size when changing notes");
    }
    if (legato_engine.debug_active_note_count(67) != 0) {
        fail("mono-legato should not allow chords");
    }
    if (legato_engine.debug_active_note_count(71) != 2) {
        fail("mono-legato note changes should move the full unison stack");
    }

    engine.note_off(60);
    render_some(engine, 32768, 128);
    if (engine.debug_active_voice_count() != 0) {
        fail("note_off should fully release voice");
    }

    ppf_engine_t held_engine;
    ppf_params_t held_params;
    ppf_default_params(&held_params);
    held_params.voice_mode = 1;  // poly
    held_params.polyphony = 4;
    held_params.unison = 1;
    held_params.lpg_decay = 1.0f;
    held_engine.set_params(held_params);
    held_engine.note_on(64, 1.0f);
    render_some(held_engine, PPF_SAMPLE_RATE * 2, 128);
    float held_peak_before_release = render_peak(held_engine, 2048, 128);
    if (held_peak_before_release < 1e-5f) {
        fail("held note should remain audible before note_off");
    }
    held_engine.note_off(64);
    float early_release_peak = render_peak(held_engine, 2048, 128);
    if (early_release_peak < 1e-5f) {
        fail("held note release should keep audible tail until release completes");
    }
    render_some(held_engine, 128, 128);
    if (held_engine.debug_active_voice_count() == 0) {
        fail("held note release should not hard-cut immediately");
    }
    render_some(held_engine, PPF_SAMPLE_RATE * 2, 128);
    if (held_engine.debug_active_voice_count() == 0) {
        fail("max LPG decay should keep release active beyond 2 seconds");
    }
    render_some(held_engine, PPF_SAMPLE_RATE * 12, 128);
    if (held_engine.debug_active_voice_count() != 0) {
        fail("held note release should eventually finish");
    }

    ppf_engine_t release_timing_engine;
    ppf_params_t release_timing_params;
    ppf_default_params(&release_timing_params);
    release_timing_params.voice_mode = 1;  // poly
    release_timing_params.polyphony = 1;
    release_timing_params.unison = 1;
    release_timing_params.lpg_decay = 0.0f;
    release_timing_params.env_release_ms = 5000;
    release_timing_engine.set_params(release_timing_params);
    release_timing_engine.note_on(60, 1.0f);
    render_some(release_timing_engine, 512, 64);
    release_timing_engine.note_off(60);
    int release_samples = release_timing_engine.debug_release_samples_total_for_note(60);
    if (release_samples <= 0) {
        fail("note_off should initialize release timing for active note");
    }
    if (release_samples >= PPF_SAMPLE_RATE) {
        fail("release timing should follow LPG decay, not ADSR release time");
    }

    params.model = 0;
    engine.set_params(params);
    engine.note_on(48, 1.0f);
    render_some(engine, 512, 64);
    int engine_a = engine.debug_voice_active_engine(0);
    engine.note_off(48);
    render_some(engine, 4096, 128);

    params.model = 23;
    engine.set_params(params);
    engine.note_on(48, 1.0f);
    render_some(engine, 512, 64);
    int engine_b = engine.debug_voice_active_engine(0);

    if (engine_a == engine_b) {
        fail("model changes should select a different plaits engine");
    }

    ppf_engine_t mix_engine;
    ppf_params_t mix_params;
    ppf_default_params(&mix_params);
    mix_params.voice_mode = 1;  // poly
    mix_params.polyphony = 8;
    mix_params.unison = 1;
    mix_engine.set_params(mix_params);

    for (int n = 0; n < 8; ++n) {
        mix_engine.note_on(48 + n, 1.0f);
    }
    float chord_peak = render_peak(mix_engine, 4096, 128);
    if (chord_peak > 1.0f) {
        fail("poly voice mix peak should stay in [-1, 1]");
    }

    std::printf("PASS: plaits engine voice behavior\n");
    return 0;
}
