#!/bin/sh
# Card names are localized through the phrase table, which is keyed by the
# English string. A key that does not match cards[] character for character
# does not fail the build - it silently leaves that one card in English, and
# with sixty two of them that is easy to miss.
#
# This compares the two lists and prints anything that appears in one but not
# the other. Silence means they agree.
#
#   sh tools/checknames.sh [path/to/main.c]

SRC="${1:-LowByteGame/LowByteGame/main.c}"
[ -f "$SRC" ] || { echo "cannot read $SRC"; exit 1; }

TMP="${TMPDIR:-/tmp}/checknames.$$"
mkdir -p "$TMP" || exit 1
trap 'rm -rf "$TMP"' EXIT

# Names as the game defines them.
awk '/^static const Card cards\[\] = \{/,/^\};$/' "$SRC" \
  | grep -o '^    { "[^"]*"' | sed 's/^    { "//; s/"$//' | sort > "$TMP/cards"

# Names as the phrase table keys them, i.e. the entries whose key is also a
# card name. Comparing the whole table would report every ordinary phrase.
awk '/^static const Phrase phrases\[\] = \{/,/^\};$/' "$SRC" \
  | grep -o '{ "[^"]*"' | sed 's/{ "//; s/"$//' | sort -u > "$TMP/keys"
comm -12 "$TMP/cards" "$TMP/keys" > "$TMP/have"

missing=$(comm -23 "$TMP/cards" "$TMP/have")
[ -n "$missing" ] && {
  echo "in cards[] but not in the phrase table (these stay English):"
  echo "$missing" | sed 's/^/  /'
}

untranslated=$(awk '/^static const Phrase phrases\[\] = \{/,/^\};$/' "$SRC" \
  | grep -o '{ "\([^"]*\)", "\1" }' | sed 's/{ "//; s/",.*//' | sort > "$TMP/same"
  comm -12 "$TMP/have" "$TMP/same")
[ -n "$untranslated" ] && {
  echo "still showing the English name (second column not filled in):"
  echo "$untranslated" | sed 's/^/  /' | tr '\n' ' '
  echo ""
}

n=$(wc -l < "$TMP/cards" | tr -d ' ')
h=$(wc -l < "$TMP/have" | tr -d ' ')
echo "cards: $n   keyed: $h"
[ -z "$missing" ] && echo "every card name has a phrase entry."
