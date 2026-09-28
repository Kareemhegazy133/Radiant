#!/bin/bash

# Resolve the repo root from this script's location, so it works from any CWD
cd "$(dirname "$0")/.."

Tools/Binaries/Premake/Linux/premake5 --cc=clang --file=Build.lua gmake2
