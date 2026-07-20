#!/usr/bin/env bash
# scripts/bootstrap.sh, one-time (idempotent) dev-environment setup: the
# host-toolchain packages every module's CMake host build needs, plus the
# pre-commit hook (scripts/install-git-hooks.sh). Linux only, developed
# against Debian/Ubuntu's apt; see "Other package managers" below if
# that's not you.
#
#   ./scripts/bootstrap.sh              install what's missing, then the hook
#   ./scripts/bootstrap.sh --check-only report only, install nothing, exit
#                                        nonzero if anything is missing
#
# Real-target (XC8) builds need MPLAB X + MPLAB XC8 v3.x installed
# manually: proprietary, license-gated, an interactive installer, not
# something apt can hand you. This script only checks whether `xc8-cc`
# is on PATH and points at the README if it isn't; host-simulation builds
# (the ones this script actually prepares you for) work without it.

set -euo pipefail

check_only=0
[ "${1:-}" = "--check-only" ] && check_only=1

repo_root="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
problems=0

# ---- host toolchain packages ----
# apt package name : the binary on PATH that proves it's installed.
packages=(
    "cmake:cmake"
    "build-essential:gcc"
    "cppcheck:cppcheck"
    "clang-format:clang-format"
)

missing_pkgs=()
for entry in "${packages[@]}"; do
    pkg="${entry%%:*}"
    bin="${entry##*:}"
    command -v "$bin" >/dev/null 2>&1 || missing_pkgs+=("$pkg")
done

if [ "${#missing_pkgs[@]}" -eq 0 ]; then
    echo "bootstrap: all host toolchain packages present (cmake, gcc, cppcheck, clang-format)."
elif [ "$check_only" = 1 ]; then
    echo "bootstrap: missing packages: ${missing_pkgs[*]}"
    echo "bootstrap: run: sudo apt-get install -y ${missing_pkgs[*]}"
    problems=1
elif command -v apt-get >/dev/null 2>&1; then
    echo "bootstrap: installing missing packages: ${missing_pkgs[*]}"
    sudo apt-get update
    sudo apt-get install -y "${missing_pkgs[@]}"
else
    echo "bootstrap: missing packages: ${missing_pkgs[*]}"
    echo "bootstrap: apt-get not found. Other package managers:"
    echo "  dnf/rpm:  sudo dnf install cmake gcc gcc-c++ make cppcheck clang-tools-extra"
    echo "  pacman:   sudo pacman -S cmake base-devel cppcheck clang"
    echo "  (package names above are Debian/Ubuntu's; adjust to your distro's naming)"
    problems=1
fi

# ---- pre-commit hook ----
if [ "$check_only" = 1 ]; then
    if [ -e "$repo_root/.git/hooks/pre-commit" ]; then
        echo "bootstrap: pre-commit hook already installed."
    else
        echo "bootstrap: pre-commit hook not installed (run ./scripts/install-git-hooks.sh)."
        problems=1
    fi
else
    "$repo_root/scripts/install-git-hooks.sh"
fi

# ---- XC8 / MPLAB X (real-target builds; not automatable, see header) ----
if command -v xc8-cc >/dev/null 2>&1; then
    echo "bootstrap: xc8-cc found on PATH ($(command -v xc8-cc)), real-target builds are ready."
else
    echo "bootstrap: xc8-cc not on PATH. Host-simulation builds (cmake, most of this repo's"
    echo "  tests) work without it. For real-target builds, install MPLAB X + XC8 v3.x"
    echo "  manually, see README.md #Requirements, then add xc8-cc's bin/ to PATH."
fi

if [ "$check_only" = 1 ] && [ "$problems" -eq 1 ]; then
    exit 1
fi
exit 0
