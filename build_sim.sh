#!/bin/bash
# Delegate to the simulation build helper from the repository root.
exec "$(dirname "$0")/simulation/build_sim.sh" "$@"
