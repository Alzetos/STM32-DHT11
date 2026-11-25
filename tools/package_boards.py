#!/usr/bin/env python3
"""Create/update the two independent board projects; never flash hardware.

Shared sources are maintained in the root. Running this script overwrites the
distributed source/config files, but leaves their build folders untouched.
"""
from pathlib import Path
import shutil
from integrate import ROOT, integrate

BOARDS = [('board_1_sensor', 'GreenhouseSensor', 'BOARD_SENSOR'),
          ('board_2_servo', 'GreenhouseServo', 'BOARD_ACTUATOR')]

def package():
    integrate()
    for folder, target, role in BOARDS:
        dest = ROOT / folder
        dest.mkdir(exist_ok=True)
        for name in ['Core', 'App', 'Drivers', '.vscode']:
            shutil.copytree(ROOT / name, dest / name, dirs_exist_ok=True,
                            ignore=shutil.ignore_patterns('__pycache__', '*.pyc', '.cortex-debug*'))
        (dest / 'tools').mkdir(exist_ok=True)
        for name in ['integrate.py', 'project.py']:
            content = (ROOT / 'tools' / name).read_text().replace('DHT11Monitor', target)
            (dest / 'tools' / name).write_text(content)
        for name in ['Makefile', 'STM32Make.make', 'STM32-for-VSCode.config.yaml',
                     '.stm32env', 'openocd.cfg', 'STM32F107xx_FLASH.ld', 'startup_stm32f107xc.s']:
            (dest / name).write_text((ROOT / name).read_text().replace('DHT11Monitor', target))
        role_header = (dest / 'App/Inc/board_role.h').read_text()
        role_header = role_header.replace('#define BOARD_ROLE BOARD_STANDALONE', '#define BOARD_ROLE ' + role)
        (dest / 'App/Inc/board_role.h').write_text(role_header)
        for path in (dest / '.vscode').glob('*.json'):
            path.write_text(path.read_text().replace('DHT11Monitor', target))
        # Retain the known-working clocks, ADC/TIM/TFT setup. Reserve SPI pins
        # as GPIO in CubeMX; App/Src/spi_link.c configures SPI2 + DMA at runtime.
        ioc = (ROOT / 'DHT11Monitor.ioc').read_text().replace('DHT11Monitor', target)
        ioc = ioc.replace('Mcu.PinsNb=27', 'Mcu.PinsNb=31')
        entries = []
        for i, pin in enumerate(['PB12', 'PB13', 'PB14', 'PB15'], 27):
            entries.append(f'Mcu.Pin{i}={pin}')
            output = (pin != 'PB14') if role == 'BOARD_SENSOR' else (pin == 'PB14')
            label = {'PB12': 'SPI2_CS' if role == 'BOARD_SENSOR' else 'SPI2_NSS',
                     'PB13': 'SPI2_SCK', 'PB14': 'SPI2_MISO', 'PB15': 'SPI2_MOSI'}[pin]
            entries.extend([f'{pin}.GPIOParameters=GPIO_Label', f'{pin}.GPIO_Label={label}',
                            f'{pin}.Locked=true', f'{pin}.Signal=GPIO_Output' if output else f'{pin}.Signal=GPIO_Input'])
        ioc += '\n' + '\n'.join(entries) + '\n'
        if role == 'BOARD_SENSOR':
            ioc = ioc.replace('PB12.GPIOParameters=GPIO_Label', 'PB12.GPIOParameters=GPIO_Label,PinState')
            ioc += 'PB12.PinState=GPIO_PIN_SET\n'
        (dest / (target + '.ioc')).write_text(ioc)
        (dest / '.gitignore').write_text('build/\n__pycache__/\n.vscode/.cortex-debug*\n')
        (dest / 'README.md').write_text(
            f'# {target}\n\n' +
            ('Плата 1: DHT11 на PB8, SPI2 Master, TFT, потенциометр PA4, LED PB6 и шаговый двигатель PC6–PC9.\n'
             if role == 'BOARD_SENSOR' else
             'Плата 2: SPI2 Slave, сервопривод PA6 / TIM3_CH1, TFT. DHT11 и потенциометр не нужны.\n') +
            '\nОткройте именно эту папку в VS Code. Ctrl+Shift+B собирает проект; F5 запускает отладку через ST-LINK. После остановки на main нажмите Continue.\n\n' +
            f'Сборка: `python3 tools/project.py build`. Прошивка: `python3 tools/project.py flash-stlink`. Файл: `build/{target}.elf`.\n\n' +
            'При прошивке подключайте к компьютеру только ST-LINK нужной платы. Одинаковые программаторы автоматически не различаются.\n\n' +
            'Инструкция по соединению, протоколу и проверке: SPI_GREENHOUSE_RU.md. SPI2 и DMA настраиваются в App/Src/spi_link.c; IOC сохраняет рабочие клоки и резервирует выводы.\n')
        shutil.copy2(ROOT / 'SPI_GREENHOUSE_RU.md', dest / 'SPI_GREENHOUSE_RU.md')
        print(f'{folder}: {role} -> build/{target}.elf')

if __name__ == '__main__':
    package()
