/*
 * filename :	sort.h
 *
 * programmer : Cao Jiayin
 */

#ifndef SORT__
#define SORT__

// include the header files
#include <iostream>
#include <cstring>
#include <string>
#include <stdlib.h>

// use default name space
using namespace std;

// some useful macro
#define SAFE_DELETE(p) { if(p) { delete p; p = 0; } }
#define SAFE_DELETE_ARRAY(p) { if(p) { delete[] p; p = 0 ; } }

// initialize SORT system
bool	initSort();

// release SORT system
void	releaseSort();

#endif
