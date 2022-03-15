#include "lib.h"

#define ASSERT(cond)                                                                               \
    if (!(cond))                                                                                   \
    jd_panic()

struct _queue {
    uint16_t front;
    uint16_t back;
    uint16_t size;
    uint16_t curr_size;
    uint8_t data[0];
};

#define FRM_SIZE(f) ((JD_FRAME_SIZE(f) + 3) & ~3)

int jd_queue_will_fit(jd_queue_t q, unsigned size) {
    int ret;
    target_disable_irq();
    if (q->front <= q->back) {
        ret = (q->back + size <= q->size || q->front > size);
    } else {
        ret = q->back + size < q->front;
    }
    target_enable_irq();
    return ret;
}

int jd_queue_push(jd_queue_t q, jd_frame_t *pkt) {
    if (pkt->size == 0)
        jd_panic();

    target_disable_irq();

    int ret = 0;

    unsigned size = FRM_SIZE(pkt);
    if (q->front <= q->back) {
        if (q->back + size <= q->size)
            q->back += size;
        else if (q->front > size) {
            q->curr_size = q->back;
            q->back = size;
        } else {
            ret = -1;
        }
    } else {
        if (q->back + size < q->front)
            q->back += size;
        else
            ret = -2;
    }
    if (ret == 0)
        memcpy(q->data + q->back - size, pkt, size);

    ASSERT(q->front <= q->size);
    ASSERT(q->back <= q->size);
    ASSERT(q->curr_size <= q->size);

    target_enable_irq();

    return ret;
}

jd_frame_t *jd_queue_front(jd_queue_t q) {
    if (q->front != q->back) {
        if (q->front >= q->curr_size)
            return (jd_frame_t *)(q->data);
        else
            return (jd_frame_t *)(q->data + q->front);
    } else
        return NULL;
}

void jd_queue_shift(jd_queue_t q) {
    jd_frame_t *pkt = jd_queue_front(q);
    ASSERT(pkt != NULL);
    unsigned size = FRM_SIZE(pkt);
    if (q->front >= q->curr_size) {
        ASSERT(q->front == q->curr_size);
        q->front = size;
        q->curr_size = q->size;
    } else {
        q->front += size;
    }
}

jd_queue_t jd_queue_alloc(unsigned size) {
    jd_queue_t q = jd_alloc(sizeof(*q) + size);
    q->size = q->curr_size = size;
    q->front = q->back = 0;
    return q;
}

#define TEST_SIZE 512
void jd_queue_test() {
    jd_queue_t q = jd_queue_alloc(TEST_SIZE);
    int push = 0;
    int shift = 0;
    int len = 0;
    static jd_frame_t frm;

    for (int i = 0; i < 20000; ++i) {
        if ((jd_random() & 3) == 1) {
            frm.crc = push;
            int sz = (jd_random() & 0xff) + 1;
            if (sz > 240)
                sz = 12;
            frm.size = sz;
            DMESG("push %d %d", push, frm.size);
            if (jd_queue_push(q, &frm) == 0) {
                push++;
                len += FRM_SIZE(&frm);
            } else {
                DMESG("full");
                ASSERT(len + JD_FRAME_SIZE(&frm) > TEST_SIZE - 255);
            }
        } else {
            jd_frame_t *f = jd_queue_front(q);
            if (shift == push) {
                ASSERT(f == NULL);
            } else {
                ASSERT(f != NULL);
                DMESG("pop %d", f->crc);
                ASSERT(f->crc == shift);
                shift++;
                for (int i = 0; i < f->size; ++i)
                    ASSERT(f->data[i] == 0);
                jd_queue_shift(q);
            }
        }
    }

    DMESG("q-test OK");
}
