#!/usr/bin/env bash
# start.sh — One-command setup & launch for nano1602
# Checks/installs Python3 + pip if missing, then runs sender.py (auto-installs deps)
#
# Usage:
#   chmod +x start.sh && ./start.sh          # auto-detect port
#   chmod +x start.sh && ./start.sh setup    # install deps only, don't run
#   ./start.sh /dev/ttyUSB0                   # specify port

# ─── Colors & Styles ─────────────────────────────────────
GREEN='\033[0;32m'
BLUE='\033[0;34m'
YELLOW='\033[1;33m'
RED='\033[0;31m'
CYAN='\033[0;36m'
BOLD='\033[1m'
NC='\033[0m' # No Color

# ─── Helpers ─────────────────────────────────────────────
log_info()  { echo -e "${GREEN}[✓]${NC} $1"; }
log_warn()  { echo -e "${YELLOW}[!]${NC} $1"; }
log_error() { echo -e "${RED}[✗]${NC} $1"; }
log_step()  { echo -e "${CYAN}➜${NC} ${BOLD}$1${NC}"; }

print_banner() {
    echo ""
    echo -e "${BLUE}╔════════════════════════════════════════════════╗${NC}"
    echo -e "${BLUE}║${NC}  ${BOLD}🖥️  PC Monitor — Arduino Nano + LCD1602  ${NC} ${BLUE}║${NC}"
    echo -e "${BLUE}╚════════════════════════════════════════════════╝${NC}"
    echo -e "   ${YELLOW}v1.6.0${NC} | ${CYAN}USB Auto-Connect${NC} | ${CYAN}Smart Install${NC}"
    echo ""
}

# ─── Main Logic ──────────────────────────────────────────
print_banner

SCRIPT_DIR="$(cd "$(dirname "$0")" && pwd)"
SENDER="${SCRIPT_DIR}/sender.py"

if [ ! -f "$SENDER" ]; then
    log_error "Файл sender.py не найден. Убедитесь, что он лежит рядом со start.sh"
    exit 1
fi

# 1. Проверка Python
if ! command -v python3 &>/dev/null; then
    log_step "Установка Python3..."
    if command -v apt-get &>/dev/null; then
        sudo apt-get update -qq && sudo apt-get install -y -qq python3 python3-pip > /dev/null
    else
        log_error "Не удалось установить Python автоматически. Пожалуйста, установите его вручную."
        exit 1
    fi
    log_info "Python3 установлен"
else
    log_info "Python3 найден: $(python3 --version 2>&1)"
fi

# 2. Проверка pip
if ! python3 -m pip --version &>/dev/null; then
    log_step "Установка pip..."
    curl -sS https://bootstrap.pypa.io/get-pip.py | python3 > /dev/null
    log_info "pip готов"
fi

# 3. Режим только установки
if [ "$1" = "setup" ]; then
    log_step "Установка зависимостей..."
    python3 -m pip install --quiet -r "${SCRIPT_DIR}/requirements.txt" 2>/dev/null
    log_info "Всё готово! Запустите ./start.sh для старта."
    exit 0
fi

# 4. Запуск монитора
echo -e "\n${BOLD}🚀 Запуск монитора...${NC}\n"
echo -e "${BLUE}────────────────────────────────────────────────${NC}"

if [ -n "$1" ]; then
    python3 "$SENDER" "$1"
else
    python3 "$SENDER"
fi