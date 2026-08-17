#!/usr/bin/env bash
set -euo pipefail

# --- НАСТРОЙКИ ---
ROUTER_IP="192.168.1.1"
ROUTER_USER="root"

BASE_DIR="$HOME/Projects/light/light_control/external/openwrt"
IPK_NAME="light_control_1.0.19-1_mips_24kc.ipk"
IPK_PATH="${BASE_DIR}/bin/packages/mips_24kc/light_control/${IPK_NAME}"

# --- ПРОВЕРКИ ---
if [[ ! -d "$BASE_DIR" ]]; then
  echo "Ошибка: папка сборки не найдена: $BASE_DIR" >&2
  exit 1
fi

if [[ ! -f "$IPK_PATH" ]]; then
  echo "Ошибка: .ipk не найден: $IPK_PATH" >&2
  echo "Сначала запусти сборку: ./build_openwrt_light_control.sh" >&2
  exit 1
fi

echo "=== Деплой пакета на роутер ($ROUTER_IP) ==="
echo "Пакет: $(ls -lh "$IPK_PATH")"

# --- ПЕРЕДАЧА (tar | ssh) ---
tar czf - -C "$(dirname "$IPK_PATH")" "$(basename "$IPK_PATH")" \
  | ssh "${ROUTER_USER}@${ROUTER_IP}" 'cat > /tmp/light_control.tar.gz'

# --- РАСПАКОВКА И УСТАНОВКА ---
ssh "${ROUTER_USER}@${ROUTER_IP}" "cd /tmp && tar xzf light_control.tar.gz && opkg install ${IPK_NAME}"

echo "=== Перезапуск сервиса light_control ==="
ssh "${ROUTER_USER}@${ROUTER_IP}" "/etc/init.d/light_control restart"

#echo "=== Проверка бинарника (размер и дата) ==="
#ssh "${ROUTER_USER}@${ROUTER_IP}" "ls -l /usr/bin/light_control"

#echo "=== Быстрая проверка ELF-заголовка (убеждаемся, что это Linux-бинарник) ==="
# Если head и hexdump есть — покажем первые байты. Если нет — просто пропустим без ошибки.
#if ssh "${ROUTER_USER}@${ROUTER_IP}" command -v head >/dev/null 2>&1 && \
#   ssh "${ROUTER_USER}@${ROUTER_IP}" command -v hexdump >/dev/null 2>&1; then
#  ssh "${ROUTER_USER}@${ROUTER_IP}" "head -c 16 /usr/bin/light_control | hexdump -C"
#else
#  echo "(head/hexdump отсутствуют на роутере — пропускаем проверку заголовка)"
#fi

#echo "=== Последние логи (поиск light_control) ==="
#ssh -t "${ROUTER_USER}@${ROUTER_IP}" "logread | grep -i light_control | tail -n 20"

echo "=== Готово ==="
