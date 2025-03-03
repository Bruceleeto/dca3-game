#pragma once

struct alignas(8) CColPoint
{
	alignas(8) CVector point;
	float pad1 = 0.0f;
	// the surface normal on the surface of point
	alignas(8) CVector normal;
	float pad2 = 0.0f;
	uint8 surfaceA;
	uint8 pieceA;
	uint8 surfaceB;
	uint8 pieceB;
	float depth;

	const CVector &GetNormal() { return normal; }
	float GetDepth() { return depth; }
	void Set(float depth, uint8 surfA, uint8 pieceA, uint8 surfB, uint8 pieceB) {
		this->depth = depth;
		this->surfaceA = surfA;
		this->pieceA = pieceA;
		this->surfaceB = surfB;
		this->pieceB = pieceB;
	}
	void Set(uint8 surfA, uint8 pieceA, uint8 surfB, uint8 pieceB) {
		this->surfaceA = surfA;
		this->pieceA = pieceA;
		this->surfaceB = surfB;
		this->pieceB = pieceB;
	}

	CColPoint &operator=(const CColPoint &other);
};

