#!/usr/bin/env bash
# scripts/install-git-hooks.sh, one-time setup: symlinks the pre-commit hook
# into .git/hooks/ (not itself tracked by git, so this install step is
# needed once per clone). See scripts/README.md for what the hook checks.

set -euo pipefail

repo_root="$(git -C "$(dirname "${BASH_SOURCE[0]}")" rev-parse --show-toplevel)"
hook="$repo_root/.git/hooks/pre-commit"

ln -sf ../../scripts/pre-commit-checks.sh "$hook"
chmod +x "$repo_root/scripts/pre-commit-checks.sh"

echo "Installed: $hook -> scripts/pre-commit-checks.sh"
