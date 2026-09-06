"""Punto d'ingresso: `python -m rt3 ...`."""

import sys

from .cli import main

if __name__ == "__main__":
    sys.exit(main())
