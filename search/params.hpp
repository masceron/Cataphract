#pragma once

#include <string_view>
#include <print>
#include <charconv>
#include <string>
#include <cmath>
#include <cstdint>

#include "../options.hpp"

inline std::array<std::array<int, 63>, MAX_PLY> reductions;
inline std::array<std::array<int, 16>, 2> lmp;

void reduction_cal();
void prune_cal();

#define TUNABLE_PARAMETERS \
    PARAM_CB(double, lmr_base, 0.53, 0, 2, 0.1, reduction_cal) \
    PARAM_CB(double, lmr_div, 4.0, 1, 8, 0.35, reduction_cal) \
    PARAM_CB(int, lmp_base, 7, 0, 10, 1, prune_cal) \
    PARAM_CB(double, lmp_nidiv, 1.71, 1, 7, 0.3, prune_cal) \
    PARAM_CB(double, lmp_idiv, 0.62, 0.5, 7, 0.325, prune_cal) \
    PARAM(int, futility_cutoff_scale, 85, 40, 200, 8) \
    PARAM(int, futility_cutoff_scale_imp, 62, 20, 120, 5) \
    PARAM(int, futility_scale, 150, 70, 210, 7) \
    PARAM(int, razoring_scale, 123, 30, 210, 9) \
    PARAM(int, null_search_div, 224, 100, 300, 10) \
    PARAM(int, null_search_depth_scale, 315, 128, 512, 19) \
    PARAM(int, probcut_margin, 238, 100, 340, 12) \
    PARAM(int, probcut_scale, 51, 10, 100, 5) \
    PARAM(int, history_prune_scale, 711, 100, 1200, 55) \
    PARAM(int, history_prune_div, 3926, 2048, 8192, 300) \
    PARAM(int, singular_margin, 773, 100, 2000, 95) \
    PARAM(int, singular_triple, 132, 50, 210, 8) \
    PARAM(int, singular_double, 13, 10, 120, 6) \
    PARAM(int, delta_margin, 179, 50, 210, 8) \
    PARAM(double, max_time_scale, 0.3, 0.1, 0.8, 0.04) \
    PARAM(double, opt_time_scale, 0.84, 0.4, 1.2, 0.04) \
    PARAM(int, default_moves_to_go, 28, 10, 40, 1) \
    PARAM(double, stable_base_scale, 1.8, 1.2, 3.5, 0.115) \
    PARAM(double, stable_scale, 0.1, 0.01, 0.25, 0.012) \
    PARAM(int, score_diff_scale, 153, 100, 300, 10) \
    PARAM(int, piece_history_weight, 452, 256, 2048, 96) \
    PARAM(int, counter_move_weight, 1257, 256, 2048, 96) \
    PARAM(int, follow_up_weight, 1420, 256, 2048, 96) \
    PARAM(int, four_plies_weight, 791, 256, 2048, 96) \
    PARAM(int, mvv_weight, 1175, 256, 2048, 96) \
    PARAM(int, capture_history_weight, 684, 256, 2048, 96) \
    PARAM(int16_t, max_capture_history, 29591, 8192, INT16_MAX, 768) \
    PARAM(int, capture_history_bonus, 161, 10, 500, 25) \
    PARAM(int16_t, max_butterfly_history, 8254, 4096, INT16_MAX, 768) \
    PARAM(int, butterfly_history_scale, 32, 1, 100, 5) \
    PARAM(int, butterfly_history_minus, 8, 1, 100, 5) \
    PARAM(int16_t, max_piece_to_history, 6161, 4096, INT16_MAX, 768) \
    PARAM(int, piece_to_history_scale, 17, 1, 100, 5) \
    PARAM(int, piece_to_history_minus, 7, 1, 100, 5) \
    PARAM(int16_t, max_continuation_history, 10277, 4096, INT16_MAX, 768) \
    PARAM(int, continuation_history_scale, 44, 1, 100, 5) \
    PARAM(int, continuation_history_minus, 9, 1, 100, 5) \
    PARAM(int, pawn_weight, 150, 50, 300, 13) \
    PARAM(int, knight_weight, 992, 390, 1560, 59) \
    PARAM(int, bishop_weight, 1060, 412, 1650, 80) \
    PARAM(int, rook_weight, 1617, 640, 2550, 96) \
    PARAM(int, queen_weight, 2283, 1270, 5080, 191) \
    PARAM(int, initial_aspiration_window, 25, 10, 160, 8) \
    PARAM(int, aspiration_expansion_rate, 65, 10, 160, 8) \
    PARAM(int, reduction_table_weight, 1358, 128, 2048, 96) \
    PARAM(int, reduction_cut_node_weight, 903, 128, 2048, 96) \
    PARAM(int, reduction_history_weight, 977, 128, 2048, 96) \
    PARAM(int, reduction_non_pv_weight, 1017, 128, 4096, 96) \
    PARAM(int, reduction_improving_weight, 906, 128, 4096, 96) \
    PARAM(int, reduction_tt_depth_weight, 1006, 128, 4096, 96) \
    PARAM(int, reduction_killer_weight, 2018, 128, 4096, 96) \
    PARAM(int, reduction_check_weight, 1012, 128, 4096, 96) \
    PARAM(int, pawn_correction_weight, 180, 16, 192, 90) \
    PARAM(int, non_pawn_correction_weight, 180, 16, 192, 9) \
    PARAM(int, minor_correction_weight, 180, 16, 192, 9) \
    PARAM(int, major_correction_weight, 180, 16, 192, 9) \


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