#pragma once

struct alignas(8) CColLine
{
	// NB: this has to be compatible with two CVuVectors
	alignas(8) CVector p0;
	float pad0 = 0.0f;
	alignas(8) CVector p1;
	float pad1 = 0.0f;

	CColLine(void) = default;
	CColLine(const CVector &p0, const CVector &p1) { this->p0 = p0; this->p1 = p1; };
	void Set(const CVector &p0, const CVector &p1);
};