#pragma once

#include <string_view>
#include <print>
#include <charconv>
#include <cmath>
#include <cstdint>

#include "../options.hpp"

inline std::array<std::array<int, 63>, MAX_PLY> reductions;
inline std::array<std::array<int, 16>, 2> lmp;

void reduction_cal();
void prune_cal();

#define TUNABLE_PARAMETERS \
    PARAM_CB(double, lmr_base, 0.47, 0, 2, 0.1, reduction_cal) \
    PARAM_CB(double, lmr_div, 3.66, 1, 8, 0.35, reduction_cal) \
    PARAM_CB(int, lmp_base, 7, 0, 10, 1, prune_cal) \
    PARAM_CB(double, lmp_nidiv, 1.88, 1, 7, 0.3, prune_cal) \
    PARAM_CB(double, lmp_idiv, 0.74, 0.5, 7, 0.325, prune_cal) \
    PARAM(int, futility_cutoff_scale, 74, 40, 200, 8) \
    PARAM(int, futility_cutoff_scale_imp, 63, 20, 120, 5) \
    PARAM(int, futility_scale, 143, 70, 210, 7) \
    PARAM(int, razoring_scale, 149, 30, 210, 9) \
    PARAM(int, null_search_div, 233, 100, 300, 10) \
    PARAM(int, null_search_depth_scale, 390, 128, 512, 19) \
    PARAM(int, probcut_margin, 244, 100, 340, 12) \
    PARAM(int, probcut_scale, 52, 10, 100, 5) \
    PARAM(int, history_prune_scale, 782, 100, 1200, 55) \
    PARAM(int, history_prune_div, 3722, 2048, 8192, 300) \
    PARAM(int, singular_margin, 672, 100, 2000, 95) \
    PARAM(int, singular_triple, 147, 50, 210, 8) \
    PARAM(int, singular_double, 10, 10, 120, 6) \
    PARAM(int, delta_margin, 181, 50, 210, 8) \
    PARAM(double, max_time_scale, 0.43, 0.1, 0.8, 0.04) \
    PARAM(double, opt_time_scale, 0.96, 0.4, 1.0, 0.03) \
    PARAM(int, default_moves_to_go, 28, 10, 40, 1) \
    PARAM(double, stable_base_scale, 1.9, 1.2, 3.5, 0.115) \
    PARAM(double, stable_scale, 0.11, 0.01, 0.25, 0.012) \
    PARAM(int, score_diff_scale, 143, 100, 300, 10) \
    PARAM(int, piece_to_history_weight, 591, 256, 2048, 96) \
    PARAM(int, butterfly_history_weight, 931, 256, 2048, 96) \
    PARAM(int, counter_move_weight, 1370, 256, 2048, 96) \
    PARAM(int, follow_up_weight, 1289, 256, 2048, 96) \
    PARAM(int, four_plies_weight, 647, 256, 2048, 96) \
    PARAM(int, mvv_weight, 1461, 256, 2048, 96) \
    PARAM(int, capture_history_weight, 627, 256, 2048, 96) \
    PARAM(int16_t, max_capture_history, 29873, 8192, INT16_MAX, 768) \
    PARAM(int, capture_history_bonus, 91, 10, 500, 25) \
    PARAM(int16_t, max_butterfly_history, 7745, 4096, INT16_MAX, 768) \
    PARAM(int, butterfly_history_scale, 53, 1, 100, 5) \
    PARAM(int, butterfly_history_minus, 4, 1, 100, 5) \
    PARAM(int16_t, max_piece_to_history, 4127, 4096, INT16_MAX, 768) \
    PARAM(int, piece_to_history_scale, 29, 1, 100, 5) \
    PARAM(int, piece_to_history_minus, 16, 1, 100, 5) \
    PARAM(int16_t, max_continuation_history, 8071, 4096, INT16_MAX, 768) \
    PARAM(int, continuation_history_scale, 48, 1, 100, 5) \
    PARAM(int, continuation_history_minus, 20, 1, 100, 5) \
    PARAM(int, pawn_weight, 154, 50, 300, 13) \
    PARAM(int, knight_weight, 1157, 390, 1560, 59) \
    PARAM(int, bishop_weight, 1124, 412, 1650, 80) \
    PARAM(int, rook_weight, 1893, 640, 2550, 96) \
    PARAM(int, queen_weight, 2613, 1270, 5080, 191) \
    PARAM(int, initial_aspiration_window, 16, 10, 160, 8) \
    PARAM(int, aspiration_expansion_rate, 78, 10, 160, 8) \
    PARAM(int, reduction_table_weight, 1466, 128, 2048, 96) \
    PARAM(int, reduction_cut_node_weight, 974, 128, 2048, 96) \
    PARAM(int, reduction_history_weight, 898, 128, 2048, 96) \
    PARAM(int, reduction_non_pv_weight, 1016, 128, 4096, 96) \
    PARAM(int, reduction_improving_weight, 931, 128, 4096, 96) \
    PARAM(int, reduction_tt_depth_weight, 1063, 128, 4096, 96) \
    PARAM(int, reduction_killer_weight, 2117, 128, 4096, 96) \
    PARAM(int, reduction_check_weight, 718, 128, 4096, 96) \
    PARAM(int, pawn_correction_weight, 66, 16, 192, 90) \
    PARAM(int, non_pawn_correction_weight, 134, 16, 192, 9) \
    PARAM(int, major_correction_weight, 153, 16, 192, 9) \
    PARAM(int, continuation_correction_weight_1, 134, 16, 192, 9) \
    PARAM(int, continuation_correction_weight_2, 98, 16, 192, 9) \
    PARAM(int, continuation_correction_weight_4, 130, 16, 192, 9) \

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