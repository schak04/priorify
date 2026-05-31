#!/bin/bash

INSTALL_DIR="$HOME/.local/share/priorify"
BIN_FILE="$HOME/.local/bin/priorify"

if [ ! -f "$BIN_FILE" ] && [ ! -d "$INSTALL_DIR" ]; then
    echo "Priorify doesn't seem to be installed. Nothing to do."
    exit 0
fi

echo "NUKING Priorify..."

if [ -f "$BIN_FILE" ]; then
    rm "$BIN_FILE"
    echo "Removed the wrapper script at $BIN_FILE."
fi

if [ -d "$INSTALL_DIR" ]; then
    if [ -f "$INSTALL_DIR/data/tasks.db" ]; then
        echo "Wait... I found your tasks database."
        read -p "Do you want me to absolutely OBLITERATE it too? [y/N] " -n 1 -r
        echo
        if [[ $REPLY =~ ^[Yy]$ ]]; then
            rm -rf "$INSTALL_DIR"
            echo "Fine. All gone. ($INSTALL_DIR is gone)."
        else
            rm -rf "$INSTALL_DIR/bin"
            echo "Okay, binaries are gone but your tasks are safe in $INSTALL_DIR/data"
            echo "(...just in case you change your mind later)"
        fi
    else
        rm -rf "$INSTALL_DIR"
        echo "Cleaned up $INSTALL_DIR. Nothing to see here."
    fi
fi

echo "Successfully uninstalled. If something bugged you about the app, please open an issue so I can work on it, would genuinely appreciate it. :)"