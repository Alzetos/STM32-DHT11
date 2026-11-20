#!/usr/bin/env python3
"""Test the real stepper driver, all direction/holding configurations."""
from pathlib import Path
import subprocess
import tempfile
root = Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='dht11-stepper-test-') as folder:
    for reverse in (0, 1):
        for hold in (0, 1):
            binary = str(Path(folder) / f'stepper_{reverse}_{hold}')
            subprocess.run([
                'gcc', '-std=c11', '-Wall', '-Wextra', '-Werror',
                '-fsanitize=undefined', '-fno-sanitize-recover=all',
                f'-DSTEPPER_REVERSE_DIRECTION={reverse}',
                f'-DSTEPPER_HOLD_AT_TARGET={hold}',
                '-I' + str(root / 'tests/stepper_mock'),
                '-I' + str(root / 'App/Inc'),
                str(root / 'App/Src/stepper_control.c'),
                str(root / 'tests/stepper_control_test.c'), '-o', binary,
            ], check=True)
            subprocess.run([binary], check=True)
