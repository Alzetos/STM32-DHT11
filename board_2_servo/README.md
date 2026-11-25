# GreenhouseServo

Плата 2: SPI2 Slave, сервопривод PA6 / TIM3_CH1, TFT. DHT11 и потенциометр не нужны.

Откройте именно эту папку в VS Code. Ctrl+Shift+B собирает проект; F5 запускает отладку через ST-LINK. После остановки на main нажмите Continue.

Сборка: `python3 tools/project.py build`. Прошивка: `python3 tools/project.py flash-stlink`. Файл: `build/GreenhouseServo.elf`.

При прошивке подключайте к компьютеру только ST-LINK нужной платы. Одинаковые программаторы автоматически не различаются.

Инструкция по соединению, протоколу и проверке: SPI_GREENHOUSE_RU.md. SPI2 и DMA настраиваются в App/Src/spi_link.c; IOC сохраняет рабочие клоки и резервирует выводы.
