#!/usr/bin/env python3

import importlib
import sys

import argus


def main() -> int:
    print(f"argus {argus.__version__}")

    try:
        algo = importlib.import_module("argus.algo")
    except ModuleNotFoundError:
        algo = None
    if algo is not None:
        print(algo.library_name())

    try:
        sim = importlib.import_module("argus.sim")
    except ModuleNotFoundError:
        sim = None
    if sim is not None:
        print(sim.library_name())

    return 0


if __name__ == "__main__":
    sys.exit(main())
