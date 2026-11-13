"""Standalone regression checks. Run from the repository: python -B tests/check.py."""
import ast
import contextlib
import importlib.util
import io
import json
import math
import os
from pathlib import Path
import re
import runpy
import subprocess
import sys
import tempfile
import types

ROOT = Path(__file__).resolve().parents[1]


def run(args, **kwargs):
    result = subprocess.run(args, capture_output=True, text=True, timeout=45, **kwargs)
    assert result.returncode == 0, (args, result.stdout, result.stderr)
    return result.stdout

def check_movies(tmp):
    exe = tmp / 'movies.exe'
    run(['gcc', '-std=c11', '-O2', '-Wall', '-Wextra', '-Wpedantic', '-Werror', str(ROOT / 'src/main.c'), '-lm', '-o', str(exe)])
    # Three users, four movies: non-square dimensions exposed the original indexing bug.
    fixture = '1 10 5 1\n1 20 1 1\n2 10 5 1\n2 30 5 1\n3 10 1 1\n3 40 1 1\n'
    (tmp / 'u.data').write_text(fixture)
    for choice in range(1, 7):
        inputs = f'1\n{choice}\n' + ('1\n' if choice == 5 else '') + '0\n'
        out = run([str(exe), str(tmp / 'u.data')], cwd=tmp, input=inputs)
        movies = [int(x) for x in re.findall(r'Film ID: (\d+)', out)]
        assert movies and set(movies) <= {30, 40} and len(movies) == len(set(movies)), (choice, out)
        if choice == 4: assert movies[0] == 30, out
    assert 'Gecersiz kume' in run([str(exe), str(tmp / 'u.data')], cwd=tmp, input='1\n5\n0\n0\n')
    # All movies already seen: random recommendations must terminate.
    (tmp / 'u.data').write_text('1 10 5 1\n')
    out = run([str(exe), str(tmp / 'u.data')], cwd=tmp, input='1\n1\n0\n')
    assert 'Onerilecek film yok' in out
    for bad in ('', '1 10 5', '1 10 9 1\n', '1 10 5\n2 20 4 1 7\n',
                '1 10 5 1 2\n', '2147483648 10 5 1\n', '1 10 5 1x\n'):
        (tmp / 'u.data').write_text(bad)
        assert subprocess.run([str(exe), str(tmp / 'u.data')], cwd=tmp, input='', text=True, capture_output=True, timeout=5).returncode != 0
    (tmp / 'u.data').unlink()
    assert subprocess.run([str(exe), str(tmp / 'u.data')], cwd=tmp, input='', text=True, capture_output=True, timeout=5).returncode != 0
    out = run([str(exe)], cwd=ROOT, input='1\n1\n2\n3\n4\n5\n3\n6\n0\n')
    assert 'Film ID:' in out
    print('PASS: all six movie methods, ranking, uniqueness, seen filtering, invalid/missing data, bundled synthetic sample')

if __name__ == '__main__':
    with tempfile.TemporaryDirectory() as directory:
        check_movies(Path(directory))
    print('All movie recommendation checks passed.')
