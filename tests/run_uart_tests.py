#!/usr/bin/env python3
"""Compile production UART/parser/protocol/session code against register mocks."""
from pathlib import Path
import subprocess
import tempfile
root=Path(__file__).resolve().parents[1]
with tempfile.TemporaryDirectory(prefix='uart-link-') as folder:
    binary=str(Path(folder)/'uart_test')
    subprocess.run(['gcc','-std=c11','-Wall','-Wextra','-Werror',
        '-fsanitize=undefined','-fno-sanitize-recover=all','-DBOARD_ROLE=1',
        '-I'+str(root/'tests/uart_mock'),'-I'+str(root/'App/Inc'),
        *[str(root/'App/Src'/name) for name in ['uart_link.c','uart_framer.c',
            'link_protocol.c','greenhouse_client.c','greenhouse_server.c','greenhouse.c']],
        str(root/'tests/uart_link_test.c'),'-o',binary],check=True)
    subprocess.run([binary],check=True)
