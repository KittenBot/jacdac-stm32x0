#include "jdsimple.h"

struct srv_state {
    SENSOR_COMMON;
    uint8_t state, inited;
    int32_t sample, position;
    uint32_t nextSample;
};

const static int8_t posMap[] = {0, +1, -1, +2, -1, 0, -2, +1, +1, -2, 0, -1, +2, -1, +1, 0};
static void update(srv_t *state) {
    // based on comments in https://github.com/PaulStoffregen/Encoder/blob/master/Encoder.h
    uint16_t s = state->state & 3;
    if (pin_get(PIN_P0))
        s |= 4;
    if (pin_get(PIN_P1))
        s |= 8;

    state->state = (s >> 2);
    if (posMap[s]) {
        state->position += posMap[s];
        state->sample = state->position >> 2;
    }
}

static void maybe_init(srv_t *state) {
    if (state->is_streaming && !state->inited) {
        state->inited = true;
        pin_setup_input(PIN_P0, 1);
        pin_setup_input(PIN_P1, 1);
        update(state);
    }
}

void crank_process(srv_t *state) {
    maybe_init(state);

    if (should_sample(&state->nextSample, 997) && state->inited)
        update(state);

    sensor_process_simple(state, &state->sample, sizeof(state->sample));
}

void crank_handle_packet(srv_t *state, jd_packet_t *pkt) {
    sensor_handle_packet_simple(state, pkt, &state->sample, sizeof(state->sample));
}

SRV_DEF(crank, JD_SERVICE_CLASS_ROTARY_ENCODER);

void crank_init() {
    SRV_ALLOC(crank);
}
