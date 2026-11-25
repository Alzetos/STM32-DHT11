#!/usr/bin/env python3
"""Restore application integration after CubeMX regeneration; idempotent."""
from pathlib import Path
ROOT = Path(__file__).resolve().parents[1]
def integrate():
    path = ROOT / 'Core/Src/main.c'
    text = path.read_text()
    for marker, addition in [('Includes', '#include "monitor.h"'), ('2', '  Monitor_Init();'), ('3', '    Monitor_Run();')]:
        begin = '/* USER CODE BEGIN ' + marker + ' */'
        end = '/* USER CODE END ' + marker + ' */'
        section = text.split(begin, 1)[1].split(end, 1)[0]
        if addition.strip() not in section:
            text = text.replace(begin, begin + '\n' + addition, 1)
    # CubeMX 6.18.1 enables unused PLL3 unconditionally for F105/F107.
    # Match the correction already used by the working TrafficTimer project.
    text = text.replace('  __HAL_RCC_PLLI2S_ENABLE();\n', '')
    path.write_text(text)
    path = ROOT / 'Core/Src/stm32f1xx_it.c'
    with path.open(newline='') as stream:
        text = stream.read()
    for include in ['#include "board_role.h"', '#include "spi_link.h"', '#include "stepper_control.h"']:
        if include not in text:
            text = text.replace('/* USER CODE BEGIN Includes */', '/* USER CODE BEGIN Includes */\n' + include, 1)
    if 'StepperControl_Tick1ms();' not in text:
        text = text.replace('/* USER CODE BEGIN SysTick_IRQn 1 */',
                            '/* USER CODE BEGIN SysTick_IRQn 1 */\n  StepperControl_Tick1ms();', 1)
    if 'SpiLink_NssRisingIRQ();' not in text:
        text = text.replace('/* USER CODE BEGIN 1 */', '''/* USER CODE BEGIN 1 */
#if BOARD_ROLE == BOARD_ACTUATOR
void EXTI15_10_IRQHandler(void) { SpiLink_NssRisingIRQ(); }
#endif''', 1)
    with path.open('w', newline='') as stream:
        stream.write(text)
    for filename in ['Makefile', 'STM32Make.make']:
        path = ROOT / filename
        if not path.exists():
            continue
        text = path.read_text()
        # Literal source paths also work with the STM32-for-VSCode parser.
        for source in sorted((ROOT / 'App/Src').glob('*.c')):
            name = source.relative_to(ROOT).as_posix()
            if name not in text:
                text = text.replace('Core/Src/main.c', name + ' ' + chr(92) + '\nCore/Src/main.c', 1)
        text = text.replace('C_SOURCES += $(wildcard App/Src/*.c)\n', '')
        app = '\n# Application integration (restored by tools/integrate.py)\nC_INCLUDES += -IApp/Inc\n'
        if '-IApp/Inc' not in text:
            text = text.replace('OBJECTS =', app + '\nOBJECTS =', 1)
        path.write_text(text)
if __name__ == '__main__':
    integrate()
