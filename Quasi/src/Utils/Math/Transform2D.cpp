#include "Transform2D.h"

#include "Random.h"
#include "Transform3D.h"

namespace Quasi::Math {
    Radians Rotor2D::Angle() const { return Atan2(im, re); }
    Radians Rotor2D::AngleBetween(const Rotor2D& other) const { return Arccos(re * other.re + im * other.im); }

    Rotor2D Rotor2D::RotateCCW90() const { return fComplex { -im,  re }; }
    Rotor2D Rotor2D::RotateCW90()  const { return fComplex {  im, -re }; }
    Rotor2D Rotor2D::Rotate180()   const { return fComplex { -re, -im }; }
    Rotor2D Rotor2D::RotateBy(Radians theta) const { return RotateBy(Rotor2D { theta }); }
    Rotor2D Rotor2D::RotateBy   (const Rotor2D& r) const { return fComplex { re * r.re - im * r.im, re * r.im + im * r.re }; }
    Rotor2D Rotor2D::RotateByInv(const Rotor2D& r) const { return fComplex { re * r.re + im * r.im, im * r.re - re * r.im }; }

    Rotor2D Rotor2D::Halved() const { return UnitSqrt(); }
    Rotor2D Rotor2D::HalvedCCW() const { return UnitUpperSqrt(); }
    Rotor2D Rotor2D::Mul(f32 p) const { return ExpImag(*Angle() * p); }

    fv2 Rotor2D::Rotate   (const fv2& v) const { return { v.x * re - v.y * im, v.x * im + v.y * re }; }
    fv2 Rotor2D::InvRotate(const fv2& v) const { return { v.x * re + v.y * im, v.y * re - v.x * im }; }

    Rotor2D Rotor2D::Lerp(const Rotor2D& z, f32 t) const {
        const Radians theta = AngleBetween(z);
        const f32 inv = 1 / Math::Sin(theta), p = Math::Sin(theta * (1 - t)) * inv, q = Math::Sin(theta * t) * inv;
        return AsComplex() * p + z.AsComplex() * q;
    }

    Rotor2D Rotor2D::Random(RandomGenerator& rg) {
        return Radians { rg.Get<f32>(0, TAU) };
    }

    Transform2D Transform2D::Translate(const fv2& p)        { return { p }; }
    Transform2D Transform2D::Scale    (const fv2& s)        { return { 0, s }; }
    Transform2D Transform2D::Rotation (const Rotor2D& r) { return { 0, 1, r }; }
    Transform2D Transform2D::RotateAround(const Rotor2D& r, const fv2& center) {
        return { center - r.Rotate(center), 1, r };
    }

    Transform2D Transform2D::NormalTransform() const { return { 0, 1.0f / scale, rotation }; }

    fv2 Transform2D::Transform             (const fv2& point)  const { return rotation.Rotate(point * scale) + position; }
    fv2 Transform2D::TransformInverse      (const fv2& point)  const { return rotation.InvRotate(point - position) / scale;}
    fv2 Transform2D::TransformNormal       (const fv2& normal) const { return rotation.Rotate(normal / scale).Norm(); }
    fv2 Transform2D::TransformInverseNormal(const fv2& normal) const { return (rotation.InvRotate(normal) * scale).Norm(); }

    Transform3D Transform2D::As3D() const {
        return { position.AddZ(0), scale.AddZ(1), Rotor3D::RotateZ(rotation) };
    }
    void Transform2D::Reset() { position = 0; scale = 1; rotation = {}; }
    Matrix2x2 Transform2D::LinearMatrix() const {
        const fComplex& r = rotation.AsComplex();
        return {{
            r.re * scale.x, -r.im * scale.x,
            r.im * scale.y,  r.re * scale.y,
        }};
    }
    Matrix2D Transform2D::TransformMatrix() const {
        const fComplex& r = rotation.AsComplex();
        return {{ r.re * scale.x, -r.im * scale.x, position.x,
                  r.im * scale.y,  r.re * scale.y, position.y,
                  0,               0             , 1 }};
    }

    Transform2D Transform2D::operator*(const Transform2D& t) const {
        return { position + t.position.RotateBy(rotation) * scale, scale * t.scale, rotation + t.rotation };
    }

    Transform2D& Transform2D::operator*=(const Transform2D& t) {
        position += t.position.RotateBy(rotation) * scale;
        scale *= t.scale;
        rotation += t.rotation;
        return *this;
    }
} // Quasi