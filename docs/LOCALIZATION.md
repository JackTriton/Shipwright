# Localization

UI text and host-created game messages have separate language settings.
The editable catalogs live in `soh/assets/custom/ui-lang/` and
`soh/assets/custom/game-lang/`. The English catalog defines the key schema;
missing entries fall back to English. Built-in recovery catalogs are generated
from these JSON files at configure time. Keep the ROM language/resource index
separate from a catalog's locale identifier.

## Editing catalogs

- Keep keys, numbered option values, placeholders, printf arguments, ImGui
  `##`/`###` IDs, tabs, and NUL-separated choices intact. Description arrays
  concatenate in order, so write explicit `\n` for intended line breaks.
- UI option labels are presentation only; the menu still stores the original
  numeric value or CVar. Translate labels without changing that identity.
- Game message `&` starts a line, `^` starts a page, `@` inserts the player name,
  and `_` is a space. `[[byte:HH]]` denotes a native control byte. Preserve
  control operands and authored page breaks when editing message bodies.
- `game-lang/ja_JP.json` owns layout for **added** Japanese messages under
  `message_layout`. Alignment may be `auto`, `left`, `center`, or `right`.
  Rules follow the same nested paths as `messages`; the nearest rule wins.
  `auto` centers plain text and keeps choice messages left aligned. Item-icon
  pages keep native placement regardless of the alignment rule. Existing ROM
  messages are unaffected.
- Added Japanese messages use the existing game font for Japanese characters
  and native Latin glyphs for ASCII. Their width and line wrapping use the
  actual glyph advances. The committed `JapaneseMessageFontTable.inc` maps
  Unicode to the native glyph IDs in `soh/assets/textures/kanji/kanji.h`;
  it contains no font images. An unavailable glyph uses the native fallback.
- French and German UI catalogs follow the English key order. Prefer the
  supplied European OoT text for established item, character, and location
  names; names of new randomizer features need separate translation review.
  Technical IDs and product names may intentionally remain untranslated.
- The Japanese Roc's Feather pause-menu name uses
  `soh/assets/custom/textures/item_name_static/gRocsFeatherItemNameJPNTex.ia4.png`
  when that image is present in the archive; otherwise it uses the English
  image. The image is a separate asset, not part of the text patches.

Translation and formatting happen when a message is built or decoded; glyph
layout is not recomputed each frame. More languages can use catalog locale
IDs, but rendering profiles and fonts for other writing systems require
separate implementation.

## Building and checking

After editing a catalog, rebuild the asset archive and application, then
restart with the newly generated `soh.o2r`:

```sh
cmake --build build-cmake --target GenerateSohOtr
cmake --build build-cmake
```

Review translations beside the corresponding English JSON values. Verify
numeric choices and formatting markers, then inspect long labels, choice
positions, icon pagination, and Japanese/Latin mixed text in the game.
The source changes do not modify original ROM dialogue or add new fonts.
