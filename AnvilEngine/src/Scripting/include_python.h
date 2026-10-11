// Fix for windows debug not being able to link to python debug libraries
#pragma once
#ifdef _DEBUG
#undef _DEBUG
#include <Python.h>
#define _DEBUG
#else
#include <Python.h>
#endif

