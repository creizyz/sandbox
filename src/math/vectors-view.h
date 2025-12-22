#pragma once

#include <array>
#include "vectors.h"

namespace math
{
    template <std::size_t N, typename T>
    struct VectorViewBase
    {
        std::array<T*, N> components;

        T& operator[](std::size_t i) { return *components[i]; }
        const T& operator[](std::size_t i) const { return *components[i]; }

        explicit operator Vector<N, T>() const
        {
            Vector<N, T> result;
            for (std::size_t i = 0; i < N; ++i)
            {
                result[i] = (*this)[i];
            }
            return result;
        }
    };

    template <std::size_t N, typename T>
    struct VectorView : public VectorViewBase<N, T>
    { };

    template <typename T>
    struct VectorView<2, T> : public VectorViewBase<2, T>
    {
        T& x() { return *this->components[0]; }
        T& y() { return *this->components[1]; }

        const T& x() const { return *this->components[0]; }
        const T& y() const { return *this->components[1]; }
    };

    template <typename T>
    struct VectorView<3, T> : public VectorViewBase<3, T>
    {
        T& x() { return *this->components[0]; }
        T& y() { return *this->components[1]; }
        T& z() { return *this->components[2]; }

        const T& x() const { return *this->components[0]; }
        const T& y() const { return *this->components[1]; }
        const T& z() const { return *this->components[2]; }

        T& r() { return *this->components[0]; }
        T& g() { return *this->components[1]; }
        T& b() { return *this->components[2]; }

        const T& r() const { return *this->components[0]; }
        const T& g() const { return *this->components[1]; }
        const T& b() const { return *this->components[2]; }
    };

    template <typename T>
    struct VectorView<4, T> : public VectorViewBase<4, T>
    {
        T& x() { return *this->components[0]; }
        T& y() { return *this->components[1]; }
        T& z() { return *this->components[2]; }
        T& w() { return *this->components[3]; }

        const T& x() const { return *this->components[0]; }
        const T& y() const { return *this->components[1]; }
        const T& z() const { return *this->components[2]; }
        const T& w() const { return *this->components[3]; }

        T& r() { return *this->components[0]; }
        T& g() { return *this->components[1]; }
        T& b() { return *this->components[2]; }
        T& a() { return *this->components[3]; }

        const T& r() const { return *this->components[0]; }
        const T& g() const { return *this->components[1]; }
        const T& b() const { return *this->components[2]; }
        const T& a() const { return *this->components[3]; }
    };

    template <typename T>
    using VectorView2 = VectorView<2, T>;

    template <typename T>
    using VectorView3 = VectorView<3, T>;

    template <typename T>
    using VectorView4 = VectorView<4, T>;

    template <size_t N, typename T>
    using ConstVectorView = VectorView<N, const T>;
}