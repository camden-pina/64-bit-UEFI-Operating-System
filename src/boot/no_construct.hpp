#pragma once

namespace util {

// Helper template for creating objects without running constructors
template <typename T>
union no_construct {
    T value;
    no_construct() {}
    ~no_construct() {}
};
} // namespace util
