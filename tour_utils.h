#pragma once
#include "point.h"
#include <vector>
#include <algorithm>
#include <unordered_map>

// Reversión eficiente de un segmento del tour
inline void reverse_segment(std::vector<Point>& tour, size_t start, size_t end) {
    while (start < end) {
        std::swap(tour[start], tour[end]);
        start++;
        end--;
    }
}

// Reversión inteligente: reversar el segmento más corto para minimizar operaciones
inline void smart_reverse_segment(std::vector<Point>& tour, size_t i, size_t j) {
    size_t n = tour.size();
    
    // Asegurar que i < j
    if (i > j) std::swap(i, j);
    
    // Calcular las dos distancias posibles de reversión
    size_t direct_length = j - i + 1;
    size_t wrap_length = n - direct_length;
    
    // Elegir la reversión más corta
    if (direct_length <= wrap_length) {
        // Reversión directa del segmento [i, j]
        reverse_segment(tour, i, j);
    } else {
        // Reversión wrap-around: reversar todo excepto [i, j]
        // Esto es equivalente a reversar [j+1, i-1] con wrap-around
        std::reverse(tour.begin(), tour.begin() + i);
        std::reverse(tour.begin() + j + 1, tour.end());
        std::reverse(tour.begin(), tour.end());
    }
}

// Realiza un swap 2-opt en el tour usando reversión inteligente
inline void perform_2opt_swap(std::vector<Point>& tour, size_t i, size_t j) {
    // Asegurarse de que i < j
    if (i > j) std::swap(i, j);
    
    // Usar reversión inteligente para minimizar operaciones
    smart_reverse_segment(tour, i + 1, j);
}

// Calcula la ganancia de un swap 2-opt sin modificar el tour
inline double calculate_2opt_gain(const std::vector<Point>& tour, size_t i, size_t j) {
    size_t n = tour.size();
    
    // Asegurar que i < j
    if (i > j) std::swap(i, j);
    
    // Validar índices
    if (j <= i + 1 || (i == 0 && j == n - 1)) return 0.0;
    
    // Calcular aristas actuales y nuevas
    size_t i_next = (i + 1) % n;
    size_t j_next = (j + 1) % n;
    
    // Distancias actuales
    double old_dist = distance(tour[i], tour[i_next]) + distance(tour[j], tour[j_next]);
    
    // Distancias nuevas después del swap
    double new_dist = distance(tour[i], tour[j]) + distance(tour[i_next], tour[j_next]);
    
    return old_dist - new_dist;
}

// Versión más eficiente usando distancias al cuadrado para comparaciones
inline double calculate_2opt_gain_fast(const std::vector<Point>& tour, size_t i, size_t j) {
    size_t n = tour.size();
    
    // Asegurar que i < j
    if (i > j) std::swap(i, j);
    
    // Validar índices
    if (j <= i + 1 || (i == 0 && j == n - 1)) return 0.0;
    
    // Calcular aristas actuales y nuevas usando distancias al cuadrado
    size_t i_next = (i + 1) % n;
    size_t j_next = (j + 1) % n;
    
    // Distancias al cuadrado actuales
    double old_dist_sq = distance_squared(tour[i], tour[i_next]) + distance_squared(tour[j], tour[j_next]);
    
    // Distancias al cuadrado nuevas después del swap
    double new_dist_sq = distance_squared(tour[i], tour[j]) + distance_squared(tour[i_next], tour[j_next]);
    
    return old_dist_sq - new_dist_sq;
}

// Encuentra el mejor swap 2-opt en un rango de puntos
inline std::pair<size_t, size_t> find_best_2opt_swap(
    const std::vector<Point>& tour,
    size_t start,
    size_t end,
    double min_gain = 0.0) {
    
    double best_gain = min_gain;
    std::pair<size_t, size_t> best_swap = {0, 0};
    
    for (size_t i = start; i < end; ++i) {
        for (size_t j = i + 2; j < end; ++j) {
            if (j == tour.size() - 1 && i == 0) continue;
            
            double gain = calculate_2opt_gain(tour, i, j);
            if (gain > best_gain) {
                best_gain = gain;
                best_swap = {i, j};
            }
        }
    }
    
    return best_swap;
}

// Calcula todas las mejoras posibles en un tour (para análisis)
inline std::vector<std::tuple<size_t, size_t, double>> find_all_improvements(const std::vector<Point>& tour) {
    std::vector<std::tuple<size_t, size_t, double>> improvements;
    
    for (size_t i = 0; i < tour.size() - 2; ++i) {
        for (size_t j = i + 2; j < tour.size(); ++j) {
            if (j == tour.size() - 1 && i == 0) continue;
            
            double gain = calculate_2opt_gain(tour, i, j);
            if (gain > 1e-9) { // Solo mejoras significativas
                improvements.emplace_back(i, j, gain);
            }
        }
    }
    
    // Ordenar por ganancia descendente
    std::sort(improvements.begin(), improvements.end(),
              [](const auto& a, const auto& b) {
                  return std::get<2>(a) > std::get<2>(b);
              });
    
    return improvements;
}

// Verifica si un tour es válido (todos los puntos únicos, cíclico)
inline bool is_valid_tour(const std::vector<Point>& tour, const std::vector<Point>& original_points) {
    if (tour.size() != original_points.size()) return false;
    
    // Verificar que todos los puntos estén presentes
    std::unordered_map<size_t, bool> seen;
    for (const auto& point : tour) {
        if (seen[point.id]) return false; // Punto duplicado
        seen[point.id] = true;
    }
    
    // Verificar que todos los puntos originales estén en el tour
    for (const auto& point : original_points) {
        if (!seen[point.id]) return false;
    }
    
    return true;
}

// Calcula la mejora de calidad del tour
inline double tour_improvement_ratio(double initial_length, double final_length) {
    return (initial_length - final_length) / initial_length;
}

// Encuentra los segmentos más prometedores para optimización
inline std::vector<std::pair<size_t, size_t>> find_promising_segments(
    const std::vector<Point>& tour, 
    size_t segment_size = 10,
    size_t max_segments = 5) {
    
    std::vector<std::pair<size_t, size_t>> segments;
    size_t n = tour.size();
    
    // Dividir el tour en segmentos y evaluar la "promesa" de cada uno
    std::vector<std::pair<double, std::pair<size_t, size_t>>> segment_promises;
    
    for (size_t start = 0; start < n; start += segment_size) {
        size_t end = std::min(start + segment_size, n);
        
        // Calcular una métrica de "promesa" basada en la longitud local
        double segment_length = 0.0;
        for (size_t i = start; i < end; ++i) {
            segment_length += distance(tour[i], tour[(i + 1) % n]);
        }
        
        double promise = segment_length / (end - start); // Longitud promedio
        segment_promises.emplace_back(promise, std::make_pair(start, end));
    }
    
    // Ordenar por promesa descendente
    std::sort(segment_promises.begin(), segment_promises.end(),
              [](const auto& a, const auto& b) {
                  return a.first > b.first;
              });
    
    // Tomar los segmentos más prometedores
    for (size_t i = 0; i < std::min(max_segments, segment_promises.size()); ++i) {
        segments.push_back(segment_promises[i].second);
    }
    
    return segments;
} 