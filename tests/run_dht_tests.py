#!/usr/bin/env python3
"""Test the actual DHT11 driver against generated bus waveforms."""
from pathlib import Path
import subprocess
import tempfile

root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dht11-wave-test-') as folder:
    binary = str(Path(folder) / 'dht11_test')
    subprocess.run([
        'gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
        '-fsanitize=undefined', '-fno-sanitize-recover=all',
        '-I' + str(root / 'tests/dht_mock'), '-I' + str(root / 'App/Inc'),
        str(root / 'App/Src/dht11.c'), str(root / 'tests/dht11_test.c'),
        '-o', binary,
    ], check=True)
    subprocess.run([binary], check=True)
