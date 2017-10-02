#!/bin/sh

if [ "$(id -u)" -ne 0 ]; then
    echo "This script must be run as root" 
    exit 1
fi

TARGET_USER="$1"
SYMLINK_PATH="/home/$TARGET_USER/.nix-profile"
PROFILE_DIR="/nix/var/nix/profiles/per-user/$TARGET_USER"

echo "Creating profile $PROFILE_DIR..."
echo "Profile symlink: $SYMLINK_PATH"

rm "$SYMLINK_PATH"
mkdir -p "$PROFILE_DIR"
chown "$TARGET_USER:$TARGET_USER" "$PROFILE_DIR"

ln -s "$PROFILE_DIR/profile" "$SYMLINK_PATH"
chown -h "$TARGET_USER:$TARGET_USER" "$SYMLINK_PATH"

echo "export NIX_REMOTE=daemon" >> "/home/$TARGET_USER/.bashrc"
echo ". /usr/local/etc/profile.d/nix.sh" >> "/home/$TARGET_USER/.bashrc"

su -lc "cd; . /usr/local/etc/profile.d/nix.sh; NIX_REMOTE=daemon nix-channel --update" "$TARGET_USER"

exit 0

