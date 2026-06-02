#!/bin/bash
set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
ROOT_DIR="$(dirname "$SCRIPT_DIR")"

chmod +x "$SCRIPT_DIR/build.sh"
"$SCRIPT_DIR/build.sh"

echo ""
echo "Installing and moving stuff around so your puter can find 'em..."
INSTALL_DIR="$HOME/.local/share/priorify"
BIN_DIR="$HOME/.local/bin"

mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/data"
mkdir -p "$BIN_DIR"

cp "$ROOT_DIR/bin/priorify" "$INSTALL_DIR/bin/"

cat << EOF > "$BIN_DIR/priorify"
#!/bin/bash
cd "$INSTALL_DIR" && ./bin/priorify "\$@"
EOF

chmod +x "$BIN_DIR/priorify"

echo "Aaaand we're done."
echo "You can now type 'priorify' from anywhere in your terminal to use it."
echo "(Assuming $BIN_DIR is in your PATH. If it isn't, well, go fix your dotfiles. Refer to the README of this project if you need help with that.)"