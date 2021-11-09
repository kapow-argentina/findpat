#ifndef __KARRAY_H__
#define __KARRAY_H__

/* Tipos */
typedef struct str_PyKArray {
	PyObject_HEAD
	unsigned int fldsz;
	unsigned long n;
	void* buf;
	PyObject* bind_to;
	unsigned int binded_from;
} PyKArray;

/** Invariantes:
 *
 * buf es NULL sii n*fldsz == 0.
 * fldsz es siempre 0, 1, 2, 4 u 8.
 * fldsz == 0 sólo si n == 0.
 * bind_to es NULL o dueño de una referencia.
 *
 **/

void kapow_Array_init(PyObject* m);

PyAPI_DATA(PyTypeObject) PyKArray_Type;

/* Funcs */
PyAPI_FUNC(PyObject *) PyKArray_new(void);
PyAPI_FUNC(int) PyKArray_init(PyKArray *self, unsigned long n, unsigned int fldsz);
PyAPI_FUNC(int) PyKArray_resize(PyKArray *self, unsigned long n, unsigned int fldsz);
PyAPI_FUNC(int) PyKArray_inverse(PyKArray *self, PyKArray *other);
PyAPI_FUNC(int) PyKArray_bindto(PyKArray *self, PyObject *obj, void* buf, unsigned long n, unsigned int fldsz); /* Consumes the reference to obj */
PyAPI_FUNC(int) PyKArray_load(PyKArray *self, FILE* fp);
PyAPI_FUNC(int) PyKArray_store(PyKArray *self, FILE* fp);
PyAPI_FUNC(PyKArray *) PyKArray_bind(PyKArray *self, unsigned long st, unsigned long n, unsigned int fldsz);
PyAPI_FUNC(unsigned PY_LONG_LONG) PyKArray_max(PyKArray *self);
PyAPI_FUNC(unsigned PY_LONG_LONG) PyKArray_min(PyKArray *self);

/* Macros */

#define PyKArray_GET_SIZE(karr) (((PyKArray*)(karr))->n)
#define PyKArray_GET_BUF(karr) (((PyKArray*)(karr))->buf)
#define PyKArray_GET_ITEMSZ(karr) (((PyKArray*)(karr))->fldsz)
#define PyKArray_GET_BUFSZ(karr) (PyKArray_GET_SIZE(karr) * (unsigned long)PyKArray_GET_ITEMSZ(karr))

#define PyKArray_GET_ITEM_1(karr, i) (((unsigned char*)PyKArray_GET_BUF(karr))[i])
#define PyKArray_GET_ITEM_2(karr, i) (((unsigned short*)PyKArray_GET_BUF(karr))[i])
#define PyKArray_GET_ITEM_4(karr, i) (((unsigned int*)PyKArray_GET_BUF(karr))[i])
#define PyKArray_GET_ITEM_8(karr, i) (((unsigned PY_LONG_LONG*)PyKArray_GET_BUF(karr))[i])

#define PyKArray_GET_ITEM(karr, i) (\
	PyKArray_GET_ITEMSZ(karr)==1?PyKArray_GET_ITEM_1(karr, i): \
	PyKArray_GET_ITEMSZ(karr)==2?PyKArray_GET_ITEM_2(karr, i): \
	PyKArray_GET_ITEMSZ(karr)==4?PyKArray_GET_ITEM_4(karr, i): \
	PyKArray_GET_ITEMSZ(karr)==8?PyKArray_GET_ITEM_8(karr, i): \
	0)

#define PyKArray_SET_ITEM(karr, i, vl) (\
	PyKArray_GET_ITEMSZ(karr)==1?PyKArray_GET_ITEM_1(karr, i) = (vl): \
	PyKArray_GET_ITEMSZ(karr)==2?PyKArray_GET_ITEM_2(karr, i) = (vl): \
	PyKArray_GET_ITEMSZ(karr)==4?PyKArray_GET_ITEM_4(karr, i) = (vl): \
	PyKArray_GET_ITEMSZ(karr)==8?PyKArray_GET_ITEM_8(karr, i) = (vl): \
	0)

#define PyKArray_Check(op) (Py_TYPE(op) == &PyKArray_Type)

#endif
