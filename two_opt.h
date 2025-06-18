#pragma once
#include "point.h"
#include "kd_tree.h"
#include "tour_utils.h"
#include <vector>
#include <chrono>
#include <unordered_set>
#include <iostream>
#include <iomanip>
#include <algorithm>
#include <random>

struct OptimizationStats {
    double initial_length;
    double final_length;
    size_t num_swaps;
    size_t num_visited;          // Nodos visitados en K-d tree
    size_t total_comparisons;    // Comparaciones totales realizadas
    double cpu_time;
    size_t iterations;
    size_t active_nodes;         // Para versión aproximada
    
    OptimizationStats() : initial_length(0), final_length(0), num_swaps(0), 
                         num_visited(0), total_comparisons(0), cpu_time(0), 
                         iterations(0), active_nodes(0) {}
    
    void print_detailed_stats(const std::string& algorithm_name) const {
        std::cout << "\n#stat " << algorithm_name << " Results:\n";
        std::cout << "#stat Initial Tour Length: " << std::fixed << std::setprecision(6) << initial_length << "\n";
        std::cout << "#stat Final Tour Length: " << final_length << "\n";
        std::cout << "#stat Improvement: " << std::setprecision(2) 
                  << (initial_length - final_length) / initial_length * 100.0 << "%\n";
        std::cout << "#stat Total Swaps: " << num_swaps << "\n";
        std::cout << "#stat Total Iterations: " << iterations << "\n";
        std::cout << "#stat KD-Tree Nodes Visited: " << num_visited << "\n";
        std::cout << "#stat Total Comparisons: " << total_comparisons << "\n";
        std::cout << "#stat CPU Time: " << std::setprecision(4) << cpu_time << " seconds\n";
        if (active_nodes > 0) {
            std::cout << "#stat Active Nodes (Approx): " << active_nodes << "\n";
        }
        std::cout << "#stat Swaps per Second: " << std::setprecision(2) 
                  << (cpu_time > 0 ? num_swaps / cpu_time : 0) << "\n";
        std::cout << "#stat Length Reduction: " << std::setprecision(6) 
                  << (initial_length - final_length) << "\n";
    }
};

// =============== ALGORITMO 2-OPT BÁSICO ===============
inline OptimizationStats basic_2opt(std::vector<Point>& tour) {
    OptimizationStats stats;
    stats.initial_length = tour_length(tour);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    bool improved = true;
    const size_t max_iterations = 1000;
    const double min_improvement = 1e-9;
    
    while (improved && stats.iterations < max_iterations) {
        improved = false;
        stats.iterations++;
        
        double best_gain = min_improvement;
        size_t best_i = 0, best_j = 0;
        
        // Búsqueda exhaustiva del mejor swap
        for (size_t i = 0; i < tour.size() - 2; ++i) {
            for (size_t j = i + 2; j < tour.size(); ++j) {
                if (j == tour.size() - 1 && i == 0) continue;
                
                double gain = calculate_2opt_gain(tour, i, j);
                stats.total_comparisons++;
                
                if (gain > best_gain) {
                    best_gain = gain;
                    best_i = i;
                    best_j = j;
                }
            }
        }
        
        // Aplicar el mejor swap encontrado
        if (best_gain > min_improvement) {
            perform_2opt_swap(tour, best_i, best_j);
            stats.num_swaps++;
            improved = true;
        }
        
        if (stats.iterations % 100 == 0) {
            std::cout << "\rBasic 2-Opt: Iter " << stats.iterations 
                      << ", Swaps: " << stats.num_swaps 
                      << ", Length: " << std::fixed << std::setprecision(2) 
                      << tour_length(tour) << std::flush;
        }
    }
    std::cout << std::endl;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    stats.cpu_time = std::chrono::duration<double>(end_time - start_time).count();
    stats.final_length = tour_length(tour);
    
    return stats;
}

// =============== ALGORITMO 2-OPT GEOMÉTRICO CON K-D TREE ===============
inline OptimizationStats geometric_2opt(std::vector<Point>& tour) {
    OptimizationStats stats;
    stats.initial_length = tour_length(tour);
    
    // Construir K-d tree
    KDTree kdtree;
    kdtree.build(tour);
    
    auto start_time = std::chrono::high_resolution_clock::now();
    bool improved = true;
    const size_t max_iterations = 1000;
    const double min_improvement = 1e-9;
    
    while (improved && stats.iterations < max_iterations) {
        improved = false;
        stats.iterations++;
        
        double best_gain = min_improvement;
        size_t best_i = 0, best_j = 0;
        kdtree.reset_nodes_visited();
        
        for (size_t i = 0; i < tour.size() - 2; ++i) {
            // Radio dinámico más grande - usar promedio de distancias de aristas cercanas
            double edge_dist = distance(tour[i], tour[(i + 1) % tour.size()]);
            double prev_edge_dist = distance(tour[(i + tour.size() - 1) % tour.size()], tour[i]);
            double avg_edge_dist = (edge_dist + prev_edge_dist) / 2.0;
            
            // Radio más generoso para encontrar más vecinos
            double radius = std::max(avg_edge_dist * 3.0, 0.1); // Mínimo 0.1 para asegurar vecinos
            
            // FRNN: Fixed-Radius Near Neighbors
            auto neighbors = kdtree.find_neighbors(tour[i], radius);
            
            // Si encontramos muy pocos vecinos, incrementar radio
            if (neighbors.size() < 5) {
                radius *= 2.0;
                neighbors = kdtree.find_neighbors(tour[i], radius);
            }
            
            for (const auto& neighbor : neighbors) {
                // Encontrar el índice del vecino en el tour
                auto it = std::find_if(tour.begin(), tour.end(),
                    [&neighbor](const Point& p) { return p.id == neighbor.id; });
                
                if (it != tour.end()) {
                    size_t j = it - tour.begin();
                    if (j > i + 1 && !(j == tour.size() - 1 && i == 0)) {
                        double gain = calculate_2opt_gain(tour, i, j);
                        stats.total_comparisons++;
                        
                        if (gain > best_gain) {
                            best_gain = gain;
                            best_i = i;
                            best_j = j;
                        }
                    }
                }
            }
        }
        
        stats.num_visited += kdtree.get_nodes_visited();
        
        // Aplicar el mejor swap encontrado
        if (best_gain > min_improvement) {
            perform_2opt_swap(tour, best_i, best_j);
            stats.num_swaps++;
            improved = true;
            
            // Reconstruir K-d tree después de cambios significativos
            if (stats.num_swaps % 25 == 0) {  // Más frecuente
                kdtree.build(tour);
            }
        }
        
        if (stats.iterations % 100 == 0) {
            std::cout << "\rGeometric 2-Opt: Iter " << stats.iterations 
                      << ", Swaps: " << stats.num_swaps 
                      << ", Length: " << std::fixed << std::setprecision(2) 
                      << tour_length(tour) << std::flush;
        }
    }
    std::cout << std::endl;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    stats.cpu_time = std::chrono::duration<double>(end_time - start_time).count();
    stats.final_length = tour_length(tour);
    
    return stats;
}

// =============== ALGORITMO 2-OPT APROXIMADO CON BITS DE ACTIVACIÓN ===============
inline OptimizationStats approximate_2opt(std::vector<Point>& tour) {
    OptimizationStats stats;
    stats.initial_length = tour_length(tour);
    
    // Inicializar todos los puntos como activos
    for (auto& p : tour) p.active = true;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    bool improved = true;
    const size_t max_iterations = 1000;
    const double min_improvement = 1e-9;
    
    while (improved && stats.iterations < max_iterations) {
        improved = false;
        stats.iterations++;
        
        double best_gain = min_improvement;
        size_t best_i = 0, best_j = 0;
        
        // Recopilar índices de puntos activos
        std::vector<size_t> active_indices;
        for (size_t i = 0; i < tour.size(); ++i) {
            if (tour[i].active) {
                active_indices.push_back(i);
            }
        }
        stats.active_nodes = active_indices.size();
        
        // Solo evaluar swaps entre puntos activos
        for (size_t idx_i = 0; idx_i < active_indices.size(); ++idx_i) {
            size_t i = active_indices[idx_i];
            for (size_t idx_j = idx_i + 1; idx_j < active_indices.size(); ++idx_j) {
                size_t j = active_indices[idx_j];
                if (j > i + 1 && !(j == tour.size() - 1 && i == 0)) {
                    double gain = calculate_2opt_gain(tour, i, j);
                    stats.total_comparisons++;
                    
                    if (gain > best_gain) {
                        best_gain = gain;
                        best_i = i;
                        best_j = j;
                    }
                }
            }
        }
        
        // Aplicar el mejor swap y actualizar bits de activación
        if (best_gain > min_improvement) {
            perform_2opt_swap(tour, best_i, best_j);
            stats.num_swaps++;
            improved = true;
            
            // Desactivar todos los puntos
            for (auto& p : tour) p.active = false;
            
            // Activar nodos "prometedores": vecinos de los puntos swapeados
            size_t n = tour.size();
            std::unordered_set<size_t> to_activate;
            
            // Activar vecinos de los puntos involucrados en el swap
            for (int offset = -2; offset <= 2; ++offset) {
                to_activate.insert((best_i + n + offset) % n);
                to_activate.insert((best_j + n + offset) % n);
            }
            
            // Aplicar activación
            for (size_t idx : to_activate) {
                tour[idx].active = true;
            }
        } else {
            // Si no hay mejoras, activar más nodos gradualmente
            size_t to_activate_count = std::min(tour.size(), active_indices.size() + 10);
            
            // Activar nodos aleatorios adicionales
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<size_t> dist(0, tour.size() - 1);
            
            for (size_t i = active_indices.size(); i < to_activate_count; ++i) {
                tour[dist(gen)].active = true;
            }
        }
        
        if (stats.iterations % 100 == 0) {
            std::cout << "\rApproximate 2-Opt: Iter " << stats.iterations 
                      << ", Swaps: " << stats.num_swaps 
                      << ", Active: " << stats.active_nodes
                      << ", Length: " << std::fixed << std::setprecision(2) 
                      << tour_length(tour) << std::flush;
        }
    }
    std::cout << std::endl;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    stats.cpu_time = std::chrono::duration<double>(end_time - start_time).count();
    stats.final_length = tour_length(tour);
    
    return stats;
}

// =============== ALGORITMO 2-OPT HÍBRIDO (COMBINACIÓN DE TÉCNICAS) ===============
inline OptimizationStats hybrid_2opt(std::vector<Point>& tour) {
    OptimizationStats stats;
    stats.initial_length = tour_length(tour);
    
    KDTree kdtree;
    kdtree.build(tour);
    
    // Inicializar bits de activación
    for (auto& p : tour) p.active = true;
    
    auto start_time = std::chrono::high_resolution_clock::now();
    bool improved = true;
    const size_t max_iterations = 1000;
    const double min_improvement = 1e-9;
    
    while (improved && stats.iterations < max_iterations) {
        improved = false;
        stats.iterations++;
        
        double best_gain = min_improvement;
        size_t best_i = 0, best_j = 0;
        kdtree.reset_nodes_visited();
        
        // Obtener puntos activos
        std::vector<size_t> active_indices;
        for (size_t i = 0; i < tour.size(); ++i) {
            if (tour[i].active) {
                active_indices.push_back(i);
            }
        }
        stats.active_nodes = active_indices.size();
        
        // Usar K-d tree solo en puntos activos
        for (size_t i : active_indices) {
            if (i >= tour.size() - 2) continue;
            
            // Radio dinámico adaptativo más generoso
            double edge_dist = distance(tour[i], tour[(i + 1) % tour.size()]);
            double radius = std::max(edge_dist * 4.0, 0.15); // Factor más grande y mínimo mayor
            
            auto neighbors = kdtree.find_neighbors_adaptive(tour[i], radius, 8); // Más vecinos mínimos
            
            for (const auto& neighbor : neighbors) {
                auto it = std::find_if(tour.begin(), tour.end(),
                    [&neighbor](const Point& p) { return p.id == neighbor.id; });
                
                if (it != tour.end()) {
                    size_t j = it - tour.begin();
                    if (j > i + 1 && !(j == tour.size() - 1 && i == 0) && tour[j].active) {
                        double gain = calculate_2opt_gain_fast(tour, i, j);
                        stats.total_comparisons++;
                        
                        if (gain > best_gain) {
                            best_gain = gain;
                            best_i = i;
                            best_j = j;
                        }
                    }
                }
            }
        }
        
        stats.num_visited += kdtree.get_nodes_visited();
        
        if (best_gain > min_improvement) {
            perform_2opt_swap(tour, best_i, best_j);
            stats.num_swaps++;
            improved = true;
            
            // Actualizar activación de manera inteligente
            for (auto& p : tour) p.active = false;
            
            size_t n = tour.size();
            for (int offset = -4; offset <= 4; ++offset) { // Vecindario más grande
                tour[(best_i + n + offset) % n].active = true;
                tour[(best_j + n + offset) % n].active = true;
            }
            
            // Reconstruir árbol periódicamente
            if (stats.num_swaps % 30 == 0) {
                kdtree.build(tour);
            }
        } else {
            // Reactivar más nodos si no hay mejoras - estrategia más agresiva
            size_t nodes_to_activate = std::min(tour.size(), std::max(active_indices.size() + 15, tour.size() / 4));
            
            for (auto& p : tour) p.active = false;
            for (size_t i = 0; i < nodes_to_activate; i += 2) {
                if (i < tour.size()) tour[i].active = true;
            }
        }
        
        if (stats.iterations % 100 == 0) {
            std::cout << "\rHybrid 2-Opt: Iter " << stats.iterations 
                      << ", Swaps: " << stats.num_swaps 
                      << ", Active: " << stats.active_nodes
                      << ", Length: " << std::fixed << std::setprecision(2) 
                      << tour_length(tour) << std::flush;
        }
    }
    std::cout << std::endl;
    
    auto end_time = std::chrono::high_resolution_clock::now();
    stats.cpu_time = std::chrono::duration<double>(end_time - start_time).count();
    stats.final_length = tour_length(tour);
    
    return stats;
} 