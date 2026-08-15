#include "../include/ZeusCore.hpp"
#include <iostream>
#include <cstdio>

int main() {
    std::cout << "ZEUS Autonomous Spherical Network Router Initiated\n";
    std::cout << "---------------------------------------------------\n";

    Zeus::MeshRouter router;

    // 1. Configure baseline ground terminal transceiver node location parameters
    Zeus::NetworkNode harareTerminal;
    harareTerminal.nodeId = 1001;
    harareTerminal.latitude = -17.8252; // Coordinates mapping Harare
    harareTerminal.longitude = 31.0335;
    harareTerminal.altitudeKm = 1.49;   // Average elevation above sea level
    std::snprintf(harareTerminal.nodeLabel, sizeof(harareTerminal.nodeLabel), "hre_terminal");

    // 2. Configure a Low-Earth-Orbit satellite router node path pass directly overhead
    Zeus::NetworkNode telemetrySat;
    telemetrySat.nodeId = 5001;
    telemetrySat.latitude = -17.5000;   // Sled pass offset close to target station coordinates
    telemetrySat.longitude = 31.2000;
    telemetrySat.altitudeKm = 550.00;   // Low Earth Orbit cruising altitude bounds
    std::snprintf(telemetrySat.nodeLabel, sizeof(telemetrySat.nodeLabel), "zeus_sat_01");

    router.register_node(harareTerminal);
    router.register_node(telemetrySat);

    std::cout << "[Network] Tracking Node Positions in 3D Cartesian Coordinates:\n";
    Zeus::CartesianVector v1 = router.get_cartesian_coordinates(harareTerminal);
    Zeus::CartesianVector v2 = router.get_cartesian_coordinates(telemetrySat);

    std::printf(" -> %s Matrix Points: [X: %.2f, Y: %.2f, Z: %.2f] km\n", harareTerminal.nodeLabel, v1.x, v1.y, v1.z);
    std::printf(" -> %s Matrix Points: [X: %.2f, Y: %.2f, Z: %.2f] km\n", telemetrySat.nodeLabel, v2.x, v2.y, v2.z);

    // 3. Run Great-Circle vector tracking diagnostics
    double calculatedLinkDistance = router.calculate_link_distance(harareTerminal, telemetrySat);
    std::printf("\n[Metrics] Great-Circle Link Diagonal Range: %.2f km\n", calculatedLinkDistance);

    // Establish maximum processing threshold capabilities for radio uplinks (e.g., 800 KM maximum)
    const double TRANSCEIVER_RANGE_LIMIT = 800.0;
    bool linkActive = router.verify_link_viability(harareTerminal, telemetrySat, TRANSCEIVER_RANGE_LIMIT);

    std::cout << " -> Dynamic Link State Assessment: "
              << (linkActive ? "ONLINE (Payload Delivery Verified)" : "OFFLINE (Link Range Exhausted)") << "\n";

    std::cout << "\n[Metrics] Mesh Topology Tracking Summary:\n";
    std::cout << " -> Active Registered Nodes: " << router.get_active_nodes_count() << " telemetry points live.\n";

    return linkActive ? 0 : 1;
}
