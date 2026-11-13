"""Portable C build: python scripts/build.py [--cc gcc]."""
import argparse
import os
from pathlib import Path
import subprocess

parser = argparse.ArgumentParser()
parser.add_argument('--cc', default=os.environ.get('CC', 'gcc'))
args = parser.parse_args()
root = Path(__file__).resolve().parents[1]
build = root / 'build'
build.mkdir(exist_ok=True)
binary = build / ('movie_recommender' + ('.exe' if os.name == 'nt' else ''))
subprocess.run([args.cc, '-std=c11', '-O2', '-Wall', '-Wextra', '-Wpedantic',
                str(root / 'src/main.c'), '-lm', '-o', str(binary)], check=True)
print(binary)
