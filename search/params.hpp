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
    PARAM_CB(double, lmr_base, 0.52, 0, 2, 0.1, reduction_cal) \
    PARAM_CB(double, lmr_div, 3.87, 1, 8, 0.35, reduction_cal) \
    PARAM_CB(int, lmp_base, 7, 0, 10, 1, prune_cal) \
    PARAM_CB(double, lmp_nidiv, 1.58, 1, 7, 0.3, prune_cal) \
    PARAM_CB(double, lmp_idiv, 0.77, 0.5, 7, 0.325, prune_cal) \
    PARAM(int, futility_cutoff_scale, 84, 40, 200, 8) \
    PARAM(int, futility_cutoff_scale_imp, 62, 20, 120, 5) \
    PARAM(int, futility_scale, 140, 70, 210, 7) \
    PARAM(int, razoring_scale, 132, 30, 210, 9) \
    PARAM(int, null_search_div, 220, 100, 300, 10) \
    PARAM(int, null_search_depth_scale, 359, 128, 512, 19) \
    PARAM(int, probcut_margin, 242, 100, 340, 12) \
    PARAM(int, probcut_scale, 50, 10, 100, 5) \
    PARAM(int, history_prune_scale, 715, 100, 1200, 55) \
    PARAM(int, history_prune_div, 3922, 2048, 8192, 300) \
    PARAM(int, singular_margin, 715, 100, 2000, 95) \
    PARAM(int, singular_triple, 142, 50, 210, 8) \
    PARAM(int, singular_double, 10, 10, 120, 6) \
    PARAM(int, delta_margin, 175, 50, 210, 8) \
    PARAM(double, max_time_scale, 0.32, 0.1, 0.8, 0.04) \
    PARAM(double, opt_time_scale, 0.93, 0.4, 1.2, 0.04) \
    PARAM(int, default_moves_to_go, 28, 10, 40, 1) \
    PARAM(double, stable_base_scale, 1.96, 1.2, 3.5, 0.115) \
    PARAM(double, stable_scale, 0.13, 0.01, 0.25, 0.012) \
    PARAM(int, score_diff_scale, 140, 100, 300, 10) \
    PARAM(int, piece_to_history_weight, 1024, 256, 2048, 96) \
    PARAM(int, butterfly_history_weight, 1024, 256, 2048, 96) \
    PARAM(int, counter_move_weight, 1360, 256, 2048, 96) \
    PARAM(int, follow_up_weight, 1443, 256, 2048, 96) \
    PARAM(int, four_plies_weight, 745, 256, 2048, 96) \
    PARAM(int, mvv_weight, 1379, 256, 2048, 96) \
    PARAM(int, capture_history_weight, 738, 256, 2048, 96) \
    PARAM(int16_t, max_capture_history, 28985, 8192, INT16_MAX, 768) \
    PARAM(int, capture_history_bonus, 130, 10, 500, 25) \
    PARAM(int16_t, max_butterfly_history, 8292, 4096, INT16_MAX, 768) \
    PARAM(int, butterfly_history_scale, 43, 1, 100, 5) \
    PARAM(int, butterfly_history_minus, 6, 1, 100, 5) \
    PARAM(int16_t, max_piece_to_history, 6170, 4096, INT16_MAX, 768) \
    PARAM(int, piece_to_history_scale, 27, 1, 100, 5) \
    PARAM(int, piece_to_history_minus, 22, 1, 100, 5) \
    PARAM(int16_t, max_continuation_history, 8898, 4096, INT16_MAX, 768) \
    PARAM(int, continuation_history_scale, 44, 1, 100, 5) \
    PARAM(int, continuation_history_minus, 17, 1, 100, 5) \
    PARAM(int, pawn_weight, 150, 50, 300, 13) \
    PARAM(int, knight_weight, 1149, 390, 1560, 59) \
    PARAM(int, bishop_weight, 1078, 412, 1650, 80) \
    PARAM(int, rook_weight, 1772, 640, 2550, 96) \
    PARAM(int, queen_weight, 2526, 1270, 5080, 191) \
    PARAM(int, initial_aspiration_window, 23, 10, 160, 8) \
    PARAM(int, aspiration_expansion_rate, 59, 10, 160, 8) \
    PARAM(int, reduction_table_weight, 1413, 128, 2048, 96) \
    PARAM(int, reduction_cut_node_weight, 1008, 128, 2048, 96) \
    PARAM(int, reduction_history_weight, 1077, 128, 2048, 96) \
    PARAM(int, reduction_non_pv_weight, 974, 128, 4096, 96) \
    PARAM(int, reduction_improving_weight, 950, 128, 4096, 96) \
    PARAM(int, reduction_tt_depth_weight, 1179, 128, 4096, 96) \
    PARAM(int, reduction_killer_weight, 2028, 128, 4096, 96) \
    PARAM(int, reduction_check_weight, 864, 128, 4096, 96) \
    PARAM(int, pawn_correction_weight, 114, 16, 192, 90) \
    PARAM(int, non_pawn_correction_weight, 167, 16, 192, 9) \
    PARAM(int, major_correction_weight, 151, 16, 192, 9) \
    PARAM(int, continuation_correction_weight_1, 120, 16, 192, 9) \
    PARAM(int, continuation_correction_weight_2, 120, 16, 192, 9) \
    PARAM(int, continuation_correction_weight_4, 120, 16, 192, 9) \

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