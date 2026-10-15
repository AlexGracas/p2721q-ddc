#!/bin/sh
# Builds build/p2721q-<version>.pkg, which installs /usr/local/bin/p2721q
set -e
export COPYFILE_DISABLE=1
cd "$(dirname "$0")/.."
VERSION="${1:-1.0.0}"
make clean >/dev/null && make
mkdir -p build/root/usr/local/bin
cp p2721q build/root/usr/local/bin/p2721q
pkgbuild --root build/root --filter '/\._' --filter '/\.DS_Store$' --identifier io.github.alexgracas.p2721q \
  --version "$VERSION" --install-location / "build/p2721q-$VERSION.pkg"
echo "built build/p2721q-$VERSION.pkg"
