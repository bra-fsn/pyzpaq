#define PY_SSIZE_T_CLEAN

#include <Python.h>
#include "zpaq/libzpaq.h"
#include <queue>
#include <iostream>
#include <stdexcept>

void libzpaq::error(const char* msg)
{
    std::cout << msg << std::endl;
    throw std::runtime_error(msg);
}

class In: public libzpaq::Reader
{
public:
    In(const uint8_t* data, size_t size) : _data(data), _size(size), _offset(0) {}
    int get()
    {
        if(_offset >= _size)
            return -1;
        return (int)_data[_offset++];
    }
private:
    const uint8_t* _data;
    size_t _size;
    size_t _offset;
};

class Out: public libzpaq::Writer
{
public:
    void put(int c)
    {
        _data.push_back(c);
    }
    std::vector<uint8_t> data() const { return _data; }
private:
    std::vector<uint8_t>_data;
};

static PyObject* compress(PyObject *self, PyObject *args, PyObject *kwargs)
{
    Py_buffer input_buffer;
    int compression_level = 5;  // Default compression level
    static char *kwlist[] = {"input", "level", NULL};

    // Parse the input arguments and keyword arguments: a byte string and an optional integer
    if (!PyArg_ParseTupleAndKeywords(args, kwargs, "s*|i", kwlist, &input_buffer, &compression_level))
        return NULL;

    // Check if compression level is within valid range
    if (compression_level < 1 || compression_level > 5) {
        PyErr_SetString(PyExc_ValueError, "Compression level must be between 1 and 5");
        return NULL;
    }

    In in((const uint8_t*)input_buffer.buf, input_buffer.len);
    Out out;

    // Convert the integer compression level to a string
    std::string level_str = std::to_string(compression_level);

    // Compress using the specified compression level
    libzpaq::compress(&in, &out, level_str.c_str());

    // Return the compressed data as a Python bytes object
    return PyBytes_FromStringAndSize((const char*)out.data().data(), out.data().size());
}

static PyObject* decompress(PyObject *self, PyObject *args)
{
    Py_buffer input_buffer;
    if(!PyArg_ParseTuple(args, "s*", &input_buffer))
        return NULL;
    In in((const uint8_t*)input_buffer.buf, input_buffer.len);
    Out out;
    libzpaq::decompress(&in, &out);
    return PyBytes_FromStringAndSize((const char*)out.data().data(), out.data().size());
}

static PyMethodDef zpaq_methods[] = {
    {"compress", (PyCFunction)compress, METH_VARARGS | METH_KEYWORDS, "Compress data with an optional compression level"},
    {"decompress", decompress, METH_VARARGS, ""},
    {NULL, NULL, 0, NULL},
};

static struct PyModuleDef zpaq_definition = {
    PyModuleDef_HEAD_INIT,
    "",
    "",
    -1,
    zpaq_methods,
};

PyMODINIT_FUNC PyInit_zpaq(void) {
  Py_Initialize();
  PyObject *m = PyModule_Create(&zpaq_definition);

  return m;
}