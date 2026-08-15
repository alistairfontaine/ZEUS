#ifndef ZEUS_CORE_HPP
#define ZEUS_CORE_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <iostream>
#include <fstream>

namespace Zeus {

    // PI constant definition safely configured without heavy math header dependencies
    constexpr double ZEUS_PI = 3.14159265358979323846;
    constexpr double EARTH_RADIUS_KM = 6371.0;

    inline double degrees_to_radians(double deg) {
        return deg * (ZEUS_PI / 180.0);
    }

    // Cache-aligned orbital/ground node telemetry descriptor
    struct alignas(8) NetworkNode {
        uint32_t nodeId = 0;
        double latitude = 0.0;  // Coordinates stored in degrees
        double longitude = 0.0;
        double altitudeKm = 0.0; // Distance added over base sea-level metrics
        char nodeLabel[16] = "sat_node";
    };

    struct CartesianVector {
        double x = 0.0;
        double y = 0.0;
        double z = 0.0;
    };

    class MeshRouter {
    private:
        std::vector<NetworkNode> clusterNodes;

    public:
        MeshRouter() = default;

        void register_node(const NetworkNode& node) {
            clusterNodes.push_back(node);
        }

        // Convert coordinates to 3D Cartesian vectors for quick structural tracking
        CartesianVector get_cartesian_coordinates(const NetworkNode& node) const {
            double radLat = degrees_to_radians(node.latitude);
            double radLon = degrees_to_radians(node.longitude);
            double totalRadius = EARTH_RADIUS_KM + node.altitudeKm;

            CartesianVector vec;
            vec.x = totalRadius * std::cos(radLat) * std::cos(radLon);
            vec.y = totalRadius * std::cos(radLat) * std::sin(radLon);
            vec.z = totalRadius * std::sin(radLat);
            return vec;
        }

        // High-velocity Great-Circle distance tracking logic
        double calculate_link_distance(const NetworkNode& n1, const NetworkNode& n2) const {
            double rLat1 = degrees_to_radians(n1.latitude);
            double rLat2 = degrees_to_radians(n2.latitude);
            double deltaLon = degrees_to_radians(n2.longitude - n1.longitude);

            // Haversine spherical projection loop equation
            double arcScalar = std::sin(rLat1) * std::sin(rLat2) +
                               std::cos(rLat1) * std::cos(rLat2) * std::cos(deltaLon);

            // Constrain value to valid tracking ranges to prevent nan math faults
            if (arcScalar > 1.0) arcScalar = 1.0;
            if (arcScalar < -1.0) arcScalar = -1.0;

            double baseCentralAngle = std::acos(arcScalar);
            double averageRadius = EARTH_RADIUS_KM + ((n1.altitudeKm + n2.altitudeKm) / 2.0);

            return averageRadius * baseCentralAngle;
        }

        // Verify if a laser or radio handshake vector link is viable
        bool verify_link_viability(const NetworkNode& n1, const NetworkNode& n2, double maxRangeKm) const {
            double actualDistance = calculate_link_distance(n1, n2);
            return actualDistance <= maxRangeKm;
        }

        // --- PHASE 2: MATRIX-BASED GRAPH PATH SEARCH ROUTINE ---
        // Dynamically identifies hops across intermediate nodes using range limitations
        int calculate_shortest_path_hops(uint32_t srcId, uint32_t destId, double maxRangeKm) const {
            if (clusterNodes.empty()) return -1;
            if (srcId == destId) return 0;

            size_t n = clusterNodes.size();
            std::vector<int> distances(n, 1e9); // Infinite baseline tracking initialization
            std::vector<bool> visited(n, false);

            int srcIdx = -1, destIdx = -1;
            for (size_t i = 0; i < n; ++i) {
                if (clusterNodes[i].nodeId == srcId) srcIdx = i;
                if (clusterNodes[i].nodeId == destId) destIdx = i;
            }

            if (srcIdx == -1 || destIdx == -1) return -1; // Target nodes missing from active array

            distances[srcIdx] = 0;

            for (size_t count = 0; count < n - 1; ++count) {
                int u = -1;
                int minDist = 1e9;

                for (size_t i = 0; i < n; ++i) {
                    if (!visited[i] && distances[i] < minDist) {
                        minDist = distances[i];
                        u = i;
                    }
                }

                if (u == -1 || u == destIdx) break;
                visited[u] = true;

                for (size_t v = 0; v < n; ++v) {
                    if (!visited[v]) {
                        double distBetween = calculate_link_distance(clusterNodes[u], clusterNodes[v]);
                        if (distBetween <= maxRangeKm && distances[u] + 1 < distances[v]) {
                            distances[v] = distances[u] + 1;
                        }
                    }
                }
            }

            return distances[destIdx] == 1e9 ? -1 : distances[destIdx];
        }

        // --- PHASE 2: CUSTOM .ZEUS BINARY STATE SERIALIZATION EXPORTER ---
        void export_to_binary(const std::string& filename) const {
            std::ofstream out(filename, std::ios::binary);
            if (!out.is_open()) {
                throw std::runtime_error("Failed to open file for binary mesh target output: " + filename);
            }

            const uint32_t MAGIC_HEADER = 0x5A455553; // "ZEUS" in hex ASCII
            size_t nodeCount = clusterNodes.size();

            out.write(reinterpret_cast<const char*>(&MAGIC_HEADER), sizeof(MAGIC_HEADER));
            out.write(reinterpret_cast<const char*>(&nodeCount), sizeof(nodeCount));

            if (nodeCount > 0) {
                out.write(reinterpret_cast<const char*>(clusterNodes.data()), nodeCount * sizeof(NetworkNode));
            }
            std::cout << "[Exporter] Successfully saved planetary routing infrastructure map to: " << filename << "\n";
        }

        // --- PHASE 2: CUSTOM .ZEUS BINARY STATE DESCRIPTOR LOADER ---
        void load_from_binary(const std::string& filename) {
            std::ifstream in(filename, std::ios::binary);
            if (!in.is_open()) {
                throw std::runtime_error("Failed to open file for binary reading: " + filename);
            }

            const uint32_t MAGIC_HEADER = 0x5A455553;
            uint32_t headerCheck = 0;
            in.read(reinterpret_cast<char*>(&headerCheck), sizeof(headerCheck));

            if (headerCheck != MAGIC_HEADER) {
                throw std::runtime_error("Invalid or corrupted .zeus binary telemetry signature detected.");
            }

            size_t nodeCount = 0;
            in.read(reinterpret_cast<char*>(&nodeCount), sizeof(nodeCount));

            clusterNodes.resize(nodeCount);
            if (nodeCount > 0) {
                in.read(reinterpret_cast<char*>(clusterNodes.data()), nodeCount * sizeof(NetworkNode));
            }
            std::cout << "[Loader] Successfully restored planetary mesh routing node array view.\n";
        }

        size_t get_active_nodes_count() const { return clusterNodes.size(); }
        void clear_routing_grid() { clusterNodes.clear(); }

    };
}

#endif // ZEUS_CORE_HPP
