#!/bin/bash
set -e

echo "Building..."
chmod +x scripts/build.sh
./scripts/build.sh

echo "Installing and moving stuff around so your puter can find 'em..."
INSTALL_DIR="$HOME/.local/share/priorify"
BIN_DIR="$HOME/.local/bin"

mkdir -p "$INSTALL_DIR/bin"
mkdir -p "$INSTALL_DIR/data"
mkdir -p "$BIN_DIR"

cp bin/priorify "$INSTALL_DIR/bin/"

if [ -f "data/tasks.db" ]; then
    echo "Oh hey, you already have some tasks. Carrying them over."
    cp data/tasks.db "$INSTALL_DIR/data/"
fi

cat << EOF > "$BIN_DIR/priorify"
#!/bin/bash
cd "$INSTALL_DIR" && ./bin/priorify "\$@"
EOF

chmod +x "$BIN_DIR/priorify"

echo "Aaaand we're done"
echo "You can now type 'priorify' from anywhere in your terminal to use it."
echo "(Assuming $BIN_DIR is in your PATH. If it isn't, well, go fix your dotfiles. Refer to the README of this project if you need help with that.)"