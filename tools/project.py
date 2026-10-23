#!/usr/bin/env python3
"""Build and flash from VS Code using locally installed STM32 tools."""
import argparse
import os
from pathlib import Path
import shutil
import subprocess
import sys
from integrate import ROOT, integrate

def compiler_dir():
    if os.environ.get('ARM_GCC_BIN'):
        return Path(os.environ['ARM_GCC_BIN'])
    found = shutil.which('arm-none-eabi-gcc')
    if found:
        return Path(found).parent
    matches = sorted(Path('/opt/st').glob('**/tools/bin/arm-none-eabi-gcc'))
    if matches:
        return matches[-1].parent
    sys.exit('ARM GCC not found. Add arm-none-eabi-gcc to PATH or set ARM_GCC_BIN.')

def build():
    integrate()
    subprocess.run(['make', '-j4', 'GCC_PATH=' + str(compiler_dir())], cwd=ROOT, check=True)

def regenerate():
    cubemx = os.environ.get('CUBEMX', str(Path.home() / 'STM32CubeMX/STM32CubeMX'))
    script = ROOT / 'tools/generate.mx'
    script.write_text('config load "' + str(ROOT / 'DHT11Monitor.ioc') + '"\n'
                      'project name DHT11Monitor\nproject toolchain Makefile\n'
                      'project generate "' + str(ROOT) + '"\nconfig save\nexit\n')
    log = ROOT / 'tools/cubemx-generation.log'
    with log.open('w') as out:
        subprocess.run([cubemx, '-q', str(script)], cwd=ROOT, stdout=out, stderr=subprocess.STDOUT, check=True, timeout=240)
    if 'Generated code:' not in log.read_text():
        sys.exit('CubeMX generation failed; see tools/cubemx-generation.log')
    integrate()

parser = argparse.ArgumentParser()
parser.add_argument('action', choices=['build', 'clean', 'regenerate', 'flash-dfu', 'flash-stlink'])
args = parser.parse_args()
try:
    if args.action == 'regenerate':
        regenerate()
    elif args.action == 'clean':
        subprocess.run(['make', 'clean'], cwd=ROOT, check=True)
    else:
        build()
        if args.action == 'flash-dfu':
            tool = shutil.which('dfu-util')
            if not tool:
                sys.exit('Install dfu-util. Connect MCU USB, BOOT0=1, BOOT1=0, press RESET. See README.md.')
            subprocess.run([tool, '-d', '0483:df11', '-a', '0', '-s', '0x08000000:leave', '-D', str(ROOT / 'build/DHT11Monitor.bin')], check=True)
            print('Set BOOT0=0 and press RESET to run the program.')
        elif args.action == 'flash-stlink':
            tool = shutil.which('openocd')
            if not tool:
                sys.exit('Install OpenOCD. This task requires a compatible ST-LINK; see README.md.')
            subprocess.run([tool, '-f', str(ROOT / 'openocd.cfg'), '-c', 'program {' + str(ROOT / 'build/DHT11Monitor.elf') + '} verify reset exit'], cwd=ROOT, check=True)
except (subprocess.CalledProcessError, subprocess.TimeoutExpired) as exc:
    sys.exit(str(exc))
