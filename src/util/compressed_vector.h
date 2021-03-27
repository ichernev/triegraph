#ifndef __COMPRESSED_VECTOR_H__
#define __COMPRESSED_VECTOR_H__

#include <string.h>

namespace triegraph {

template <typename T>
struct compressed_vector {
    static constexpr unsigned max_cap = sizeof(std::vector<T>)/sizeof(T);
    static constexpr unsigned real_cap = max_cap - sizeof(void*) / sizeof(T);
    using const_iterator = typename std::vector<T>::const_iterator;

    union {
        std::vector<T> vec;
        struct {
            T raw[real_cap];
            long long len;
        };
    };

    compressed_vector() : len(0) {}
    ~compressed_vector() {
        if (len > real_cap) {
            // std::cerr << "destroying vector" << std::endl;
            (&vec)->~vector();
        } else {
            // std::cerr << "chilling" << std::endl;
        }

    }

    compressed_vector(compressed_vector &&other) {
        if (other.is_fast()) {
            memcpy(raw, other.raw, sizeof(T) * real_cap);
            len = other.len;
        } else {
            new (&vec) std::vector<T>(std::move(other.vec));
        }
    }

    compressed_vector &operator= (compressed_vector &&other) {
        if (is_fast()) {
            if (other.is_fast()) {
                memcpy(raw, other.raw, sizeof(T) * real_cap);
                len = other.len;
            } else {
                new (&vec) std::vector<T>(std::move(other.vec));
            }
        } else {
            if (other.is_fast()) {
                (&vec)->~vector();
                memcpy(raw, other.raw, sizeof(T) * real_cap);
                len = other.len;
            } else {
                vec = std::move(other.vec);
            }
        }
    }

    T &operator[](size_t i) {
        if (is_fast()) {
            return raw[i];
        }
        return vec[i];
    }

    const T &operator[](size_t i) const {
        if (is_fast()) {
            return raw[i];
        }
        return vec[i];
    }

    unsigned size() const {
        if (is_fast()) {
            return len;
        }
        return vec.size();
    }

    const_iterator begin() const {
        if (is_fast()) {
            return const_iterator(&raw[0]);
        }
        return vec.begin();
    }

    const_iterator end() const {
        if (is_fast()) {
            return const_iterator(&raw[len]);
        }
        return vec.end();
    }

    void push_back(const T &t) {
        if (len < real_cap) {
            raw[len++] = t;
        } else if (len == real_cap) {
            // std::cerr << "transitioning to vector" << std::endl;
            T rawx[real_cap + 1];
            memcpy(rawx, raw, sizeof(T) * real_cap);
            rawx[real_cap] = t;
            new(&vec) std::vector<T>(rawx, rawx + real_cap + 1);
        } else {
            vec.push_back(t);
        }
    }

    template <typename... Args>
    void emplace_back(Args&&... args) {
        return push_back(T(std::forward<Args>(args)...));
    }

    bool is_fast() const {
        return len <= real_cap;
    }
};

} /* namespace triegraph */

#endif /* __COMPRESSED_VECTOR_H__ */
