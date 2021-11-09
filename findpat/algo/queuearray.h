#ifndef __QUEUE_ARRAY_H__
#define __QUEUE_ARRAY_H__

/* Usage:
 * tipo miqueue[MAX];
 * uint_32 miqueue_b = 0;
 * uint_32 miqueue_e = 0;
 */

#define queue_declare(q, MAX) q[MAX]; unsigned int q##_b, q##_e
#define queue_init(q) { q##_b = q##_e = 0; }
#define queue_clean(q) queue_init(q)
#define queue_empty(q) (q##_b == q##_e)
#define queue_full(q, MAX) ((q##_b + (MAX) - q##_e)%(MAX) == 1)
#define queue_size(q, MAX) ((q##_e + (MAX) - q##_b)%(MAX))
#define queue_push(q, MAX, x) { if (!queue_full(q, MAX)) { q[q##_e++] = (x); q##_e %= (MAX); } }
#define queue_pop(q, MAX) { if (!queue_empty(q)) { q##_b = (q##_b + 1) % (MAX); } }
#define queue_front(q) (q[q##_b])

#endif
