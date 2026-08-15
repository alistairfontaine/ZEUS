#!/bin/bash
set -e

echo "=========================================================="
echo "ZEUS PLANETARY ROUTER MASTER AUTOMATION PIPELINE"
echo "=========================================================="

echo -e "\n[Step 1/3] Compiling host-optimized native networking router..."
rm -f zeus-router
g++ -O3 -std=c++17 -march=native src/main.cpp -I include -o zeus-router

echo -e "\n[Step 2/3] Executing spherical trigonometry path simulations..."
./zeus-router

echo -e "\n[Step 3/3] Committing production modules to cloud tracking networks..."
git add src/main.cpp include/ZeusCore.hpp docs/ROADMAP.md docs/ARCHITECTURE.md README.md deploy.sh .gitignore .gitattributes tests/

if ! git diff-index --quiet HEAD --; then
    git commit -m "pipeline: finalize Project ZEUS multi-hop graph matrices and binary state systems"
    git push origin main
else
    echo "No modifications detected. Codebase status is fully synchronized."
fi

echo -e "\n=========================================================="
echo "SUCCESS! ZEUS MASTERWORK IS SIGNED, SEALED, AND RUNNING LIVE."
echo "=========================================================="
