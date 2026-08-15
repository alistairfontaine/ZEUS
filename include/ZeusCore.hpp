#ifndef ZEUS_CORE_HPP
#define ZEUS_CORE_HPP

#include <vector>
#include <string>
#include <cstdint>
#include <cmath>
#include <stdexcept>
#include <iostream>

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

        size_t get_active_nodes_count() const { return clusterNodes.size(); }
        void clear_routing_grid() { clusterNodes.clear(); }
    };
}

#endif // ZEUS_CORE_HPP
