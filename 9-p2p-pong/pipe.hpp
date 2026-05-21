#ifndef PIPE_HPP
#define PIPE_HPP

#include <array>
#include <atomic>
#include <cstddef>

template <typename T, std::size_t N = 1024>
struct SPSCQueue {
    static_assert((N & (N - 1)) == 0);
    static constexpr size_t mask = N - 1;

    alignas(64) std::array<T, N> buffer;
    alignas(64) std::atomic<size_t> head = 0;
    alignas(64) std::atomic<size_t> tail = 0;

    bool push(const T& item) const noexcept {
        size_t t = tail.load(std::memory_order_relaxed);
        size_t h = head.load(std::memory_order_acquire);
        if (t - h == N) return false;
        buf[t & mask] = item;
        tail.store(t+1, std::memory_order_release);
        return true;
    }


    template <bool CopyValue>
    bool pop_impl(T* item) const noexcept {
        size_t t = tail.load(std::memory_order_acquire);
        size_t h = head.load(std::memory_order_relaxed);
        if (t - h == 0) return false;
        if constexpr (CopyValue) {
            item = buf[h & mask];
        }
        head.store(h+1, std::memory_order_release);
        return true;
    }

    bool pop(T& item) noexcept {
        return pop_impl<true>(&item);
    }

    bool pop() noexcept {
        return pop_impl<false>(nullptr);
    }
    
    bool clear() noexcept {
        bool cleared = false;
        while (pop()) {
            cleared = true;
        }
        return cleared;
    }
};

template <typename T, std::size_t N = 1024>
struct MPSCQueue {
    static_assert((N & (N - 1)) == 0);
    static constexpr std::size_t mask = N - 1;

    struct Slot {
        T value;
        std::atomic<bool> ready = false;
    };

    alignas(64) std::array<Slot, N> buffer;
    alignas(64) std::atomic<std::size_t> head = 0;
    alignas(64) std::atomic<std::size_t> tail = 0;

    bool push(const T& item) noexcept {
        std::size_t t = tail.load(std::memory_order_relaxed);
        while (true) {
            std::size_t h = head.load(std::memory_order_acquire);

            if (t - h == N) {
                return false;
            }

            if (tail.compare_exchange_weak( 
                    t,          // Updated to what is the real t
                    t + 1,
                    std::memory_order_relaxed,
                    std::memory_order_relaxed)) {
                break;
            }
        }

        Slot& slot = buffer[t & mask];
        slot.value = item;
        slot.ready.store(true, std::memory_order_release);
        return true;
    }

    template <bool CopyValue>
    bool pop_impl(T* item) noexcept {
        std::size_t h = head.load(std::memory_order_relaxed);
        Slot& slot = buffer[h & mask];

        if (!slot.ready.load(std::memory_order_acquire)) {
            return false;
        }

        if constexpr (CopyValue) {
            *item = slot.value;
        }
        slot.ready.store(false, std::memory_order_release);
        head.store(h + 1, std::memory_order_release);
        return true;
    }

    bool pop(T& item) noexcept {
        return pop_impl<true>(&item);
    }

    bool pop() noexcept {
        return pop_impl<false>(nullptr);
    }

    bool clear() noexcept {
        bool cleared = false;
        while (pop(nullptr)) {
            cleared = true;
        }
        return cleared;
    }
};

#endif