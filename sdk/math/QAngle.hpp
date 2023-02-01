#pragma once

class QAngle
{

public:

    QAngle(void)
    {
        Init();
    }

    QAngle(float X, float Y, float Z)
    {
        Init(X, Y, Z);
    }

    QAngle(const float* clr)
    {
        Init(clr[0], clr[1], clr[2]);
    }

    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;


    FORCEINLINE void  Set(float x = 0.0f, float y = 0.0f, float z = 0.0f)
    {
        this->x = x;
        this->y = y;
        this->z = z;
    }

    FORCEINLINE QAngle ToVectors(Vector* side /*= nullptr*/, Vector* up /*= nullptr*/)
    {
        auto rad_pitch = ToRadians(this->x);
        auto rad_yaw = ToRadians(this->y);
        float rad_roll;

        auto sin_pitch = sinf(rad_pitch);
        auto sin_yaw = sinf(rad_yaw);
        float sin_roll;

        auto cos_pitch = cosf(rad_pitch);
        auto cos_yaw = cosf(rad_yaw);
        float cos_roll;

        if (side || up) {
            rad_roll = ToRadians(this->z);
            sin_roll = sinf(rad_roll);
            cos_roll = cosf(rad_roll);
        }

        if (side) {
            side->x = -1.0f * sin_roll * sin_pitch * cos_yaw + -1.0f * cos_roll * -sin_yaw;
            side->y = -1.0f * sin_roll * sin_pitch * sin_yaw + -1.0f * cos_roll * cos_yaw;
            side->z = -1.0f * sin_roll * cos_pitch;
        }

        if (up) {
            up->x = cos_roll * sin_pitch * cos_yaw + -sin_roll * -sin_yaw;
            up->y = cos_roll * sin_pitch * sin_yaw + -sin_roll * cos_yaw;
            up->z = cos_roll * cos_pitch;
        }

        return { cos_pitch * cos_yaw, cos_pitch * sin_yaw, -sin_pitch };
    }

    void Init(float ix = 0.0f, float iy = 0.0f, float iz = 0.0f)
    {
        pitch = ix;
        yaw = iy;
        roll = iz;
    }

    __inline bool IsValid() const
    {
        return std::isfinite(pitch) && std::isfinite(yaw) && std::isfinite(roll);
    }

    float operator[](int i) const
    {
        return ((float*)this)[i];
    }

    float& operator[](int i)
    {
        return ((float*)this)[i];
    }

    bool operator==(const QAngle& src) const
    {
        return (src.pitch == pitch) && (src.yaw == yaw) && (src.roll == roll);
    }

    bool operator!=(const QAngle& src) const
    {
        return (src.pitch != pitch) || (src.yaw != yaw) || (src.roll != roll);
    }

    QAngle& operator+=(const QAngle& v)
    {
        pitch += v.pitch; yaw += v.yaw; roll += v.roll;
        return *this;
    }

    QAngle& operator-=(const QAngle& v)
    {
        pitch -= v.pitch; yaw -= v.yaw; roll -= v.roll;
        return *this;
    }

    QAngle& operator*=(float fl)
    {
        pitch *= fl;
        yaw *= fl;
        roll *= fl;
        return *this;
    }

    QAngle& operator*=(const QAngle& v)
    {
        pitch *= v.pitch;
        yaw *= v.yaw;
        roll *= v.roll;
        return *this;
    }

    QAngle& operator/=(const QAngle& v)
    {
        pitch /= v.pitch;
        yaw /= v.yaw;
        roll /= v.roll;
        return *this;
    }

    QAngle& operator+=(float fl)
    {
        pitch += fl;
        yaw += fl;
        roll += fl;
        return *this;
    }

    QAngle& operator/=(float fl)
    {
        pitch /= fl;
        yaw /= fl;
        roll /= fl;
        return *this;
    }

    QAngle& operator-=(float fl)
    {
        pitch -= fl;
        yaw -= fl;
        roll -= fl;
        return *this;
    }

    QAngle& operator=(const QAngle& vOther)
    {
        pitch = vOther.pitch; yaw = vOther.yaw; roll = vOther.roll;
        return *this;
    }

    QAngle operator-(void) const
    {
        return QAngle(-pitch, -yaw, -roll);
    }

    QAngle operator+(const QAngle& v) const
    {
        return QAngle(pitch + v.pitch, yaw + v.yaw, roll + v.roll);
    }

    QAngle operator-(const QAngle& v) const
    {
        return QAngle(pitch - v.pitch, yaw - v.yaw, roll - v.roll);
    }

    QAngle operator*(float fl) const
    {
        return QAngle(pitch * fl, yaw * fl, roll * fl);
    }

    QAngle operator*(const QAngle& v) const
    {
        return QAngle(pitch * v.pitch, yaw * v.yaw, roll * v.roll);
    }

    QAngle operator/(float fl) const
    {
        return QAngle(pitch / fl, yaw / fl, roll / fl);
    }

    QAngle operator/(const QAngle& v) const
    {
        return QAngle(pitch / v.pitch, yaw / v.yaw, roll / v.roll);
    }

    float Length() const
    {
        return sqrt(pitch * pitch + yaw * yaw + roll * roll);
    }

    float LengthSqr(void) const
    {
        return (pitch * pitch + yaw * yaw + roll * roll);
    }

    bool IsZero(float tolerance = 0.01f) const
    {
        return (pitch > -tolerance && pitch < tolerance&&
            yaw > -tolerance && yaw < tolerance&&
            roll > -tolerance && roll < tolerance);
    }

    auto Clamp()
    {
        while (this->pitch < -89.0f)
            this->pitch += 89.0f;

        if (this->pitch > 89.0f)
            this->pitch = 89.0f;

        while (this->yaw < -180.0f)
            this->yaw += 360.0f;

        while (this->yaw > 180.0f)
            this->yaw -= 360.0f;

        this->roll = 0.0f;
    }

    auto Normalize()
    {
        auto x_rev = this->pitch / 360.f;
        if (this->pitch > 180.f || this->pitch < -180.f)
        {
            x_rev = abs(x_rev);
            x_rev = round(x_rev);

            if (this->pitch < 0.f)
                this->pitch = (this->pitch + 360.f * x_rev);

            else
                this->pitch = (this->pitch - 360.f * x_rev);
        }

        auto y_rev = this->yaw / 360.f;
        if (this->yaw > 180.f || this->yaw < -180.f)
        {
            y_rev = abs(y_rev);
            y_rev = round(y_rev);

            if (this->yaw < 0.f)
                this->yaw = (this->yaw + 360.f * y_rev);

            else
                this->yaw = (this->yaw - 360.f * y_rev);
        }

        auto z_rev = this->roll / 360.f;
        if (this->roll > 180.f || this->roll < -180.f)
        {
            z_rev = abs(z_rev);
            z_rev = round(z_rev);

            if (this->roll < 0.f)
                this->roll = (this->roll + 360.f * z_rev);

            else
                this->roll = (this->roll - 360.f * z_rev);
        }
    }

    float RealYawDifference(QAngle dst, QAngle add = QAngle());
    float Difference(QAngle dst, QAngle add);

    QAngle NormalizeYaw()
    {
        while (this->yaw > 180)
            this->yaw -= 360;

        while (this->yaw < -180)
            this->yaw += 360;

        return *this;
    }

    float
        pitch,
        yaw,
        roll;
};

inline QAngle operator*(float lhs, const QAngle& rhs)
{
    return rhs * lhs;
}

inline QAngle operator/(float lhs, const QAngle& rhs)
{
    return rhs / lhs;
}