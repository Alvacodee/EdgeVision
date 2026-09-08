#!/usr/bin/env bash
# Setup emsdk sekali di awal. Jalankan ini di WSL2 Ubuntu (bukan di Windows langsung).
# Referensi: AGENT.md §12.1
set -euo pipefail

INSTALL_DIR="${HOME}/emsdk"

sudo apt update
sudo apt install -y git cmake python3 build-essential

if [ -d "$INSTALL_DIR" ]; then
    echo "emsdk sudah ada di $INSTALL_DIR, pull update aja..."
    cd "$INSTALL_DIR" && git pull
else
    git clone https://github.com/emscripten-core/emsdk.git "$INSTALL_DIR"
    cd "$INSTALL_DIR"
fi

./emsdk install latest
./emsdk activate latest

# Tambahin ke .bashrc kalau belum ada, biar emcc kepanggil di shell baru
if ! grep -q "emsdk_env.sh" "${HOME}/.bashrc" 2>/dev/null; then
    echo "source ${INSTALL_DIR}/emsdk_env.sh" >> "${HOME}/.bashrc"
    echo "Ditambahin ke ~/.bashrc. Jalankan 'source ~/.bashrc' atau buka terminal baru."
fi

echo "Setup selesai. Cek dengan: emcc --version"
