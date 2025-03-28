#pragma once

#include "SurfaceTable.h"

struct alignas(8) CSphere
{
	// NB: this has to be compatible with a CVuVector
	CVector center;
	float radius;
	void Set(float radius, const CVector &center) { this->center = center; this->radius = radius; }
};

struct CColSphere : public CSphere
{
	uint8 surface;
	uint8 piece;

	void Set(float radius, uint8 surf = SURFACE_DEFAULT, uint8 piece = 0) {
		this->radius = radius;
		this->surface = surf;
		this->piece = piece;
	}
	void Set(float radius, const CVector &center, uint8 surf, uint8 piece);
	bool IntersectRay(CVector const &from, CVector const &dir, CVector &entry, CVector &exit);
	using CSphere::Set;
};