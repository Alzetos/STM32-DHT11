#!/usr/bin/env python3
"""Exercise the production protocol and receiver/controller without hardware."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='greenhouse-spi-') as folder:
    binary = str(Path(folder) / 'greenhouse_spi_test')
    subprocess.run(['gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                    '-fsanitize=undefined', '-fno-sanitize-recover=all',
                    '-I' + str(root / 'App/Inc'),
                    *[str(root / 'App/Src' / name) for name in
                      ['spi_protocol.c', 'greenhouse.c', 'greenhouse_server.c', 'greenhouse_client.c']],
                    str(root / 'tests/greenhouse_spi_test.c'), '-o', binary], check=True)
    subprocess.run([binary], check=True)
