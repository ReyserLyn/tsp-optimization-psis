#pragma once
#include <cmath>
#include <vector>
#include <random>
#include <algorithm>
#include <limits>

struct Point {
    double x, y;
    bool active;  // Para el 2-opt aproximado
    size_t id;    // Identificador único para tracking
    
    Point(double x = 0, double y = 0, size_t id = 0) : x(x), y(y), active(true), id(id) {}
    
    // Operadores para comparación
    bool operator==(const Point& other) const {
        return x == other.x && y == other.y;
    }
    
    bool operator!=(const Point& other) const {
        return !(*this == other);
    }
    
    // Operador < necesario para std::pair con Point
    bool operator<(const Point& other) const {
        if (x != other.x) return x < other.x;
        return y < other.y;
    }
};

// Distancia euclidiana
inline double distance(const Point& a, const Point& b) {
    return std::sqrt((a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y));
}

// Distancia euclidiana cuadrada (más eficiente para comparaciones)
inline double distance_squared(const Point& a, const Point& b) {
    return (a.x - b.x)*(a.x - b.x) + (a.y - b.y)*(a.y - b.y);
}

// Genera puntos aleatorios en [0,1]x[0,1]
inline std::vector<Point> generate_random_points(size_t n, unsigned int seed = 42) {
    std::vector<Point> points;
    points.reserve(n);
    
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> dist(0.0, 1.0);
    
    for (size_t i = 0; i < n; ++i) {
        points.emplace_back(dist(gen), dist(gen), i);
    }
    return points;
}

// Genera puntos en cluster (más realista para TSP)
inline std::vector<Point> generate_clustered_points(size_t n, size_t num_clusters = 5, unsigned int seed = 42) {
    std::vector<Point> points;
    points.reserve(n);
    
    std::mt19937 gen(seed);
    std::uniform_real_distribution<double> cluster_center(0.1, 0.9);
    std::normal_distribution<double> cluster_point(0.0, 0.05);
    std::uniform_int_distribution<size_t> cluster_selector(0, num_clusters - 1);
    
    // Generar centros de clusters
    std::vector<std::pair<double, double>> cluster_centers;
    for (size_t i = 0; i < num_clusters; ++i) {
        cluster_centers.emplace_back(cluster_center(gen), cluster_center(gen));
    }
    
    for (size_t i = 0; i < n; ++i) {
        // Seleccionar cluster aleatorio
        size_t selected_cluster = cluster_selector(gen);
        double cx = cluster_centers[selected_cluster].first;
        double cy = cluster_centers[selected_cluster].second;
        
        // Generar punto en el cluster
        double x = std::max(0.0, std::min(1.0, cx + cluster_point(gen)));
        double y = std::max(0.0, std::min(1.0, cy + cluster_point(gen)));
        
        points.emplace_back(x, y, i);
    }
    return points;
}

// Calcula la longitud total de un tour
inline double tour_length(const std::vector<Point>& tour) {
    if (tour.size() < 2) return 0.0;
    
    double length = 0.0;
    for (size_t i = 0; i < tour.size(); ++i) {
        length += distance(tour[i], tour[(i + 1) % tour.size()]);
    }
    return length;
}

// Heurística Nearest Neighbor para inicialización del tour
inline std::vector<Point> nearest_neighbor_tour(const std::vector<Point>& points, size_t start_idx = 0) {
    if (points.empty()) return {};
    
    std::vector<Point> tour;
    std::vector<bool> visited(points.size(), false);
    tour.reserve(points.size());
    
    // Comenzar desde el punto especificado
    size_t current = start_idx;
    tour.push_back(points[current]);
    visited[current] = true;
    
    // Construir el tour
    for (size_t step = 1; step < points.size(); ++step) {
        double min_dist = std::numeric_limits<double>::max();
        size_t next = current;
        
        // Encontrar el punto más cercano no visitado
        for (size_t i = 0; i < points.size(); ++i) {
            if (!visited[i]) {
                double dist = distance(points[current], points[i]);
                if (dist < min_dist) {
                    min_dist = dist;
                    next = i;
                }
            }
        }
        
        tour.push_back(points[next]);
        visited[next] = true;
        current = next;
    }
    
    return tour;
}

// Genera múltiples tours NN desde diferentes puntos de inicio y retorna el mejor
inline std::vector<Point> best_nearest_neighbor_tour(const std::vector<Point>& points, size_t num_starts = 10) {
    if (points.empty()) return {};
    
    std::vector<Point> best_tour;
    double best_length = std::numeric_limits<double>::max();
    
    // Probar diferentes puntos de inicio
    for (size_t start = 0; start < std::min(num_starts, points.size()); ++start) {
        auto tour = nearest_neighbor_tour(points, start);
        double length = tour_length(tour);
        
        if (length < best_length) {
            best_length = length;
            best_tour = tour;
        }
    }
    
    return best_tour;
} 