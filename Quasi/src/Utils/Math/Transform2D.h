#pragma once
#include "Complex.h"

namespace Quasi::Math {
    struct Rotor2D : private fComplex {
        Rotor2D() : fComplex(1, 0) {}
        Rotor2D(Radians theta) : fComplex(Math::Cos(theta), Math::Sin(theta)) {}
    private:
        Rotor2D(const fComplex& c) : fComplex(c) {}
    public:
        static Rotor2D FromComplex(const fComplex& c) { return { c }; }
        static Rotor2D FromUnitVector(const fv2& c) { return fComplex { c.x, c.y }; }
        fComplex&       AsComplex()       { return static_cast<fComplex&>(*this); }
        const fComplex& AsComplex() const { return static_cast<const fComplex&>(*this); }

        fv2 AsUnitVector() const { return { re, im }; }
        fv2 IHat() const { return {  re, im }; }
        fv2 JHat() const { return { -im, re }; }
        f32 Cos() const { return re; }
        f32 Sin() const { return im; }

        Radians Angle() const;
        Radians AngleBetween(const Rotor2D& other) const;

        Rotor2D RotateCCW90() const;
        Rotor2D RotateCW90()  const;
        Rotor2D Rotate180()   const;
        Rotor2D RotateBy(Radians theta) const;
        Rotor2D RotateBy   (const Rotor2D& r) const;
        Rotor2D RotateByInv(const Rotor2D& r) const;

        Rotor2D Halved() const;
        Rotor2D HalvedCCW() const;
        Rotor2D Mul(f32 p) const;
        Rotor2D Doubled()  const { return Squared(); }
        Rotor2D Tripled()  const { return Cubed(); }
        Rotor2D Inverse()  const { return Conj(); }

        Matrix2x2 AsMatrixLinear() const { return AsComplex().AsMatrixLinear(); }
        Matrix2D  AsMatrix()       const { return AsComplex().AsMatrix(); }
        fv2 Rotate   (const fv2& v) const;
        fv2 InvRotate(const fv2& v) const;

        Rotor2D Lerp(const Rotor2D& z, f32 t) const;

        Rotor2D operator+() const { return *this; }
        Rotor2D operator-() const { return Inverse(); }
        Rotor2D operator+(const Rotor2D& r) const { return RotateBy(r); }
        Rotor2D operator-(const Rotor2D& r) const { return RotateByInv(r); }
        Rotor2D operator*(f32 mul) const { return Mul(mul); }
        Rotor2D operator/(f32 div) const { return Mul(1.0f / div); }
        Rotor2D& operator+=(const Rotor2D& r) { return *this = RotateBy(r); }
        Rotor2D& operator-=(const Rotor2D& r) { return *this = RotateByInv(r); }
        Rotor2D& operator*=(f32 mul) { return *this = Mul(mul); }
        Rotor2D& operator/=(f32 div) { return *this = Mul(1.0f / div); }

        static Rotor2D Random(RandomGenerator& rg);
    };

    template <Numeric T>
    Vector<T, 2> Vector<T, 2>::FromPolar(T r, const Rotor2D& theta) requires Floating<T> {
        const fComplex& t = theta.AsComplex();
        return { t.re * r, t.im * r };
    }

    template <Numeric T>
    Vector<T, 3> Vector<T, 3>::FromSpheric(T r, const Rotor2D& yaw, const Rotor2D& pitch) requires Floating<T> {
        const fComplex& y = yaw.AsComplex(), &p = pitch.AsComplex();
        const float xz = r * p.re;
        return { xz * y.re, r * p.im, xz * y.im };
    }

    struct MatrixTransform2D {
        Matrix2D transform;
        Matrix2x2 normalMatrix;

        fv2 Transform(const fv2& p) const { return (fv2)(transform * p); }
        fv2 TransformNormal(const fv2& n) const { return (normalMatrix * n).Norm(); }
    };

    template <class TF> concept ITransformation2D = requires (const TF& t, const fv2& vec) {
        { t.Transform(vec) }       -> ConvTo<fv2>;
        { t.TransformNormal(vec) } -> ConvTo<fv2>;
    };

    template <class T>
    struct InverseTransform2D {
        static_assert(ITransformation2D<T>, "T should be a transformer2d"); // delayed constraint
        const T& tformer;
        fv2 Transform(const fv2& point)               const { return tformer.TransformInverse(point); }
        fv2 TransformInverse(const fv2& point)        const { return tformer.Transform(point); }
        fv2 TransformNormal (const fv2& normal)       const { return tformer.TransformInverseNormal(normal); }
        fv2 TransformInverseNormal(const fv2& normal) const { return tformer.TransformNormal(normal); }
    };

    struct Transform3D;

    struct Transform2D {
        fv2 position = 0, scale = 1;
        Rotor2D rotation;

        Transform2D() = default;
        Transform2D(const fv2& pos, const fv2& scale = 1, const Rotor2D& rotation = {})
            : position(pos), scale(scale), rotation(rotation) {}

        static Transform2D Translate(const fv2& p);
        static Transform2D Scale(const fv2& s);
        static Transform2D Rotation(const Rotor2D& r);
        static Transform2D RotateAround(const Rotor2D& r, const fv2& center);

        Transform2D NormalTransform() const;

        fv2 Transform             (const fv2& point) const;
        fv2 TransformInverse      (const fv2& point) const;
        fv2 TransformNormal       (const fv2& normal) const;
        fv2 TransformInverseNormal(const fv2& normal) const;

        InverseTransform2D<Transform2D> Inverse() const { return { *this }; }

        void ApplyMatrix(const Matrix2D& matrix);

        Transform2D Then(const Transform2D& t) const { return t * (*this); }

        Transform3D As3D() const;

        void Reset();

        Matrix2x2 LinearMatrix() const;
        Matrix2D  TransformMatrix() const;

        fv2 operator*(const fv2& p) const { return Transform(p); }
        Transform2D operator*(const Transform2D& t) const;
        Transform2D& operator*=(const Transform2D& t);
    };

    template <Numeric T> Vector<T, 2> Vector<T, 2>::RotateBy(const Rotor2D& r) const {
        return r.Rotate(*this);
    }
    template <Numeric T> Vector<T, 2> Vector<T, 2>::TransformBy(const Transform2D& t) const {
        return t.Transform(*this);
    }
} // Quasi
