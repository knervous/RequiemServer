#!/bin/bash
set -e

# This script installs Go 1.24.1, protoc 29.4, and protoc-gen-go if not already installed,
# and automatically updates PATH.

# Define installation directories
GO_INSTALL_DIR="/usr/local/go"  # Requires sudo if not installing to user dir
PROTOC_INSTALL_DIR="$HOME/bin/protoc"
GOPATH="${GOPATH:-$HOME/go}"   # Default to ~/go if GOPATH not set

# Function to update PATH and persist it
update_path() {
    local new_path="$1"
    export PATH="$new_path:$PATH"
    # Append to ~/.bashrc if not already present
    if ! grep -q "$new_path" ~/.bashrc 2>/dev/null; then
        echo "export PATH=\"$new_path:\$PATH\"" >> ~/.bashrc
        echo "Added $new_path to ~/.bashrc"
    fi
}

##############################
# Install Go if not found
##############################
echo "Checking for Go..."
if ! command -v go >/dev/null 2>&1; then
    echo "Go not found. Installing Go 1.24.1..."
    GO_URL="https://go.dev/dl/go1.24.1.linux-amd64.tar.gz"
    GO_TARBALL="/tmp/go1.24.1.linux-amd64.tar.gz"

    echo "Downloading Go from $GO_URL..."
    curl -fL -o "$GO_TARBALL" "$GO_URL" || { echo "Failed to download Go tarball"; exit 1; }

    echo "Extracting Go tarball to $GO_INSTALL_DIR..."
    sudo mkdir -p "$GO_INSTALL_DIR" || { echo "Failed to create $GO_INSTALL_DIR (try running with sudo)"; exit 1; }
    sudo tar -C "$GO_INSTALL_DIR" -xzf "$GO_TARBALL" --strip-components=1 || { echo "Failed to extract Go tarball"; exit 1; }
    rm -f "$GO_TARBALL"

    update_path "$GO_INSTALL_DIR/bin"
    echo "Go installed to $GO_INSTALL_DIR/bin and PATH updated."
else
    echo "Go is already installed: $(command -v go)"
fi

##############################
# Install protoc if not found
##############################
echo "Checking for protoc..."
if ! command -v protoc >/dev/null 2>&1; then
    echo "protoc not found. Installing protoc 29.4..."
    
    if ! command -v unzip >/dev/null 2>&1; then
        echo "unzip not found. Please install unzip (e.g., 'sudo apt install unzip' on Ubuntu) and re-run this script."
        exit 1
    fi

    PROTOC_URL="https://github.com/protocolbuffers/protobuf/releases/download/v29.4/protoc-29.4-linux-x86_64.zip"
    PROTOC_ZIP="/tmp/protoc-29.4-linux-x86_64.zip"

    echo "Downloading protoc from $PROTOC_URL..."
    curl -fL -o "$PROTOC_ZIP" "$PROTOC_URL" || { echo "Failed to download protoc"; exit 1; }

    echo "Extracting protoc to $PROTOC_INSTALL_DIR..."
    mkdir -p "$PROTOC_INSTALL_DIR"
    unzip -o "$PROTOC_ZIP" -d "$PROTOC_INSTALL_DIR" || { echo "Failed to unzip protoc archive"; exit 1; }
    rm -f "$PROTOC_ZIP"

    update_path "$PROTOC_INSTALL_DIR/bin"
    echo "protoc installed to $PROTOC_INSTALL_DIR/bin and PATH updated."
else
    echo "protoc is already installed: $(command -v protoc)"
fi

##############################
# Install protoc-gen-go if not found
##############################
echo "Checking for protoc-gen-go..."
if ! command -v protoc-gen-go >/dev/null 2>&1; then
    echo "protoc-gen-go not found. Installing protoc-gen-go..."
    export PATH="$GO_INSTALL_DIR/bin:$PATH"  # Ensure Go is in PATH for this step
    go install google.golang.org/protobuf/cmd/protoc-gen-go@latest || { echo "Failed to install protoc-gen-go"; exit 1; }

    if [ ! -f "$GOPATH/bin/protoc-gen-go" ]; then
        echo "protoc-gen-go not found in $GOPATH/bin after installation."
        exit 1
    fi
    update_path "$GOPATH/bin"
    echo "protoc-gen-go installed to $GOPATH/bin/protoc-gen-go and PATH updated."
else
    echo "protoc-gen-go is already installed: $(command -v protoc-gen-go)"
fi

echo "All tools installed successfully!"
echo "PATH has been updated for this session."
echo "To apply changes in a new terminal, run 'source ~/.bashrc' or restart your terminal."