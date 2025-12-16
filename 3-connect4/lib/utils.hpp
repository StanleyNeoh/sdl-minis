#ifndef LIB_UTILS_HPP
#define LIB_UTILS_HPP

template <typename T>
T* unsafe_shift(T* ptr, int n_bytes) {
    return reinterpret_cast<T*>(reinterpret_cast<uint8_t*>(ptr) + n_bytes);
}

#endif