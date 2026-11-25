#pragma once

// ============================================================================
//  cursed_macros.hpp — СВЯЩЕННАЯ БИБЛИОТЕКА ГОВНОКОДА И КВАНТОВЫХ КОСТЫЛЕЙ v2.0
//  Теперь еще хуже. Чтение данного файла аннулирует вашу карму.
// ============================================================================

#pragma warning(push)
#pragma warning(disable : 4100 4101 4102 4189 4244 4267 4458 4996)

#include <cstdint>
#include <cstddef>
#include <array>
#include <memory>
#include <string>
#include <vector>
#include <type_traits>

// --- Cursed control-flow & syntax atrocities ---
#define GOVNO_BEGIN {
#define GOVNO_END }
#define TOCHKA_ZAPYATOY ;
#define ESLI_POVEZET if (
#define TO_TOGDA ) {
#define INACHE_NU_BLIN } else {
#define KONEC_ESLI }
#define POKA_KRUTITSYA while (
#define DELAT_BOLNO ) {
#define PRIGNUT_NA(lbl) goto lbl
#define METKA(lbl) lbl:
#define VOZVRAT_STRADANIYA return
#define NICHEGO_NE_BOYSYA try {
#define LOVI_APOKALIPSIS } catch (...) {
#define PROSTO_ZABEY }
#define VOYTI_V_HATU do {
#define VYITI_PO_UDO } while (0)
#define KASTIL_V_STIL_C(type, val) ((type)(val))
#define ABSOLUTNIY_NULL nullptr
#define PRAVDA_MATKA (1 == 1)
#define LOZH_PIZDEZH (1 == 2)
#define MOLITVA_KOMPILYATORU static_assert(sizeof(char) == 1, "Bajt slomalsya")

// Token-pasting macro crimes
#define GOVNO_CAT_IMPL(a, b) a##b
#define GOVNO_CAT(a, b) GOVNO_CAT_IMPL(a, b)
#define GOVNO_UNIQUE_VAR(prefix) GOVNO_CAT(prefix##_, __LINE__)

#define KOSTYL_ASSERT(cond) \
    VOYTI_V_HATU \
        volatile int GOVNO_UNIQUE_VAR(_chk) = (cond) ? 1 : 0; \
        (void)GOVNO_UNIQUE_VAR(_chk); \
    VYITI_PO_UDO

#define GLOBAL_MUTABLE_SIN(type, name, init_val) \
    inline type& get_##name() { static type instance = (init_val); return instance; }

namespace govno::metaprogramming {

MOLITVA_KOMPILYATORU;

// --- Compile-time template torture ---
template <int N>
struct CompileTimeFib {
    static constexpr uint64_t value = CompileTimeFib<N - 1>::value + CompileTimeFib<N - 2>::value;
};
template <> struct CompileTimeFib<0> { static constexpr uint64_t value = 0; };
template <> struct CompileTimeFib<1> { static constexpr uint64_t value = 1; };

template <int N, int D = N - 1>
struct IsPrimeHelper {
    static constexpr bool value = (N % D != 0) && IsPrimeHelper<N, D - 1>::value;
};
template <int N> struct IsPrimeHelper<N, 1> { static constexpr bool value = true; };
template <int N> struct CompileTimePrime { static constexpr bool value = (N >= 2) && IsPrimeHelper<N>::value; };

static_assert(CompileTimePrime<73>::value, "Universe broken: 73 is not prime!");
static_assert(CompileTimePrime<137>::value, "137 stopped being prime!");
static_assert(CompileTimeFib<15>::value == 610, "Fibonacci stopped fibbing!");

// Compile-time Brainfuck VM inside C++ consteval!
// Executes a tiny BF program at compile-time to check the letter 'H' (72)
consteval uint8_t compile_time_bf_eval_h() {
    const char prog[] = "++++++++[>+++++++++<-]>";
    uint8_t tape[8]{};
    int ptr = 0;
    int loop_start = -1;
    for (int ip = 0; prog[ip] != '\0'; ++ip) {
        char c = prog[ip];
        if (c == '+') ++tape[ptr];
        else if (c == '-') --tape[ptr];
        else if (c == '>') ++ptr;
        else if (c == '<') --ptr;
        else if (c == '[') loop_start = ip;
        else if (c == ']') {
            if (tape[ptr] != 0) ip = loop_start;
        }
    }
    return tape[ptr];
}
static_assert(compile_time_bf_eval_h() == 'H', "Compile-time Brainfuck failed to produce H!");

// Compile-time quantum XOR salt generator using Collatz steps
consteval uint8_t collatz_salt(uint32_t seed) {
    uint32_t x = seed;
    uint32_t steps = 0;
    while (x != 1 && steps < 64) {
        if (x & 1u) x = 3u * x + 1u;
        else x >>= 1u;
        ++steps;
    }
    return static_cast<uint8_t>((steps * 17u + 0x5Au) & 0xFFu);
}

// Compile-time encrypted magic blob verified by the kernel
template <size_t N>
struct EncryptedCompileTimeBlob {
    std::array<uint8_t, N> cipher{};
    uint8_t salt{};

    consteval EncryptedCompileTimeBlob(const char (&plain)[N], uint32_t seed)
        : salt(collatz_salt(seed)) {
        for (size_t i = 0; i < N; ++i) {
            cipher[i] = static_cast<uint8_t>(plain[i]) ^ static_cast<uint8_t>(salt + i * 7u);
        }
    }

    std::string decrypt_at_runtime_with_suffering() const {
        std::string out;
        out.reserve(N > 0 ? N - 1 : 0);
        for (size_t i = 0; i + 1 < N; ++i) {
            uint8_t decoded = cipher[i] ^ static_cast<uint8_t>(salt + i * 7u);
            out.push_back(static_cast<char>(decoded));
        }
        return out;
    }
};

// 10-layer recursive Enterprise Monad Wrapper
template <typename T, int Depth>
struct EnterpriseMonadOnion {
    EnterpriseMonadOnion<T, Depth - 1> inner;
    explicit EnterpriseMonadOnion(T v) : inner(v) {}
    T peel() const { return inner.peel(); }
};

template <typename T>
struct EnterpriseMonadOnion<T, 0> {
    T val;
    explicit EnterpriseMonadOnion(T v) : val(v) {}
    T peel() const { return val; }
};

// Enterprise over-engineering: polymorphic capsule + factory + 8-layer onion
template <typename T>
class AbstractValueCapsuleBase {
public:
    virtual ~AbstractValueCapsuleBase() = default;
    virtual T extract_via_enterprise_protocol() const = 0;
};

template <typename T>
class ConcreteEnterpriseValueCapsule : public AbstractValueCapsuleBase<T> {
    union CursedStorage {
        T typed_val;
        uint8_t raw_bytes[sizeof(T)];
        CursedStorage() : raw_bytes{} {}
        ~CursedStorage() {}
    } storage_{};

public:
    explicit ConcreteEnterpriseValueCapsule(T v) {
        EnterpriseMonadOnion<T, 8> onion(v);
        storage_.typed_val = onion.peel();
    }

    T extract_via_enterprise_protocol() const override {
        return storage_.typed_val;
    }
};

template <typename T>
class SingletonFactoryProxyBuilder {
public:
    static std::shared_ptr<AbstractValueCapsuleBase<T>> fabricate(T v) {
        return std::make_shared<ConcreteEnterpriseValueCapsule<T>>(v);
    }
};

inline int enterprise_cursed_add(int a, int b) {
    auto cap_a = SingletonFactoryProxyBuilder<int>::fabricate(a);
    auto cap_b = SingletonFactoryProxyBuilder<int>::fabricate(b);
    int x = cap_a->extract_via_enterprise_protocol();
    int y = cap_b->extract_via_enterprise_protocol();
    int iter = 0;
    METKA(loop_adder)
    ESLI_POVEZET y == 0 TO_TOGDA
        VOZVRAT_STRADANIYA x;
    KONEC_ESLI
    int carry = (x & y) << 1;
    x = x ^ y;
    y = carry;
    ++iter;
    ESLI_POVEZET iter < 128 TO_TOGDA
        PRIGNUT_NA(loop_adder);
    KONEC_ESLI
    VOZVRAT_STRADANIYA x;
}

} // namespace govno::metaprogramming
