#!/usr/bin/env python3
"""Create/update the two independent board projects; never flash hardware.

Shared sources are maintained in the root. Running this script overwrites the
distributed source/config files, but leaves their build folders untouched.
"""
from pathlib import Path
import shutil
from integrate import ROOT, integrate

BOARDS = [('board_1_sensor', 'GreenhouseSensor', 'BOARD_SENSOR'),
          ('board_2_stepper', 'GreenhouseStepper', 'BOARD_ACTUATOR')]

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
        # UART2 is configured by uart_link.c; IOC reserves its default pins.
        ioc = (ROOT / 'DHT11Monitor.ioc').read_text().replace('DHT11Monitor', target)
        removed_ips = {'TIM3'} | ({'ADC1'} if role == 'BOARD_ACTUATOR' else set())
        removed_pins = {'PA6'} | ({'PA4'} if role == 'BOARD_ACTUATOR' else set())
        lines = [line for line in ioc.splitlines()
                 if not any(line.startswith(ip+'.') for ip in removed_ips)
                 and not any(line.startswith(pin+'.') for pin in removed_pins | {'PA2','PA3'})]
        ips = [line.split('=',1)[1] for line in lines
               if line.startswith('Mcu.IP') and line.split('=',1)[0][6:].isdigit()]
        ips = [ip for ip in ips if ip not in removed_ips]
        original_pins = [line.split('=',1)[1] for line in lines
                         if line.startswith('Mcu.Pin') and line.split('=',1)[0][7:].isdigit()]
        all_pins = list(dict.fromkeys([pin for pin in original_pins if pin not in removed_pins] + ['PA2','PA3']))
        lines = [line for line in lines if not line.startswith(('Mcu.IP','Mcu.Pin'))]
        lines += [f'Mcu.IP{i}={ip}' for i,ip in enumerate(ips)] + [f'Mcu.IPNb={len(ips)}']
        lines += [f'Mcu.Pin{i}={pin}' for i,pin in enumerate(all_pins)] + [f'Mcu.PinsNb={len(all_pins)}']
        for i,line in enumerate(lines):
            if line.startswith('ProjectManager.functionlistsort='):
                parts = [p for p in line.split('=',1)[1].split(',')
                         if not any('-MX_'+ip+'_Init-' in p for ip in removed_ips)]
                lines[i] = 'ProjectManager.functionlistsort=' + ','.join(
                    str(j)+'-'+part.split('-',1)[1] for j,part in enumerate(parts,1))
        for pin,label,signal in [('PA2','UART2_TX','GPIO_Output'),('PA3','UART2_RX','GPIO_Input')]:
            lines += [f'{pin}.GPIOParameters=GPIO_Label', f'{pin}.GPIO_Label={label}',
                      f'{pin}.Locked=true',f'{pin}.Signal={signal}']
        (dest / (target + '.ioc')).write_text('\n'.join(lines)+'\n')
        # Drop unused servo initialization; actuator also does not use ADC.
        main_path = dest / 'Core/Src/main.c'
        main = main_path.read_text()
        for ip in removed_ips:
            name = 'MX_'+ip+'_Init'
            main = main.replace('  '+name+'();', '').replace('static void '+name+'(void);', '')
            begin = main.find('static void '+name+'(void)')
            if begin >= 0:
                brace = main.index('{', begin); depth = 1; finish = brace + 1
                while depth:
                    depth += (main[finish] == '{') - (main[finish] == '}')
                    finish += 1
                main = main[:begin] + main[finish:]
        main_path.write_text(main)
        for obsolete in ['App/Inc/spi_link.h','App/Src/spi_link.c',
                         'App/Inc/spi_protocol.h','App/Src/spi_protocol.c']:
            (dest / obsolete).unlink(missing_ok=True)
        (dest / '.gitignore').write_text('build/\n__pycache__/\n.vscode/.cortex-debug*\n')
        (dest / 'README.md').write_text(
            f'# {target}\n\n' +
            ('Плата 1: DHT11 на PB8, UART2, TFT, потенциометр PA4, LED PB6. Двигатель подключается ко второй плате.\n'
             if role == 'BOARD_SENSOR' else
             'Плата 2: UART2, 28BYJ-48 через ULN2003, IN1–IN4 на PC6–PC9, TFT. DHT11 и потенциометр не нужны.\n') +
            '\nОткройте именно эту папку в VS Code. Ctrl+Shift+B собирает проект; F5 запускает отладку через ST-LINK. После остановки на main нажмите Continue.\n\n' +
            f'Сборка: `python3 tools/project.py build`. Прошивка: `python3 tools/project.py flash-stlink`. Файл: `build/{target}.elf`.\n\n' +
            'При прошивке подключайте к компьютеру только ST-LINK нужной платы. Одинаковые программаторы автоматически не различаются.\n\n' +
            'Инструкция по соединению, протоколу и проверке: UART_GREENHOUSE_RU.md. UART2 настраивается в App/Src/uart_link.c; IOC сохраняет рабочие клоки и резервирует выводы.\n')
        shutil.copy2(ROOT / 'UART_GREENHOUSE_RU.md', dest / 'UART_GREENHOUSE_RU.md')
        (dest / 'SPI_GREENHOUSE_RU.md').write_text('Связь перенесена на UART2. Актуальная инструкция: UART_GREENHOUSE_RU.md.\n')
        print(f'{folder}: {role} -> build/{target}.elf')

if __name__ == '__main__':
    package()
