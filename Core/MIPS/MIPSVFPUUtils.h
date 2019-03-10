// Copyright (c) 2012- PPSSPP Project.

// This program is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation, version 2.0 or later versions.

// This program is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License 2.0 for more details.

// A copy of the GPL 2.0 should have been included with the program.
// If not, see http://www.gnu.org/licenses/

// Official git repository and contact information can be found at
// https://github.com/hrydgard/ppsspp and http://www.ppsspp.org/.

#pragma once
#include <cmath>

#include "Common/CommonTypes.h"
#include "Core/MIPS/MIPS.h"

#define _VD (op & 0x7F)
#define _VS ((op>>8) & 0x7F)
#define _VT ((op>>16) & 0x7F)

inline int Xpose(int v) {
	return v^0x20;
}

#ifndef M_PI_2
#define M_PI_2     1.57079632679489661923
#endif

// Some games depend on exact values, but sinf() and cosf() aren't always precise.
// Stepping down to [0, 2pi) helps, but we also check common exact-result values.
// TODO: cos(2) and sin(1) should be -0.0, but doing that gives wrong results (possibly from floorf.)

inline float vfpu_sin(float angle) {
	angle -= floorf(angle * 0.25f) * 4.f;
	if (angle == 0.0f || angle == 2.0f) {
		return 0.0f;
	} else if (angle == 1.0f) {
		return 1.0f;
	} else if (angle == 3.0f) {
		return -1.0f;
	}
	angle *= (float)M_PI_2;
	return sinf(angle);
}

inline float vfpu_cos(float angle) {
	angle -= floorf(angle * 0.25f) * 4.f;
	if (angle == 1.0f || angle == 3.0f) {
		return 0.0f;
	} else if (angle == 0.0f) {
		return 1.0f;
	} else if (angle == 2.0f) {
		return -1.0f;
	}
	angle *= (float)M_PI_2;
	return cosf(angle);
}

inline float vfpu_asin(float angle) {
	return asinf(angle) / M_PI_2;
}

inline void vfpu_sincos(float angle, float &sine, float &cosine) {
	angle -= floorf(angle * 0.25f) * 4.f;
	if (angle == 0.0f) {
		sine = 0.0f;
		cosine = 1.0f;
	} else if (angle == 1.0f) {
		sine = 1.0f;
		cosine = 0.0f;
	} else if (angle == 2.0f) {
		sine = 0.0f;
		cosine = -1.0f;
	} else if (angle == 3.0f) {
		sine = -1.0f;
		cosine = 0.0f;
	} else {
		angle *= (float)M_PI_2;
#if defined(__linux__)
		sincosf(angle, &sine, &cosine);
#else
		sine = sinf(angle);
		cosine = cosf(angle);
#endif
	}
}

inline float vfpu_clamp(float v, float min, float max) {
	// Note: NAN is preserved, and -0.0 becomes +0.0 if min=+0.0.
	return v >= max ? max : (v <= min ? min : v);
}

#define VFPU_FLOAT16_EXP_MAX    0x1f
#define VFPU_SH_FLOAT16_SIGN    15
#define VFPU_MASK_FLOAT16_SIGN  0x1
#define VFPU_SH_FLOAT16_EXP     10
#define VFPU_MASK_FLOAT16_EXP   0x1f
#define VFPU_SH_FLOAT16_FRAC    0
#define VFPU_MASK_FLOAT16_FRAC  0x3ff

enum VectorSize {
	V_Single = 1,
	V_Pair = 2,
	V_Triple = 3,
	V_Quad = 4,
	V_Invalid = -1,
};

enum MatrixSize {
	M_2x2 = 2,
	M_3x3 = 3,
	M_4x4 = 4,
	M_Invalid = -1
};

static u32 VFPU_SWIZZLE(int x, int y, int z, int w) {
	return (x << 0) | (y << 2) | (z << 4) | (w << 6);
}

static u32 VFPU_MASK(int x, int y, int z, int w) {
	return (x << 0) | (y << 1) | (z << 2) | (w << 3);
}

static u32 VFPU_ANY_SWIZZLE() {
	return 0x000000FF;
}

static u32 VFPU_ABS(int x, int y, int z, int w) {
	return VFPU_MASK(x, y, z, w) << 8;
}

static u32 VFPU_CONST(int x, int y, int z, int w) {
	return VFPU_MASK(x, y, z, w) << 12;
}

static u32 VFPU_NEGATE(int x, int y, int z, int w) {
	return VFPU_MASK(x, y, z, w) << 16;
}

u32 VFPURewritePrefix(int ctrl, u32 remove, u32 add);

void ReadMatrix(float *rd, MatrixSize size, int reg);
void WriteMatrix(const float *rs, MatrixSize size, int reg);

void WriteVector(const float *rs, VectorSize N, int reg);
void ReadVector(float *rd, VectorSize N, int reg);

void GetVectorRegs(u8 regs[4], VectorSize N, int vectorReg);
void GetMatrixRegs(u8 regs[16], MatrixSize N, int matrixReg);
 
// Translate between vector and matrix size. Possibly we should simply
// join the two enums, but the type safety is kind of nice.
VectorSize GetVectorSize(MatrixSize sz);
MatrixSize GetMatrixSize(VectorSize sz);

// Note that if matrix is a transposed matrix (E format), GetColumn will actually return rows,
// and vice versa.
int GetColumnName(int matrix, MatrixSize msize, int column, int offset);
int GetRowName(int matrix, MatrixSize msize, int row, int offset);

int GetMatrixName(int matrix, MatrixSize msize, int column, int row, bool transposed);

void GetMatrixColumns(int matrixReg, MatrixSize msize, u8 vecs[4]);
void GetMatrixRows(int matrixReg, MatrixSize msize, u8 vecs[4]);

enum MatrixOverlapType {
	OVERLAP_NONE = 0,
	OVERLAP_PARTIAL = 1,
	OVERLAP_EQUAL = 2,
	// Transposed too?  (same space but transposed)
};

MatrixOverlapType GetMatrixOverlap(int m1, int m2, MatrixSize msize);


// Returns a number from 0-7, good for checking overlap for 4x4 matrices.
inline int GetMtx(int matrixReg) {
	return (matrixReg >> 2) & 7;
}

VectorSize GetVecSizeSafe(MIPSOpcode op);
VectorSize GetVecSize(MIPSOpcode op);
MatrixSize GetMtxSizeSafe(MIPSOpcode op);
MatrixSize GetMtxSize(MIPSOpcode op);
VectorSize GetHalfVectorSizeSafe(VectorSize sz);
VectorSize GetHalfVectorSize(VectorSize sz);
VectorSize GetDoubleVectorSizeSafe(VectorSize sz);
VectorSize GetDoubleVectorSize(VectorSize sz);
VectorSize MatrixVectorSizeSafe(MatrixSize sz);
VectorSize MatrixVectorSize(MatrixSize sz);
int GetNumVectorElements(VectorSize sz);
int GetMatrixSideSafe(MatrixSize sz);
int GetMatrixSide(MatrixSize sz);
const char *GetVectorNotation(int reg, VectorSize size);
const char *GetMatrixNotation(int reg, MatrixSize size);
inline bool IsMatrixTransposed(int matrixReg) {
	return (matrixReg >> 5) & 1;
}
inline bool IsVectorColumn(int vectorReg) {
	return !((vectorReg >> 5) & 1);
}
inline int TransposeMatrixReg(int matrixReg) {
	return matrixReg ^ 0x20;
}
int GetVectorOverlap(int reg1, VectorSize size1, int reg2, VectorSize size2);

float Float16ToFloat32(unsigned short l);
