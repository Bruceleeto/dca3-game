#pragma once

extern void stacktrace();

struct CMatrixRef {
	CMatrix& ref;

	CMatrixRef(CMatrix &f): ref(f) { }
	// operator ->
	CMatrix* operator->() { return &ref; }

	CMatrix& r() { return ref; }

	operator const CMatrix&() { return ref; }

	CMatrixRef& operator=(const CMatrixRef &m) {
		ref = m.ref;
		return *this;
	}

	CMatrixRef& operator=(const CMatrix &m) {
		ref = m;
		return *this;
	}

	~CMatrixRef() {
		if (std::isnan(ref.px)) {
			fprintf(stderr, "ref.px is NaN\n");
			stacktrace();
		}
	}
};

class CPlaceable	
{
protected:
	CMatrix m_matrixPlaceable;

public:
	// disable allocation
	static void *operator new(size_t) throw();

	CPlaceable(void);
	virtual ~CPlaceable(void);
	const CVector &GetPosition(void) { return m_matrixPlaceable.GetPosition(); }
	void SetPosition(float x, float y, float z) {
		if (std::isnan(x)) {
			fprintf(stderr, "x is NaN\n");
			stacktrace();
		}

		m_matrixPlaceable.GetPosition().x = x;
		m_matrixPlaceable.GetPosition().y = y;
		m_matrixPlaceable.GetPosition().z = z;
	}
	void SetPosition(const CVector &pos) {
		if (std::isnan(pos.x)) {
			fprintf(stderr, "pos.x is NaN\n");
			stacktrace();
		}
		m_matrixPlaceable.GetPosition() = pos;
	}
	CVector &GetRight(void) { return m_matrixPlaceable.GetRight(); }
	CVector &GetForward(void) { return m_matrixPlaceable.GetForward(); }
	CVector &GetUp(void) { return m_matrixPlaceable.GetUp(); }
	CMatrixRef GetMatrix(void) { return CMatrixRef(m_matrixPlaceable); }
	void SetMatrix(const CMatrix &newMatrix) { 
		m_matrixPlaceable = newMatrix; 
		if (std::isnan(newMatrix.px)) {
			fprintf(stderr, "newMatrix.px is NaN\n");
			stacktrace();
		}
	}
	void SetTransform(RwMatrix *m) { 
		m_matrixPlaceable = CMatrix(m, false); 
		if (std::isnan(m_matrixPlaceable.px)) {
			fprintf(stderr, "newMatrix.px is NaN\n");
			stacktrace();
		}
	}
	void SetHeading(float angle);
	void SetOrientation(float x, float y, float z){
		CVector pos = m_matrixPlaceable.GetPosition();
		m_matrixPlaceable.SetRotate(x, y, z);
		m_matrixPlaceable.Translate(pos);
	}
	bool IsWithinArea(float x1, float y1, float x2, float y2);
	bool IsWithinArea(float x1, float y1, float z1, float x2, float y2, float z2);
};

VALIDATE_SIZE(CPlaceable, 0x4C);
