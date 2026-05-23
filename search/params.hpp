#pragma once

#include <string_view>
#include <print>
#include <charconv>
#include <string>
#include <cmath>

inline std::array<std::array<uint8_t, 63>, 127> reductions;
inline std::array<std::array<uint8_t, 16>, 2> lmp;

void reduction_cal();
void prune_cal();

#define TUNABLE_PARAMETERS \
    PARAM_CB(float, lmr_base, 0.5, 0, 2, 0.1, reduction_cal) \
    PARAM_CB(float, lmr_div, 3.5, 1, 8, 0.2, reduction_cal) \
    PARAM_CB(int, lmp_base, 5, 0, 10, 1, prune_cal) \
    PARAM_CB(float, lmp_nidiv, 2, 1, 7, 0.15, prune_cal) \
    PARAM_CB(float, lmp_idiv, 1, 0.5, 7, 0.15, prune_cal) \
    PARAM(int, futility_cutoff_scale, 120, 40, 200, 8) \
    PARAM(int, futility_cutoff_scale_imp, 70, 20, 120, 5) \
    PARAM(int, futility_scale, 140, 70, 210, 7) \
    PARAM(int, razoring_scale, 120, 30, 210, 9) \
    PARAM(int, null_search_div, 200, 100, 300, 10) \
    PARAM(int, null_search_depth_scale, 256, 128, 512, 19) \
    PARAM(int, probcut_margin, 230, 100, 340, 12) \
    PARAM(int, probcut_scale, 50, 10, 100, 4.5) \
    PARAM(int, history_prune_scale, 600, 100, 1200, 55) \
    PARAM(int, history_prune_div, 4096, 2048, 8192, 300) \
    PARAM(int, singular_margin, 682, 100, 2000, 95) \
    PARAM(int, singular_triple, 120, 50, 210, 8) \
    PARAM(int, singular_double, 30, 10, 120, 14) \
    PARAM(int, delta_margin, 150, 50, 210, 8)


#ifndef SPSA_TUNE

#define PARAM(type, name, def, min, max, step) \
    [[nodiscard]] inline constexpr type name() { return def; }
#define PARAM_CB(type, name, def, min, max, step, cb) \
    [[nodiscard]] inline constexpr type name() { return def; }

TUNABLE_PARAMETERS

#undef PARAM
#undef PARAM_CB

#else

namespace Tunables {
    #define PARAM(type, name, def, min, max, step) inline type name##_val = def;
    #define PARAM_CB(type, name, def, min, max, step, cb) inline type name##_val = def;

    TUNABLE_PARAMETERS

    #undef PARAM
    #undef PARAM_CB
}

#define PARAM(type, name, def, min, max, step) \
    [[nodiscard]] inline type name() { return Tunables::name##_val; }
#define PARAM_CB(type, name, def, min, max, step, cb) \
    [[nodiscard]] inline type name() { return Tunables::name##_val; }

TUNABLE_PARAMETERS

#undef PARAM
#undef PARAM_CB

namespace Tuning {
    template <typename T>
    T parse_value(std::string_view str) {
        double temp_val{};

        if (auto [ptr, ec] = std::from_chars(str.data(), str.data() + str.size(), temp_val); ec != std::errc()) {
            std::println("info string Warning: Failed to parse tunable value '{}'", str);
        }

        if constexpr (std::is_integral_v<T>) {
            return static_cast<T>(std::round(temp_val));
        } else {
            return static_cast<T>(temp_val);
        }
    }

    inline void print_options() {
        #define PARAM(type, name, def, min, max, step) \
            std::println("option name {} type string default {} min {} max {}", #name, def, min, max);
        #define PARAM_CB(type, name, def, min, max, step, cb) \
            std::println("option name {} type string default {} min {} max {}", #name, def, min, max);

        TUNABLE_PARAMETERS

        #undef PARAM
        #undef PARAM_CB
    }

    inline void print_spsa_params() {
        #define PARAM(type, name, def, min, max, step) \
            std::println("{}, {}, {}, {}, {}, {}", #name, #type, def, min, max, step);
        #define PARAM_CB(type, name, def, min, max, step, cb) \
            std::println("{}, {}, {}, {}, {}, {}", #name, #type, def, min, max, step);

        TUNABLE_PARAMETERS

        #undef PARAM
        #undef PARAM_CB
    }

    inline void set_parameter(std::string_view name, const std::string_view value_str) {
        #define PARAM(type, p_name, def, min, max, step) \
            if (name == #p_name) { \
                Tunables::p_name##_val = parse_value<type>(value_str); \
                return; \
            }
        #define PARAM_CB(type, p_name, def, min, max, step, cb) \
            if (name == #p_name) { \
                Tunables::p_name##_val = parse_value<type>(value_str); \
                cb(); \
                return; \
            }

        TUNABLE_PARAMETERS

        #undef PARAM
        #undef PARAM_CB

        std::println("info string Unknown tunable parameter: {}", name);
    }
}

#endif