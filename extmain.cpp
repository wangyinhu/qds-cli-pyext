#define PY_SSIZE_T_CLEAN
#include <Python.h>
#include "client.h"


static PyObject *QdsPyCliError;

struct QdsPyCli{
	PyObject_HEAD
    QdsClient *cli = nullptr;
};

using DID_Type = uint64_t;

static PyObject *QdsPyCli_new(PyTypeObject * type, PyObject * args, PyObject * kwds) {
	auto self = (QdsPyCli *) type->tp_alloc(type, 0);
	if (self == nullptr) {
		return nullptr;
	}

	static const char *kwlist[] = {"host", "port", "did", "cid", "devk", nullptr};
	int port;
	char *didc = nullptr;
	int cid;
	uint8_t *devk = nullptr;
    Py_ssize_t didlen;
    Py_ssize_t devklen;
    char *host = nullptr;
	if (PyArg_ParseTupleAndKeywords(args, kwds, "sis#is#;", (char **) kwlist, &host, &port, &didc, &didlen, &cid, &devk,
									&devklen)) {
	    if(didlen != 16){
            PyErr_SetString(QdsPyCliError,
                            "parameter error, did(str):len(did)=16\n");
            Py_TYPE(self)->tp_free((PyObject *) self);
            return nullptr;
	    }
	    if(devklen != 16){
            PyErr_SetString(QdsPyCliError,
                            "parameter error, did(str):len(did)=16\n");
            Py_TYPE(self)->tp_free((PyObject *) self);
            return nullptr;
	    }
        DID_Type did;
        hex2raw((uint8_t *) &did, didc, 16);
        self->cli = new QdsClient(did, cid, devk, port, host);
        return (PyObject *) self;
	} else {
		PyErr_SetString(QdsPyCliError,
						"parameter error, keys: host(str), port(int), did(str):len(did)=16, cid(int), devk(bstr):len(devk)=16\n");
        Py_TYPE(self)->tp_free((PyObject *) self);
        return nullptr;
	}
}

static void QdsCli_dealloc(QdsPyCli *self) {
    printf("QdsCli_dealloc called.\n");
	delete self->cli;
	Py_TYPE(self)->tp_free((PyObject *) self);
}

static PyObject *sendto_func(QdsPyCli *self, PyObject *args, PyObject *kwds) {
	static const char *kwlist[] = {"did", "cid", "data", nullptr};
	char *didc = nullptr;
	int cid;
	uint8_t *data = nullptr;
    Py_ssize_t datalen;
	if (PyArg_ParseTupleAndKeywords(args, kwds, "sis#;", (char **) kwlist, &didc, &cid, &data, &datalen)) {
		DID_Type did;
		hex2raw((uint8_t *) &did, didc, 16);
		if (0 == self->cli->SendTheData(did, cid, data, datalen)) {
            auto rtn = self->cli->run(nullptr);
			return PyLong_FromLong(rtn);
		} else {
			PyErr_SetString(QdsPyCliError, "fail to send data\n");
			return nullptr;
		}

	} else {
        PyErr_SetString(QdsPyCliError,
                        "parameter error, keys: did(str), cid(int), data(bstr)\n");
        return nullptr;
	}
}

static PyObject *pack_process_func_obj = nullptr;

static int pack_process_func(uint64_t _did, uint16_t _cid, uint8_t *data, size_t datalen){
    Py_ssize_t dl = datalen;
    char hexdid[20];
    raw2hex(hexdid, (uint8_t*)&_did, sizeof(_did));
    auto packdict = Py_BuildValue("({s:s,s:i,s:y#})", "did", hexdid, "cid", _cid, "data", data, dl);
    auto result = PyObject_CallObject(pack_process_func_obj, packdict);
    Py_XDECREF(packdict);
    if (result == nullptr){
//        PyErr_SetString(QdsPyCliError,"parameter error, callback function: function ({s:s,s:i,s:y#})\n");
        return PY_EXT_EXCEPTION; /* Pass error back */
    }
    Py_XDECREF(result);
    return 0;
}

static PyObject *run_func(QdsPyCli *self, PyObject *args, PyObject *kwds) {
	static const char *kwlist[] = {"pack_process_func", nullptr};
	if (PyArg_ParseTupleAndKeywords(args, kwds, "O;", (char **) kwlist, &pack_process_func_obj)) {
        Py_XINCREF(pack_process_func_obj);
        auto rtn = self->cli->run(pack_process_func);
        Py_XDECREF(pack_process_func_obj);
        if (rtn){
            if(rtn == PY_EXT_EXCEPTION){
                return nullptr;
            } else {
                char s[100];
                sprintf(s, "client run return code=%d\n", rtn);
                PyErr_SetString(QdsPyCliError, s);
                return nullptr;
            }
        }
        return PyLong_FromLong(rtn);
	} else {
        PyErr_SetString(QdsPyCliError,"parameter error, keys: function ({s:s,s:i,s:y#})\n");
        return nullptr;
	}
}

static PyMethodDef QdsCli_PyMethods[] = {
		{"sendto",     (PyCFunction) sendto_func,      METH_VARARGS |
												   METH_KEYWORDS, "send data to did, cid"},
		{"run",     (PyCFunction) run_func,      METH_VARARGS |
												   METH_KEYWORDS, "get notice packs"},
		{nullptr,    nullptr, 0,                                nullptr}  /* Sentinel */
};

static PyTypeObject QdsCli_PyClass = {
		PyVarObject_HEAD_INIT(nullptr, 0)
		"Qds.client",             /* tp_name */
		sizeof(QdsPyCli),             /* tp_basicsize */
		0,                         /* tp_itemsize */
		(destructor) QdsCli_dealloc, /* tp_dealloc */
		0,                              /* tp_print // tp_vectorcall_offset */
		nullptr,                         /* tp_getattr */
		nullptr,                         /* tp_setattr */
		nullptr,                         /* tp_reserved */
		nullptr,                         /* tp_repr */
		nullptr,                         /* tp_as_number */
		nullptr,                         /* tp_as_sequence */
		nullptr,                         /* tp_as_mapping */
		nullptr,                         /* tp_hash  */
		nullptr,                         /* tp_call */
		nullptr,                         /* tp_str */
		nullptr,                         /* tp_getattro */
		nullptr,                         /* tp_setattro */
		nullptr,                         /* tp_as_buffer */
		Py_TPFLAGS_DEFAULT |
		Py_TPFLAGS_BASETYPE,   			 /* tp_flags */
		"Qds protocol device client",  /* tp_doc */
		nullptr,                         /* tp_traverse */
		nullptr,                         /* tp_clear */
		nullptr,                         /* tp_richcompare */
		0,                         		 /* tp_weaklistoffset */
		nullptr,                         /* tp_iter */
		nullptr,                         /* tp_iternext */
		QdsCli_PyMethods,              /* tp_methods */
		nullptr,                         /* tp_members */
		nullptr,                         /* tp_getset */
		nullptr,                         /* tp_base */
		nullptr,                         /* tp_dict */
		nullptr,                         /* tp_descr_get */
		nullptr,                         /* tp_descr_set */
		0,                         /* tp_dictoffset */
		nullptr,                           /* tp_init */
		nullptr,                         /* tp_alloc */
		QdsPyCli_new,                 /* tp_new */
		nullptr,                            //freefunc tp_free; /* Low-level free-memory routine */
		nullptr,                            //inquiry tp_is_gc; /* For PyObject_IS_GC */
		nullptr,                            //PyObject *tp_bases;
		nullptr,                            //PyObject *tp_mro; /* method resolution order */
		nullptr,                            //PyObject *tp_cache;
		nullptr,                            //PyObject *tp_subclasses;
		nullptr,                            //PyObject *tp_weaklist;
		nullptr,                            //destructor tp_del;
		0,                            //unsigned int tp_version_tag;
		nullptr,                            //destructor tp_finalize;
        nullptr,                            //tp_vectorcall;
};

static PyModuleDef QdsModule = {
		PyModuleDef_HEAD_INIT,
		"Qds",
		"Qds protocol client.",
		-1,
		nullptr, nullptr, nullptr, nullptr, nullptr
};

PyMODINIT_FUNC PyInit_QdsPyCli(void) {
	PyObject * MainModule = PyModule_Create(&QdsModule);
	if (MainModule == nullptr)
		return nullptr;

	if (PyType_Ready(&QdsCli_PyClass) < 0) return nullptr;

	Py_INCREF(&QdsCli_PyClass);

	QdsPyCliError = PyErr_NewException("Qds.error", nullptr, nullptr);
	Py_INCREF(QdsPyCliError);

	PyModule_AddObject(MainModule, "client", (PyObject *) &QdsCli_PyClass);
	PyModule_AddObject(MainModule, "error", QdsPyCliError);
	return MainModule;
}
