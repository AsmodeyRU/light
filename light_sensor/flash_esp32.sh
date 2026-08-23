#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Путь к ESP‑IDF (ожидаем симлинк или реальную папку в externals)
IDF_PATH="$SCRIPT_DIR/externals/esp-idf"

if [ ! -d "$IDF_PATH" ]; then
    echo "❌ Ошибка: ESP‑IDF не найден по пути $IDF_PATH" >&2
    exit 1
fi

export IDF_PATH

# set environment
. "$IDF_PATH/export.sh"

cd "$SCRIPT_DIR/esp32"

# ------------------------------------------------------------------
# 1. Проверка существования файла прошивки
# ------------------------------------------------------------------
BUILD_DIR="build"
FIRMWARE_BIN="$BUILD_DIR/light_sensor_esp32.bin"

if [ ! -f "$FIRMWARE_BIN" ]; then
    echo "⚠️ Файл прошивки не найден: $FIRMWARE_BIN" >&2
    echo "Сначала выполните сборку: idf.py build" >&2
    # Можно раскомментировать, чтобы скрипт сам делал build:
    # idf.py build
    exit 1
fi
echo "✅ Прошивка найдена: $FIRMWARE_BIN"

# ------------------------------------------------------------------
# 2. Автоопределение порта и проверка подключения
# ------------------------------------------------------------------
PORT=""

# Ищем все USB‑serial устройства
candidates=()
for dev in /dev/ttyUSB* /dev/ttyACM*; do
    [ -c "$dev" ] || continue
    candidates+=("$dev")
done

if [ ${#candidates[@]} -eq 0 ]; then
    echo "❌ Не найдено ни одного последовательного порта (/dev/ttyUSB* или /dev/ttyACM*)" >&2
    exit 1
fi

# Если порт передан явно через переменную окружения PORT, используем его
if [ -n "${PORT:-}" ]; then
    if ! printf '%s\n' "${candidates[@]}" | grep -qx "$PORT"; then
        echo "❌ Указанный порт $PORT не найден среди доступных устройств" >&2
        exit 1
    fi
    echo "✅ Используем явно указанный порт: $PORT"
else
    # Пытаемся найти рабочий порт автоматически
    for dev in "${candidates[@]}"; do
        # Пробуем прочитать MAC — это быстрый способ проверить, что ESP32 отвечает
        if esptool.py --port "$dev" --baud 921600 read_mac >/dev/null 2>&1; then
            PORT="$dev"
            echo "✅ Устройство найдено на порту: $PORT"
            break
        fi
    done

    if [ -z "$PORT" ]; then
        echo "❌ Ни один из портов не ответил на запрос ESP32 (возможно, плата не в режиме загрузчика)" >&2
        echo "Доступные порты: ${candidates[*]}" >&2
        echo "Попробуйте вручную нажать BOOT+EN или указать порт: PORT=/dev/ttyUSB0 $0" >&2
        exit 1
    fi
fi

# ------------------------------------------------------------------
# 3. Прошивка и монитор
# ------------------------------------------------------------------
idf.py --port "$PORT" flash monitor
