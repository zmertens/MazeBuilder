#ifndef DISTANCES_H
#define DISTANCES_H

#include <unordered_map>
#include <vector>
#include <algorithm>
#include <memory>

namespace mazes {

    //class Node {
    //public:
    //    int index;
    //    std::vector<std::shared_ptr<Node>> links;

    //    Node(int idx) : index(idx) {}
    //};

    //class Distances {
    //private:
    //    std::shared_ptr<Node> root;
    //    std::unordered_map<std::shared_ptr<Node>, int> cells;

    //public:
    //    Distances(std::shared_ptr<Node> root) : root(root) {
    //        cells[root] = 0;
    //    }

    //    int operator[](std::shared_ptr<Node> cell) const {
    //        auto it = cells.find(cell);
    //        if (it != cells.end()) {
    //            return it->second;
    //        }
    //        return -1; // or some other default value indicating not found
    //    }

    //    void operator[](std::shared_ptr<Node> cell, int distance) {
    //        cells[cell] = distance;
    //    }

    //    std::vector<std::shared_ptr<Node>> getCells() const {
    //        std::vector<std::shared_ptr<Node>> keys;
    //        for (const auto& pair : cells) {
    //            keys.push_back(pair.first);
    //        }
    //        return keys;
    //    }

    //    std::shared_ptr<Distances> pathTo(std::shared_ptr<Node> goal) {
    //        auto current = goal;
    //        auto breadcrumbs = std::make_shared<Distances>(root);

    //        (*breadcrumbs)[current] = cells[current];

    //        while (current != root) {
    //            for (const auto& neighbor : current->links) {
    //                if (cells[neighbor] < cells[current]) {
    //                    (*breadcrumbs)[neighbor] = cells[neighbor];
    //                    current = neighbor;
    //                    break;
    //                }
    //            }
    //        }
    //        return breadcrumbs;
    //    }

    //    std::pair<std::shared_ptr<Node>, int> max() const {
    //        int max_distance = 0;
    //        std::shared_ptr<Node> max_cell = root;
    //        for (const auto& pair : cells) {
    //            if (pair.second > max_distance) {
    //                max_cell = pair.first;
    //                max_distance = pair.second;
    //            }
    //        }
    //        return { max_cell, max_distance };
    //    }
    //};

} // namespace mazes
#endif // DISTANCES_H