#pragma once
#include "point.h"
#include <vector>
#include <memory>
#include <queue>
#include <limits>
#include <unordered_map>

class KDNode {
public:
    Point point;
    std::unique_ptr<KDNode> left;
    std::unique_ptr<KDNode> right;
    int depth;
    
    KDNode(const Point& p, int d) : point(p), depth(d) {}
};

class KDTree {
private:
    std::unique_ptr<KDNode> root;
    size_t size_;
    mutable size_t nodes_visited; // Para métricas
    
    std::unique_ptr<KDNode> build(std::vector<Point>& points, int depth, size_t start, size_t end) {
        if (start >= end) return nullptr;
        
        size_t mid = (start + end) / 2;
        bool axis = depth % 2 == 0; // true para x, false para y
        
        // Ordenar puntos según el eje actual
        std::nth_element(points.begin() + start, points.begin() + mid, points.begin() + end,
            [axis](const Point& a, const Point& b) {
                return axis ? a.x < b.x : a.y < b.y;
            });
        
        auto node = std::make_unique<KDNode>(points[mid], depth);
        node->left = build(points, depth + 1, start, mid);
        node->right = build(points, depth + 1, mid + 1, end);
        
        return node;
    }
    
    // FRNN optimizado con radio dinámico
    void find_neighbors_frnn(const KDNode* node, const Point& query, double radius,
                            std::vector<Point>& neighbors) const {
        if (!node) return;
        
        nodes_visited++;
        
        // Verificar si el nodo actual está dentro del radio
        double dist_sq = distance_squared(node->point, query);
        if (dist_sq <= radius * radius) {
            neighbors.push_back(node->point);
        }
        
        // Determinar qué hijo explorar primero
        bool axis = node->depth % 2 == 0;
        double diff = axis ? query.x - node->point.x : query.y - node->point.y;
        
        // Explorar el lado más probable primero
        if (diff <= 0) {
            find_neighbors_frnn(node->left.get(), query, radius, neighbors);
            if (diff * diff <= radius * radius) {
                find_neighbors_frnn(node->right.get(), query, radius, neighbors);
            }
        } else {
            find_neighbors_frnn(node->right.get(), query, radius, neighbors);
            if (diff * diff <= radius * radius) {
                find_neighbors_frnn(node->left.get(), query, radius, neighbors);
            }
        }
    }
    
    // Búsqueda del vecino más cercano (para heurística NN)
    void find_nearest(const KDNode* node, const Point& query, Point& best, double& best_dist_sq) const {
        if (!node) return;
        
        nodes_visited++;
        
        double dist_sq = distance_squared(node->point, query);
        if (dist_sq < best_dist_sq) {
            best_dist_sq = dist_sq;
            best = node->point;
        }
        
        bool axis = node->depth % 2 == 0;
        double diff = axis ? query.x - node->point.x : query.y - node->point.y;
        
        // Explorar el lado más probable primero
        if (diff <= 0) {
            find_nearest(node->left.get(), query, best, best_dist_sq);
            if (diff * diff < best_dist_sq) {
                find_nearest(node->right.get(), query, best, best_dist_sq);
            }
        } else {
            find_nearest(node->right.get(), query, best, best_dist_sq);
            if (diff * diff < best_dist_sq) {
                find_nearest(node->left.get(), query, best, best_dist_sq);
            }
        }
    }
    
    // K vecinos más cercanos
    void find_k_nearest(const KDNode* node, const Point& query, size_t k,
                       std::priority_queue<std::pair<double, Point>>& best_k) const {
        if (!node) return;
        
        nodes_visited++;
        
        double dist_sq = distance_squared(node->point, query);
        
        if (best_k.size() < k) {
            best_k.push({dist_sq, node->point});
        } else if (dist_sq < best_k.top().first) {
            best_k.pop();
            best_k.push({dist_sq, node->point});
        }
        
        bool axis = node->depth % 2 == 0;
        double diff = axis ? query.x - node->point.x : query.y - node->point.y;
        
        double worst_dist = best_k.size() < k ? std::numeric_limits<double>::max() : best_k.top().first;
        
        // Explorar el lado más probable primero
        if (diff <= 0) {
            find_k_nearest(node->left.get(), query, k, best_k);
            worst_dist = best_k.size() < k ? std::numeric_limits<double>::max() : best_k.top().first;
            if (diff * diff < worst_dist) {
                find_k_nearest(node->right.get(), query, k, best_k);
            }
        } else {
            find_k_nearest(node->right.get(), query, k, best_k);
            worst_dist = best_k.size() < k ? std::numeric_limits<double>::max() : best_k.top().first;
            if (diff * diff < worst_dist) {
                find_k_nearest(node->left.get(), query, k, best_k);
            }
        }
    }

public:
    KDTree() : size_(0), nodes_visited(0) {}
    
    void build(const std::vector<Point>& points) {
        if (points.empty()) return;
        
        std::vector<Point> points_copy = points;
        root = build(points_copy, 0, 0, points.size());
        size_ = points.size();
        nodes_visited = 0;
    }
    
    // FRNN con radio fijo
    std::vector<Point> find_neighbors(const Point& query, double radius) const {
        std::vector<Point> neighbors;
        nodes_visited = 0;
        find_neighbors_frnn(root.get(), query, radius, neighbors);
        return neighbors;
    }
    
    // Encuentra el vecino más cercano
    Point find_nearest_neighbor(const Point& query) const {
        if (!root) return Point();
        
        Point best = root->point;
        double best_dist_sq = distance_squared(query, best);
        nodes_visited = 0;
        
        find_nearest(root.get(), query, best, best_dist_sq);
        return best;
    }
    
    // Encuentra los k vecinos más cercanos
    std::vector<Point> find_k_nearest_neighbors(const Point& query, size_t k) const {
        std::priority_queue<std::pair<double, Point>> best_k;
        nodes_visited = 0;
        
        find_k_nearest(root.get(), query, k, best_k);
        
        std::vector<Point> result;
        while (!best_k.empty()) {
            result.push_back(best_k.top().second);
            best_k.pop();
        }
        
        std::reverse(result.begin(), result.end()); // Orden de más cercano a más lejano
        return result;
    }
    
    // FRNN adaptativo: ajusta el radio según la densidad local
    std::vector<Point> find_neighbors_adaptive(const Point& query, double base_radius, size_t min_neighbors = 5) const {
        double radius = base_radius;
        std::vector<Point> neighbors;
        
        // Incrementar radio hasta encontrar suficientes vecinos
        while (neighbors.size() < min_neighbors && radius < 2.0) {
            neighbors.clear();
            nodes_visited = 0;
            find_neighbors_frnn(root.get(), query, radius, neighbors);
            if (neighbors.size() < min_neighbors) {
                radius *= 1.5; // Incrementar radio
            }
        }
        
        return neighbors;
    }
    
    size_t size() const { return size_; }
    size_t get_nodes_visited() const { return nodes_visited; }
    void reset_nodes_visited() const { nodes_visited = 0; }
}; 