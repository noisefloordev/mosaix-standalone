#ifndef Vec4f_h
#define Vec4f_h

struct Vec4f
{
    float x, y, z, w; 

    Vec4f(): x(0), y(0), z(0), w(0) { }
    Vec4f(float x_, float y_, float z_, float w_):
        x(x_),
        y(y_),
        z(z_),
        w(w_)
    {
    }

    const float &operator[](int i) const { return (&x)[i]; }
    float &operator[](int i) { return (&x)[i]; }

    inline Vec4f operator+(const Vec4f &v) const
    {
        return Vec4f(x + v.x, y + v.y, z + v.z, w + v.w);
    }

    inline const Vec4f &operator +=(const Vec4f &v)
    {
        x += v.x;
        y += v.y;
        z += v.z;
        w += v.w;
        return *this;
    }

    inline Vec4f operator*(const Vec4f &v) const
    {
        return Vec4f(x * v.x, y * v.y, z * v.z, w * v.w);
    }

    inline Vec4f operator*(float f) const
    {
        return Vec4f(x * f, y * f, z * f, w * f);
    }

    inline const Vec4f &operator *=(float f)
    {
        x *= f;
        y *= f;
        z *= f;
        w *= f;
        return *this;
    }
};

#endif
