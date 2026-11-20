#!/usr/bin/env python3
"""Exercise the actual servo controller with a mock timer, without a board."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dht11-servo-test-') as folder:
    binary = str(Path(folder) / 'servo_control_test')
    subprocess.run([
        'gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
        '-fsanitize=undefined', '-fno-sanitize-recover=all',
        '-I' + str(root / 'tests/mock'), '-I' + str(root / 'App/Inc'),
        str(root / 'App/Src/servo_control.c'),
        str(root / 'tests/servo_control_test.c'), '-o', binary,
    ], check=True)
    subprocess.run([binary], check=True)
