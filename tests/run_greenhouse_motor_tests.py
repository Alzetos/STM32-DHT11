#!/usr/bin/env python3
"""Test actual protocol/controller/stepper integration for all holding/direction modes."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='greenhouse-motor-') as folder:
    for reverse in (0,1):
        for hold in (0,1):
            binary=str(Path(folder)/f'motor_{reverse}_{hold}')
            subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',
                '-fsanitize=undefined','-fno-sanitize-recover=all',
                f'-DSTEPPER_REVERSE_DIRECTION={reverse}',f'-DSTEPPER_HOLD_AT_TARGET={hold}',
                '-I'+str(root/'tests/stepper_mock'),'-I'+str(root/'App/Inc'),
                *[str(root/'App/Src'/f) for f in ['stepper_control.c','greenhouse.c',
                    'greenhouse_server.c','link_protocol.c','greenhouse_motor.c']],
                str(root/'tests/greenhouse_motor_test.c'),'-o',binary],check=True)
            subprocess.run([binary],check=True)
