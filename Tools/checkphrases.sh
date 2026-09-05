#!/bin/sh
# Every string the UI draws goes through T(), which falls back to the English
# source when the phrase table has no entry. That fallback is silent: a missing
# entry looks exactly like a deliberate English label, so Korean players get a
# line of English and nothing says why.
#
# This lists literals handed to UiText / DrawCentered / T() that the table does
# not key. Judgement words (PERFECT, BURST, SWAP...) are meant to stay English,
# so expect some of these to be fine - it is a list to read, not a test to pass.
#
#   sh tools/checkphrases.sh [path/to/main.c]

SRC="${1:-LowByteGame/LowByteGame/main.c}"
[ -f "$SRC" ] || { echo "cannot read $SRC"; exit 1; }

TMP="${TMPDIR:-/tmp}/checkphrases.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# Table keys.
awk '/^static const Phrase phrases\[\] = \{/,/^\};$/' "$SRC" \
  | grep -o '{ "\([^"]*\)"' | sed 's/{ "//; s/"$//' | sort -u > "$TMP/keys"

# Literals the UI draws directly. Only the first argument is a phrase; a
# TextFormat wrapper means the literal is a format string, which is keyed too.
grep -o 'UiText("[^"]*"' "$SRC"        | sed 's/UiText("//; s/"$//'        >  "$TMP/used"
grep -o 'DrawCentered("[^"]*"' "$SRC"  | sed 's/DrawCentered("//; s/"$//'  >> "$TMP/used"
grep -o 'T("[^"]*"' "$SRC"             | sed 's/T("//; s/"$//'             >> "$TMP/used"
sort -u "$TMP/used" | grep -v '^$' > "$TMP/used.s"

missing=$(comm -23 "$TMP/used.s" "$TMP/keys")
if [ -n "$missing" ]; then
    echo "drawn but not in the phrase table (shows English in Korean):"
    echo "$missing" | sed 's/^/  /'
else
    echo "every drawn literal has a phrase entry."
fi
echo ""
echo "keys: $(wc -l < "$TMP/keys" | tr -d ' ')   drawn literals: $(wc -l < "$TMP/used.s" | tr -d ' ')"
