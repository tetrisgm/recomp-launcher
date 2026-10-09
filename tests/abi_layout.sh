#!/usr/bin/env bash
# abi_layout.sh <our-src> <recomp-ui-src>: every public struct must have the
# same size and field offsets in both headers (clang record-layout dump).
set -euo pipefail
ours="$1"; ref="$2"; cc="${CC:-clang}"
tmp="$(mktemp -d)"; trap 'rm -rf "$tmp"' EXIT
types=$(grep -oE '^typedef struct RecompLauncherC[A-Za-z]+|^struct RecompLauncherCSettings' "$ours/recomp_launcher.h" \
        | awk '{print $NF}' | sort -u)
dump() {
  { echo '#include "recomp_launcher.h"'
    for t in $types; do
      if [ "$t" = RecompLauncherCSettings ]; then echo "struct $t v_$t;"; else echo "$t v_$t;"; fi
    done; } > "$tmp/$2.c"
  "$cc" -c -o /dev/null -Xclang -fdump-record-layouts -I"$1" "$tmp/$2.c" | grep -E "^ *[0-9:-]+ \| |sizeof=" > "$tmp/$2.txt"
}
dump "$ours" ours
dump "$ref" ref
if diff -u "$tmp/ref.txt" "$tmp/ours.txt"; then
  echo "abi_layout: $(echo $types | wc -w | tr -d ' ') structs identical"
else
  echo "abi_layout: layout differs from $ref" >&2; exit 1
fi
