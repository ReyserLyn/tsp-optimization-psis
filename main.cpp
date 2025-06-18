#include "point.h"
#include "two_opt.h"
#include <iostream>
#include <iomanip>
#include <vector>
#include <algorithm>
#include <random>
#include <fstream>
#include <sstream>
#include <numeric>

// Función para imprimir un separador elegante
void print_separator(const std::string& title = "") {
    std::cout << "\n" << std::string(70, '=') << "\n";
    if (!title.empty()) {
        int padding = (70 - title.length()) / 2;
        std::cout << std::string(padding, ' ') << title << std::endl;
        std::cout << std::string(70, '=') << "\n";
    }
}

// Función para mostrar información de la instancia
void print_instance_info(const std::vector<Point>& points, const std::vector<Point>& tour) {
    std::cout << "Información de la Instancia TSP:\n";
    std::cout << "- Número de puntos: " << points.size() << "\n";
    std::cout << "- Longitud inicial (tour NN): " << std::fixed << std::setprecision(6) 
              << tour_length(tour) << "\n";
    
    // Calcular estadísticas de distancias
    std::vector<double> distances;
    for (size_t i = 0; i < points.size(); ++i) {
        for (size_t j = i + 1; j < points.size(); ++j) {
            distances.push_back(distance(points[i], points[j]));
        }
    }
    
    std::sort(distances.begin(), distances.end());
    double min_dist = distances.front();
    double max_dist = distances.back();
    double avg_dist = std::accumulate(distances.begin(), distances.end(), 0.0) / distances.size();
    
    std::cout << "- Distancia mínima entre puntos: " << std::setprecision(4) << min_dist << "\n";
    std::cout << "- Distancia máxima entre puntos: " << max_dist << "\n";
    std::cout << "- Distancia promedio entre puntos: " << avg_dist << "\n";
}

// Función para ejecutar y comparar todos los algoritmos
void run_complete_benchmark(std::vector<Point>& points) {
    print_separator("OPTIMIZACIÓN TSP - ALGORITMOS 2-OPT");
    
    // Crear tour inicial usando heurística Nearest Neighbor
    std::cout << "Generando tour inicial con heurística Nearest Neighbor...\n";
    auto initial_tour = best_nearest_neighbor_tour(points, 10); // Probar 10 puntos de inicio
    
    print_instance_info(points, initial_tour);
    
    // Verificar validez del tour inicial
    if (!is_valid_tour(initial_tour, points)) {
        std::cerr << "ERROR: Tour inicial inválido!\n";
        return;
    }
    
    std::cout << "\nEjecutando optimizaciones 2-Opt...\n";
    
    // Preparar copias del tour para cada algoritmo
    auto tour_basic = initial_tour;
    auto tour_geometric = initial_tour;
    auto tour_approximate = initial_tour;
    auto tour_hybrid = initial_tour;
    
    // ================== EJECUTAR ALGORITMOS ==================
    
    print_separator("ALGORITMO 2-OPT BÁSICO");
    std::cout << "Ejecutando 2-Opt Básico (búsqueda exhaustiva)...\n";
    auto stats_basic = basic_2opt(tour_basic);
    stats_basic.print_detailed_stats("Basic 2-Opt");
    
    print_separator("ALGORITMO 2-OPT GEOMÉTRICO");
    std::cout << "Ejecutando 2-Opt Geométrico (K-d Tree + FRNN)...\n";
    auto stats_geometric = geometric_2opt(tour_geometric);
    stats_geometric.print_detailed_stats("Geometric 2-Opt");
    
    print_separator("ALGORITMO 2-OPT APROXIMADO");
    std::cout << "Ejecutando 2-Opt Aproximado (bits de activación)...\n";
    auto stats_approximate = approximate_2opt(tour_approximate);
    stats_approximate.print_detailed_stats("Approximate 2-Opt");
    
    print_separator("ALGORITMO 2-OPT HÍBRIDO");
    std::cout << "Ejecutando 2-Opt Híbrido (K-d Tree + bits de activación)...\n";
    auto stats_hybrid = hybrid_2opt(tour_hybrid);
    stats_hybrid.print_detailed_stats("Hybrid 2-Opt");
    
    // ================== ANÁLISIS COMPARATIVO ==================
    
    print_separator("ANÁLISIS COMPARATIVO");
    
    std::cout << "#comparison Table of Results:\n";
    std::cout << std::left << std::setw(15) << "Algorithm" 
              << std::setw(12) << "Final Length" 
              << std::setw(10) << "Improvement" 
              << std::setw(8) << "Swaps" 
              << std::setw(8) << "Time(s)" 
              << std::setw(12) << "Swaps/sec"
              << std::setw(12) << "Comparisons" << "\n";
    std::cout << std::string(85, '-') << "\n";
    
    auto print_row = [](const std::string& name, const OptimizationStats& stats) {
        double improvement = (stats.initial_length - stats.final_length) / stats.initial_length * 100.0;
        double swaps_per_sec = stats.cpu_time > 0 ? stats.num_swaps / stats.cpu_time : 0;
        
        std::cout << std::left << std::setw(15) << name
                  << std::setw(12) << std::fixed << std::setprecision(4) << stats.final_length
                  << std::setw(10) << std::setprecision(2) << improvement << "%"
                  << std::setw(8) << stats.num_swaps
                  << std::setw(8) << std::setprecision(3) << stats.cpu_time
                  << std::setw(12) << std::setprecision(1) << swaps_per_sec
                  << std::setw(12) << stats.total_comparisons << "\n";
    };
    
    print_row("Basic", stats_basic);
    print_row("Geometric", stats_geometric);
    print_row("Approximate", stats_approximate);
    print_row("Hybrid", stats_hybrid);
    
    // Encontrar el mejor algoritmo
    std::vector<std::pair<std::string, OptimizationStats>> all_stats = {
        {"Basic", stats_basic},
        {"Geometric", stats_geometric},
        {"Approximate", stats_approximate},
        {"Hybrid", stats_hybrid}
    };
    
    auto best = std::min_element(all_stats.begin(), all_stats.end(),
        [](const auto& a, const auto& b) {
            return a.second.final_length < b.second.final_length;
        });
    
    std::cout << "\n#best_algorithm: " << best->first 
              << " (Length: " << std::fixed << std::setprecision(6) << best->second.final_length << ")\n";
    
    // Análisis de eficiencia
    print_separator("ANÁLISIS DE EFICIENCIA");
    
    auto fastest = std::min_element(all_stats.begin(), all_stats.end(),
        [](const auto& a, const auto& b) {
            return a.second.cpu_time < b.second.cpu_time;
        });
    
    auto most_swaps = std::max_element(all_stats.begin(), all_stats.end(),
        [](const auto& a, const auto& b) {
            return a.second.num_swaps < b.second.num_swaps;
        });
    
    std::cout << "#fastest_algorithm: " << fastest->first 
              << " (" << std::setprecision(3) << fastest->second.cpu_time << "s)\n";
    std::cout << "#most_swaps: " << most_swaps->first 
              << " (" << most_swaps->second.num_swaps << " swaps)\n";
    
    // Calcular speedup del K-d tree
    if (stats_basic.cpu_time > 0 && stats_geometric.cpu_time > 0) {
        double speedup = stats_basic.cpu_time / stats_geometric.cpu_time;
        std::cout << "#geometric_speedup: " << std::setprecision(2) << speedup << "x\n";
    }
    
    // Análisis de reducción de comparaciones
    if (stats_basic.total_comparisons > 0) {
        double reduction_geo = (1.0 - double(stats_geometric.total_comparisons) / stats_basic.total_comparisons) * 100.0;
        double reduction_app = (1.0 - double(stats_approximate.total_comparisons) / stats_basic.total_comparisons) * 100.0;
        
        std::cout << "#comparison_reduction_geometric: " << std::setprecision(1) << reduction_geo << "%\n";
        std::cout << "#comparison_reduction_approximate: " << std::setprecision(1) << reduction_app << "%\n";
    }
}

// Función para guardar resultados en archivo
void save_results_to_file(const std::vector<Point>& points, const std::vector<Point>& best_tour, 
                         const std::string& filename = "tsp_results.txt") {
    std::ofstream file(filename);
    if (file.is_open()) {
        file << "TSP Optimization Results\n";
        file << "Points: " << points.size() << "\n";
        file << "Best Tour Length: " << std::fixed << std::setprecision(6) << tour_length(best_tour) << "\n";
        file << "\nBest Tour Sequence:\n";
        for (size_t i = 0; i < best_tour.size(); ++i) {
            file << i << ": (" << std::setprecision(6) << best_tour[i].x 
                 << ", " << best_tour[i].y << ") ID:" << best_tour[i].id << "\n";
        }
        file.close();
        std::cout << "\nResultados guardados en: " << filename << "\n";
    }
}

int main(int argc, char* argv[]) {
    std::cout << "=== OPTIMIZACIÓN TSP CON ALGORITMOS 2-OPT ===\n";
    std::cout << "Implementación fiel del paper de optimizaciones geométricas\n";
    
    // Configuración por defecto
    size_t n_points = 100;
    unsigned int seed = 42;
    bool use_clustered = false;
    
    // Procesar argumentos de línea de comandos
    if (argc > 1) n_points = std::stoul(argv[1]);
    if (argc > 2) seed = std::stoul(argv[2]);
    if (argc > 3) use_clustered = (std::string(argv[3]) == "clustered");
    
    std::cout << "Configuración:\n";
    std::cout << "- Número de puntos: " << n_points << "\n";
    std::cout << "- Semilla aleatoria: " << seed << "\n";
    std::cout << "- Tipo de instancia: " << (use_clustered ? "Clustered" : "Random") << "\n";
    
    // Generar instancia del problema
    std::vector<Point> points;
    if (use_clustered) {
        points = generate_clustered_points(n_points, 5, seed);
        std::cout << "Generando instancia con puntos agrupados...\n";
    } else {
        points = generate_random_points(n_points, seed);
        std::cout << "Generando instancia con puntos aleatorios...\n";
    }
    
    if (points.empty()) {
        std::cerr << "Error: No se pudieron generar puntos.\n";
        return 1;
    }
    
    // Ejecutar benchmark completo
    try {
        run_complete_benchmark(points);
        
        // Guardar el mejor resultado (usando geometric por defecto)
        auto best_tour = best_nearest_neighbor_tour(points);
        geometric_2opt(best_tour);
        save_results_to_file(points, best_tour);
        
    } catch (const std::exception& e) {
        std::cerr << "Error durante la optimización: " << e.what() << "\n";
        return 1;
    }
    
    print_separator();
    std::cout << "Optimización completada exitosamente.\n";
    std::cout << "Para ejecutar con diferentes parámetros:\n";
    std::cout << "./tsp_optimization [num_points] [seed] [random|clustered]\n";
    std::cout << "Ejemplo: ./tsp_optimization 200 123 clustered\n";
    
    return 0;
} 