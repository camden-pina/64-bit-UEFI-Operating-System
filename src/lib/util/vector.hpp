#pragma once

#include <stdint.h>
#include <stddef.h>

namespace util {

template<typename T>
class vector {
public:
    T *pointer;
    size_t count;

    inline T &operator[](int i) { return pointer[i]; }
    inline const T &operator[](int i) const { return pointer[i]; }
}; // class vector

template <>
class vector<void> {
    void *pointer;
    size_t count;
};

} // namespace util
