#ifndef __STACK_ARRAY_H__
#define __STACK_ARRAY_H__

/* Usage:
 * tipo mistack[MAX];
 * uint_32 mistack_p = 0;
 */

#define stack_declare(st, MAX) st[MAX]; unsigned int st##_p;
#define stack_init(st) { st##_p = 0; }
#define stack_clean(st) stack_init(st)
#define stack_empty(st) (st##_p == 0)
#define stack_full(st, MAX) (st##_p == (MAX))
#define stack_size(st) (st##_p)
#define stack_push(st, MAX, x) { if (!stack_full(st, MAX)) st[st##_p++] = (x); }
#define stack_pop(st) { if (!stack_empty(st)) st##_p--; }
#define stack_front(st) (st[st##_p-1])

#endif
