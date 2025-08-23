#!/usr/bin/env bash

# install script for linux which installs (copies) the compiled binary to /usr/local/bin
#
# before installing, the script automatically checks for updates (inside a clean (!) git repo)
# and tries to pull changes if possible to install the latest version
#
# if the program is already installed (i.e. if a program with the binary name already exists in the dest. folder),
# the install script will simply overwrite it
#
# the binary name is pulled from the makefile
# if you want to modify of the immediately compiled binary, change the TARGET variable in the makefile
# if you want to install the binary under a different name than the compiled binary, pass it as the first argument via the command line

BIN_NAME="$(make -s targetname)"
DEST_DIR="/usr/local/bin"

if [ "$#" -ge 1 ] && [ -n "${1-}" ]; then BIN_NAME="$1" 
fi

if git rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  echo "[install/git] git repository detected, checking for remote updates..."
  if [ -n "$(git status --porcelain)" ]; then echo "[install/git] warn: working tree has uncommitted changes; skipping auto-pull."
  else
    if git fetch --quiet origin; then
      if git rev-parse --abbrev-ref --symbolic-full-name @{u} >/dev/null 2>&1; then
        local_rev="$(git rev-parse @)"
        remote_rev="$(git rev-parse @{u})"
        base_rev="$(git merge-base @ @{u})"

        if [ "$local_rev" = "$remote_rev" ]; then echo "[install/git] already up to date"
        elif [ "$local_rev" = "$base_rev" ]; then
          current_branch="$(git rev-parse --abbrev-ref HEAD)"
          echo "[install/git] local is behind upstream/$current_branch, pulling latest changes..."
          if git pull --ff-only --quiet origin "$current_branch"; then echo "[install/git] pulled latest changes from remote"
          else echo "[install/git] git pull failed, skipping update..." >&2
          fi
        else echo "[install/git] local and remote have diverged (or local is ahead), skipping auto-pull..." >&2
        fi
      else echo "[install/git] no upstream configured for the current branch, skipping auto-pull..."
      fi
    else echo "[install/git] git fetch failed, skipping auto-pull..."
    fi
  fi
fi

echo "[install/make] running make..."
make clean
make

if [ ! -f "./$BIN_NAME" ]; then
    echo "[install/make] error: ./$BIN_NAME not found" >&2
    exit 1
fi

echo "[install] installing $BIN_NAME to $DEST_DIR..."
if [ "$(id -u)" -eq 0 ]; then install -m 0755 "./$BIN_NAME" "$DEST_DIR/$BIN_NAME"
else sudo install -m 0755 "./$BIN_NAME" "$DEST_DIR/$BIN_NAME"
fi

echo "[install] $BIN_NAME installed successfully to $DEST_DIR/$BIN_NAME"
