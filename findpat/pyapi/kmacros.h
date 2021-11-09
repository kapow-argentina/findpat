#ifndef __MACROS_H__
#define __MACROS_H__

#ifdef _DEBUG
	#define debug(...) fprintf(stderr, __VA_ARGS__)
#else
	#define debug(...)
#endif

/* Time */

#define TIME_VARS tiempo_usec _t_ini, _t_fin
#define TIME_START getTickTime(&_t_ini)
#define TIME_END getTickTime(&_t_fin)
#define TIME_DIFF getTimeDiff(_t_ini, _t_fin)

/* Python stufs-hacks */
#define _PyMethod(module, name, params) {#name, (PyCFunction)module##_##name, params, name##__doc__}

/* Function Parameter Helpers */
#define paramNoneToNull(obj) { if ((obj) && (PyObject*)(obj) == Py_None) (obj) = NULL; }
#define paramCheckNullOr(type, obj) { if ((obj) && !type##_Check(obj)) { PyErr_SetString(PyExc_TypeError, "If set, "#obj" must be a "#type"."); return NULL; } }

#define paramCheckArrayFldsz_err(obj, sz, error) { if (obj && PyKArray_GET_SIZE(obj) && PyKArray_GET_ITEMSZ(obj) != (sz)) { \
		PyErr_Format(PyExc_TypeError, "src must be a kapow.Array of elements of size %u.", (unsigned int)(sz)); return error; \
	} }

#define paramCheckArrayFldsz(obj, sz) paramCheckArrayFldsz_err(obj, sz, NULL)
#define paramCheckArrayFldsz_int(obj, sz) paramCheckArrayFldsz_err(obj, sz, -1)

#define paramCheckArraySize_err(obj, n, sz, error) { if (obj && PyKArray_GET_SIZE(obj) != n) { \
	if (PyKArray_GET_SIZE(obj) == 0) { \
		if (PyKArray_resize((PyKArray*)(obj), n, sz) < 0) return error; \
	} else { PyErr_Format(PyExc_TypeError, "If set, "#obj" must be a kapow.Array of size %lu or empty, but has %lu.", (unsigned long)(n), PyKArray_GET_SIZE(obj)); return error; } \
} else { paramCheckArrayFldsz_err(obj, sz, error); }}
#define paramCheckArraySize(obj, n, sz) paramCheckArraySize_err(obj, n, sz, NULL)
#define paramCheckArraySize_int(obj, n, sz) paramCheckArraySize_err(obj, n, sz, -1)

#define paramCheckCharOrNull(obj, ch) { \
	if (obj && PyString_Check(obj)) { \
		if (PyString_GET_SIZE(obj) != 1) { PyErr_SetString(PyExc_TypeError, #obj" must be a single char."); return NULL; } \
		ch = *PyString_AS_STRING(obj); \
	} else if (obj && PyInt_Check(obj)) { \
		ch = PyInt_AS_LONG(obj); \
	} else if (obj) { \
		PyErr_SetString(PyExc_TypeError, #obj" must be a single char or an integer."); \
		return NULL; \
	} \
}


#define paramCheckKArrayStr_err(ksrc, src, n, error) { \
	if (ksrc && PyString_Check(ksrc)) { \
		src = (uchar*)PyString_AS_STRING(ksrc); \
		n = PyString_GET_SIZE(ksrc); \
	} else if (ksrc && PyKArray_Check(ksrc)) { \
		if (PyKArray_GET_ITEMSZ(ksrc) != 1) { \
			PyErr_SetString(PyExc_TypeError, "src must be a kapow.Array of chars."); \
			return error; \
		} \
		src = PyKArray_GET_BUF(ksrc); \
		n = PyKArray_GET_SIZE(ksrc); \
	} else { \
		PyErr_SetString(PyExc_TypeError, "src must be a string or kapow.Array of chars."); \
		return error; \
	} \
}
#define paramCheckKArrayStr(ksrc, src, n) paramCheckKArrayStr_err(ksrc, src, n, NULL)
#define paramCheckKArrayStr_int(ksrc, src, n) paramCheckKArrayStr_err(ksrc, src, n, -1)


#define paramArrayExistsOrNew(obj, n, sz) { \
	if (!(obj)) { \
		(obj) = (PyKArray*)PyKArray_new(); \
		PyKArray_init((obj), n, sz); \
	} else { \
		Py_INCREF((obj)); \
}}

#endif
