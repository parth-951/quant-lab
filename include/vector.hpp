#pragma once

#include <vector>
#include <initializer_list>

class Vector {
private:
    std::vector<double> data;

public:
    Vector();
    Vector(size_t n);
    Vector(std::initializer_list<double> values);

    size_t size() const;

    double& operator[](size_t i);
    const double& operator[](size_t i) const;

    Vector operator+(const Vector& other) const;
    Vector operator-(const Vector& other) const;
    Vector operator*(double scalar) const;

    double dot(const Vector& other) const;
    double norm() const;
};