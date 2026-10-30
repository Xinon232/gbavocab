# gbavocab 10.0 — full controls

Learn vocabulary with flashcards and five learning boxes, and create and edit your own word lists on your Game Boy Advance. This manual starts with what is hard to guess; the in-app Settings list (Select in the main menu) has the same topics.

## Quick start

### 1. Where the files go

Put everything directly in the folder `/gbavocab` at the root of the SD card, or press Start in the main menu to import files from anywhere on the card (section 10): your UTF-8 TXT word lists (for example `/gbavocab/Spanish.txt`) and your `.dict` dictionaries. Subfolders are not searched. Each list row is a word, a TAB character and its translation; extra TAB columns are kept. An empty line starts the next learning box, so a list holds Box 1 to Box 5 from top to bottom. This is the dict.cc vocabulary format: lists stay compatible with dict.cc. The separate `sample file.txt` has 20 English–Spanish entries; copy it into `/gbavocab` to try the app.

gbavocab keeps its own settings in `/gbavocab/GBAVOCAB.SYS`: for every list its language pair, learning mode and linked dictionary, and which list you opened last. Nothing is written next to your lists.

### 2. Typing: get GBAWriter first (highly recommended)

Words are typed with the GBAWriter typing system: hold a D-pad direction (and L for a second layer) and tap B, A or R. It is quick once learned. Learn and practise it with GBAWriter first, by the same author: github.com/Xinon232/gbawriter. Sections 3 and 4 describe every typing control.

### 3. The controls you will use most

- Learning: hold R to show the answer; A = right (the card moves up a box), B = wrong (back to Box 1); Left / Right choose Box 1–5.
- Learning: Start alone = save the list. Select alone = home (saves the list first; Settings > Save settings can turn that off). Start + Select = Entry editor. Added, edited and deleted entries are saved at once.
- Entry editor: Add, Edit or Delete the card on screen; Add from dictionary; Show list entries; Link to Dictionary.
- Dictionary: type a word and matches appear. Hold Start + Up / Down to choose a result, Start + A to take it, Start + B to go back. Start alone switches the search language.
- Dictionary: Start + Select opens the dictionary menu: Add entry, Edit entry, Delete entry, Show Dictionary List entries.
- Long lists (List entries, Dictionary list entries): Up / Down move (hold to scroll), Left / Right move a page, Start shows the other language first, A opens Edit / Delete, B goes back. In dictionaries L + Up / Down jumps to the next letter.

### 4. Dictionaries

Build `.dict` files from your own dict.cc or other UTF-8 TAB exports, either on the GBA (Start in the main menu > choose the TXT > Dictionary) or on a PC with the gbavocab dictionary builder (Windows, Linux or macOS, included in every release), then copy them into `/gbavocab`. A list is linked to one dictionary: with only one dictionary on the card the link is made automatically; otherwise you choose it once (Entry editor > Link to Dictionary, or when you first use Add from dictionary).

<!-- PAGEBREAK -->

## 1. Main menu, lists and learning

One ROM runs on the SuperCard SD, EZ-Flash Omega (also the Definitive Edition; start the ROM from the SD card, not from NOR) or EverDrive GBA; the cart is detected at every start (hold L+R while it starts to use the SuperCard driver). Launch `gbavocab.gba` with your flashcart's loader. Back up your TXT lists before using any program that edits them. There is no built-in vocabulary; without an SD card this is reported, and Settings stays available.

The main menu looks like gbamp3's lists: the title `gbavocab` at the top, then **Dictionaries** (with a book icon and a line under it), then every list in `/gbavocab` from A to Z (without `.txt`; case and accents do not change the order). The light-blue bar is the cursor; long names scroll sideways. The list you have open carries a play mark ▶. At start the cursor is on the list you opened last, also after switching the GBA off.

- Up / Down: move; hold to scroll faster. Left / Right: one page. L + Up / Down: first list of the previous / next letter (with Secret Settings > Letter Box On or Only L+DPAD, a box shows the letter).
- A on Dictionaries: your dictionaries (section 5). A on a list: open it; A on the open list returns to it.
- B: back to the open list. While a list is open, the grey bottom line says “B: Resume active list”. If you opened a dictionary from here last, it says “B: Resume active dictionary” and B opens that dictionary again.
- Select: **Settings** (called Help before). Its first items are **New List**, **Flashcard angle**, **Save settings** and **Hide elements** (a line under them); then Quick start, Learning, Entry editor, List entries, Dictionary, Typing, Import and Credits. A opens, Left / Right turn pages, B goes back.
- Start: Import from the SD card (section 10).
- L+R together: **sleep**. The screen goes off and the GBA stops (Stop mode: almost no power, like gbamp3's lock with nothing playing). Start held 1 s wakes it on the same main menu screen (gbamp3's Hold START to unlock): the screen comes back while Start is still down. A shorter press, or a Start that bounces up during that second, leaves it asleep: the screen stays off and nothing is redrawn. The waking Start press does nothing else, and every button stays inactive until all buttons have been released for a moment (a release that bounces does not open Import). Only the main menu sleeps: L+R does nothing new on other screens, L + Up / Down still steps letters, and L+R held while the app starts still chooses the SuperCard driver. The SD card works as before after waking.
- New List picks an unused name from `LIST001.TXT` to `LIST999.TXT` and shows it; A creates that empty list, B returns. Existing files are never overwritten; rename the TXT on a PC for a descriptive name. Then **List languages** asks for the new list's languages: a dictionary (links it and takes its languages), **Type codes** (two codes such as `de` and `en`, for languages without a dictionary) or **Later**.
- Switching lists with unsaved changes asks: A = Save, B = Discard, Select = Cancel. A failed save or load keeps the open list.

<!-- PAGEBREAK -->

### Learning with flashcards

The bottom row shows how many cards are in Box 1 to Box 5; the underline marks the current box. New entries go first in Box 1. The line above the card, for example `Box 1/5 - 20 words`, names the box shown and counts all words of the list. The top line shows which language is asked: `de > en` (German shown, English is the answer), `en > de`, or in the mixed mode the current card's direction with “(mixed)”. Without language codes it says `Front > Back`. Each list remembers its mode; the default is mixed.

- Hold R: show the answer.
- A: right; the card moves up one box (Box 5 stays Box 5). B: wrong; the card returns to Box 1.
- Hold the A or B used for a judgment to keep both sides on screen on the green / red background; after release it stays for about 0.4 seconds.
- Left / Right: previous / next box, wrapping between Box 1 and Box 5.
- Up: undo the last judgment in the same box. Down: shuffle the current box (asks "Shuffle Box N?" with No / Yes rows; B cancels).
- L: switches the asked language: `de > en`, `en > de`, mixed.
- Settings > Hide elements hides the top line (mode), the `Box N/5 - T words` line or the box counts at the bottom; everything else stays where it is, and the boxes still work.
- Settings > Flashcard angle turns this screen (90° right, 90° left or upside down) for holding the GBA sideways or upside down. The D-pad turns with it: Left / Right / Up / Down mean the directions on the turned screen. A, B, R, L, Start and Select stay the same. The Shuffle question turns too; menus, the dictionary and the Entry editor stay upright.

### Saving and switching (press and release)

- Start alone, then release: save the list, including boxes and learning progress. Added, edited and deleted entries are saved at once, and Select (main menu) saves the list first unless Settings > Save settings > Save list on menu is Off. While a save runs, “save...” shows at the top right; wait until it is gone before switching off.
- Select alone, then release: home.
- Start + Select together: Entry editor (either press order; releasing does not also save or go home).

Error notices stay on the card for three seconds. They do not clear a failed save or unsaved changes; keep the recovery files after a failure.

<!-- PAGEBREAK -->

## 2. Entry editor, List entries and linking

From learning press Start + Select. The menu is a list like the main menu (light-blue bar, no instruction line; a message such as a failed save is the grey bottom line). Up / Down choose, A opens, B returns to learning. The menu has seven rows:

- Add entry: a new entry goes first in Box 1.
- Edit entry / Delete entry: the card that was on screen. An empty box has no card to edit; Add still works.
- Add from dictionary: open the linked dictionary and take a result as a new entry draft.
- Show list entries: every entry of the list (see below).
- Link to Dictionary (Link to new Dictionary when a dictionary is linked), with the list's languages, for example “Link to Dictionary (de / en)”: choose the `.dict` this list uses, or **Type codes** for languages without a dictionary. Without any `.dict` on the card the row is called “Languages” and only offers Type codes.
- Type first: which language step 1/2 of Add entry and Edit entry asks for, for example “Type first: de”. A switches it; the list remembers it.

### Add and Edit drafts

Each step shows the language it asks for under the title (for example `de`, then `en`; “Word / front” and “Translation / back” while the list has no language codes). Step 1/2 asks for the language set in Type first, step 2/2 for the other one; each word is stored in its own language's column either way. Edit fills in the card's two fields; extra columns are kept unchanged. Start + A in step 1 goes to step 2; Start + A in step 2 applies the entry. Start + B in step 2 returns to step 1; Start + B in step 1 cancels. Start can stay held between the steps: Start + A, let go of A, A again. Both fields need text. Applied changes are saved to the list at once (“save...” at the top right while it runs), unless Settings > Save settings > Save after entry changes is Off; then they wait in RAM until Start. Edit keeps the entry's box, place and progress.

Delete from the menu shows the entry and asks “Are you sure?”: Left / Right choose No / Yes, A confirms, B cancels. Deleting the last entry leaves an empty list that can take new entries.

### Show list entries

List entries shows every entry on one line each, as `word = translation`, from top to bottom exactly as in the TXT file. Lines between entries mark where a box ends and the next begins (for example “Box 1 above / Box 2 below”). They cannot be selected. Long entries scroll sideways while selected.

If the list has unsaved changes, the menu row first asks “Saving list first. OK?”: A saves and opens the list, B stays in the Entry editor. Saving first means the order shown is the file's order.

- Up / Down: move; hold to scroll faster. A fresh press past the first or last entry wraps around.
- Left / Right: one page back / forward; the cursor keeps its place on the screen.
- Start: show the translation first (and back again).
- A: Edit entry or Delete entry for that entry. Delete asks “Delete entry? A: Yes  B: No” in its row.
- B: back to the Entry editor.

<!-- PAGEBREAK -->

### Linking a dictionary

Each list is linked to one dictionary and has two language codes (which language each column of the TXT holds), remembered in `GBAVOCAB.SYS`; the TXT is never changed. With only one valid dictionary on the card, Add from dictionary links it automatically. With several, Add from dictionary asks once; Link to Dictionary lets you change it at any time; its list marks the linked dictionary with ▶. Taking a word from a dictionary the list is not linked to (main menu > Dictionaries, or Start + R in the search) asks “List is not linked to this dictionary. Create link and go on?”: Yes links it and adds the word; No goes back.

When a list gets a dictionary (or typed codes) of languages it does not have yet:

- An empty list simply takes the dictionary's languages.
- A list with cards shows one of its cards and asks, for example, “Hund = dog. Which is German?”: choose the German word. **Other card** shows another random card (useful for words that are the same in both languages, such as names); B cancels and nothing changes.
- A list whose codes already match (in either order) links at once when you add words from a dictionary. When you choose the dictionary or Type codes yourself (Entry editor > Link to Dictionary), a list with cards is always asked: that is how you fix a list whose two languages are the wrong way round.

Words from a dictionary always go into the right columns, whichever way you searched. The link and languages are saved at once.

<!-- PAGEBREAK -->

## 3. Typing letters, case, spaces and caret

The drafts use GBAWriter's button-driven typing, not an on-screen keyboard. Hold one D-pad direction, then tap B, A or R to choose letter 1, 2 or 3. Release the letter button between taps. Multiple D-pad directions are not a letter group.

### Letter groups: B / A / R in that order

- Up: a / b / c
- Right: h / i / j
- Down: n / o / p
- Left: t / u / w
- L + Up: d / e / f
- L + Right: k / l / m
- L + Down: q / r / s
- L + Left: x / y / z

For g: keep Right held and tap R twice. The first tap inserts j; the second replaces it with g. For v: keep Left held and tap R twice; w becomes v. This is a continuous-held-group gesture, not a fast double-tap deadline. Releasing the direction or L resets the sequence; A, B, Start or Select also interrupts it. The replacement keeps the case of the first letter.

### Spaces, deletion and repeat

A alone inserts a space at the caret. B alone deletes the preceding complete UTF-8 character, not an individual byte. Hold either button alone to repeat: the first action occurs on press, repeat starts after 24 frames and continues every 5 frames. Repeat must begin with a fresh solo press and stops when another button joins; releasing a letter chord into A/B does not start repeat. Letter and Select-symbol presses themselves do not auto-repeat.

### Shift and Caps

From normal, a short isolated R release arms Shift. Hold R alone for 48 frames (about 0.8 seconds) to enable Caps while held, once per hold. Any companion before the threshold cancels eligibility until R is released and freshly pressed alone. From active Shift/Caps, another isolated R—short or long—clears on release and cannot rearm in that hold. Shift uppercases the next accepted letter; Caps stays active. Spaces, digits and punctuation do not consume Shift. Select-first accented letters consume one-shot Shift when the Select session is released. Letter-first conversion retains the original letter's case. The ß alternate remains ß even with Shift / Caps. Rejected insertion does not consume Shift. The status line shows the active letter group and Shift or Caps.

### Caret and visual rows

Hold Start and press Left / Right to move one UTF-8 character backward / forward. Start + Up / Down moves to the previous / next visual row, keeping the horizontal position where possible. Start + L / R moves by a page of draft rows. Hold Start with one navigation key to repeat on the same 24-frame / 5-frame schedule. Movement stops at text boundaries.

Draft text wraps visually without inserting line breaks; overlong words split visually. Start alone attempts a newline on release, but vocabulary fields reject it: no tabs, carriage returns or newlines may be typed. There is no Home/End, selection, clipboard or forward-delete shortcut.

<!-- PAGEBREAK -->

## 4. Punctuation, digits, signs and accents

Press Select outside a still-held producing letter chord to insert one provisional period at the caret. Keep Select held while choosing a replacement below; release Select to keep it in the draft. Each cycle replaces that same character. Release Select and begin again for a second symbol.

### Symbol cycles while Select is held

- Up moves forward and Down backward through: 1 2 3 4 5 6 7 8 9 0
- Right moves forward and Left backward through: . ( ) / ; @ # % & _ + = -
- R alone moves forward and L alone backward through: . , ' " : ! ?

Cycles wrap around. Starting from the provisional period, Up selects 1 and Down selects 0; Right selects an opening parenthesis and Left selects a hyphen; R selects a comma and L selects a question mark. For the L/R punctuation cycle, release any D-pad direction first.

### Accent cycles while Select is held

Use the normal letter chord for a supported base letter to replace the provisional character with its first alternate. Repeat that letter button while keeping its direction (and L layer, where needed) held to cycle the remaining alternates. Changing or releasing the group resets that letter's cycle. A letter with no listed alternate does not insert its plain form in this mode.

- a: á ä à â ã å æ     c: ç č ć
- e: é è ë ê     i: í ï ì î
- n: ñ ń     o: ó ö ô ò õ ø œ
- s: ß š ś     u: ü ú ù û
- y: ý ÿ     z: ž ź ż

Shift / Caps selects the corresponding uppercase accented form, except ß which stays unchanged. For example, hold Select and Up, then tap B once for á or twice for ä. Release Select to keep it.

Both press orders work. Hold Select, then enter the letter combination. Or keep the just-typed letter's exact direction, producing B/A/R button and L layer (if used) continuously held, then press Select. The same letter becomes its first alternate, keeping its case, with no duplicate letter or period. No time deadline applies; releasing or changing the chord ends eligibility. Example: Up+B held, then press Select: a becomes á. Keep Select and the group held; release and repress its producing B/A/R button to cycle, then release Select to keep the result.

### Status toggle and modifier priority

Start + Select inside a draft toggles the status bar (in the dictionary search it opens the dictionary menu instead). It removes a newly inserted Select provisional character, but retains an existing letter converted to an accent. While Select is held, typing belongs to the symbol / accent session: release Select before Start + A or Start + B. A plain B in normal typing is backspace, not cancel.

<!-- PAGEBREAK -->

## 5. Dictionaries

Dictionaries are indexed `.dict` files in `/gbavocab`, built on a PC (section 7); up to 24 are found. There are no built-in dictionaries. Open them from the main menu > Dictionaries, or from a list's Entry editor > Add from dictionary (the list's linked dictionary). Main menu > Dictionaries lists every dictionary with its languages in the same list style as the main menu (A opens its search, B returns); the dictionary you opened last carries ▶ and the cursor starts on it. With only one dictionary its search opens directly. B in the search returns to that list.

The search screen has no header and no instruction line, so three results fit. The typed word is at the top; the bottom line shows the search direction (for example `en > de`), the number of matches and the current letter group or Shift / Caps.

- Type with the normal typing controls: matching words appear as you type.
- Hold Start + Up / Down: choose a result (the light-blue bar); release Start to type again.
- Start + A: take the selected result. From home you then choose the list it goes to; from Add from dictionary it becomes a new entry draft.
- Start + B: back.
- Start alone (press and release): search the other language.
- Start + R: the next dictionary at once, A to Z, back to the first after the last. Your search text stays, and the bottom line shows the dictionary's name for two seconds. From Add from dictionary only dictionaries for the list's languages take part; switching does not change the link (Entry editor > Link to new Dictionary does). With one dictionary Start + R does nothing.
- Start + Select: the dictionary menu.

ASCII case and Latin accents are ignored when searching: m / ma / … / manana find mañana; cafe finds café. Other scripts, spacing and punctuation must match exactly. Results always keep the original spelling.

### Dictionary menu (Start + Select)

- Add entry: a new dictionary entry in two steps, as a list entry.
- Edit entry: the selected result.
- Delete entry: the selected result; the row first asks “Delete entry? A: Yes  B: No”.
- Show Dictionary List entries: the whole dictionary (below).

B closes the menu. Edit and Delete need a result on screen; type to find it first.

### Dictionary list entries

The whole dictionary, one entry per line, in alphabetical order of the shown language. Your own added and edited entries appear at their alphabetical place.

- Up / Down: move; hold to scroll faster; a fresh press at either end wraps.
- Left / Right: one page back / forward.
- L + Up / Down: first entry of the previous / next letter. The letter box is Off by default; Secret Settings (below) turns it on.
- Start: the other language first, sorted by that language; the selected entry stays selected.
- A: Edit entry or Delete entry. B: back to the search.

### First use with a list

A list without languages gets them from the dictionary when it is linked (see Linking a dictionary): an empty list takes the dictionary's order; a list with cards asks once with a sample card. Words found in either search direction go into the list's own columns.

<!-- PAGEBREAK -->

## 6. Changing dictionaries on the GBA

Add, Edit and Delete in a dictionary are saved immediately into the same `.dict` file. Enter the shown first language, Start + A, then the second language and Start + A to save. Start + B goes back a step or cancels. A failure keeps the draft for another try. Saving never rewrites the TXT list or the original dictionary data: changes are appended.

Each `.dict` file has room for 512 changes (adds, edits and deletes). When it is full, open the file in the PC builder and Save As a new file: this merges the changes and frees the room. Incomplete changes from an interrupted save are ignored and never overwritten.

Back up `.dict` files. Never switch off or remove the card while saving, and do not change a file on a PC while it is open on the GBA.

### Old dictionaries

This version needs v4 `.dict` files (from builder 2.0 or newer). Back up an old dictionary, open it in the PC builder, choose Save .dict as with a NEW filename and copy that file to `/gbavocab`. The builder keeps all live entries, including those added or edited on the GBA, and removes deleted ones.

### Search speed and size

Each dictionary keeps a sorted copy of both directions in 4 KiB blocks, so the GBA reads only a few blocks per search and never the whole file. Up to 64 MiB and 40,000+ entries per dictionary are supported, independent of the 15,000-entry limit of a list. The format is custom (not StarDict); see `docs/dictionary-format.md`.

In the search, Start + Left / Right move the cursor in the typed text.

<!-- PAGEBREAK -->

## 7. PC standalone dictionary builder

The builder is available at halimj.itch.io. Extract the native Windows, Linux or macOS package and launch gbavocab-builder.exe, gbavocab-builder or gbavocab-builder.app. On a Mac choose the Apple Silicon (M1 and newer) or Intel package; the app is not notarized, so the first time Control-click it and choose Open (or use System Settings > Privacy & Security > Open Anyway). No compiler, ROM template or network is needed. Builder 2.0.0 Import TXT / TSV / ZIP accepts legally obtained UTF-8 TAB-separated exports. Detection uses comment headers, not filenames: dict.cc DE-ES suggests German-Spanish, de/es and German/Spanish labels. All identity fields remain editable. Choose Save .dict as, then copy the result directly into /gbavocab.

The first two TAB-separated fields are preserved, including multiword text, spaces, braces, brackets, parentheses, accents, gender/usage labels and punctuation. Additional dict.cc columns stay in review metadata, not vocabulary output. BOM, LF/CRLF, comments, blank and decorative separator lines are tolerated. Source/attribution comments are visible above the table. Names use 1..31 UTF-8 bytes; labels 1..23; distinct codes use 1..11 lowercase ASCII letters/digits/hyphens starting with a letter. Each pair is at most 191 UTF-8 bytes including TAB.

Review All or Flagged in pages of 100 rows. Columns show original source line, both fields, byte length, issue reasons and inclusion state; double-click for full text and extra metadata. Missing/blank fields, invalid controls and oversized pairs are collected together. Exact duplicate pairs are warnings, not invalid; distinct translations of the same headword remain separate.

Remove flagged oversized entries excludes only those rows after confirming the count. Remove selected and Remove exact duplicates have separate confirmations; duplicate removal keeps the first included copy. No rows are automatically removed or truncated. Undo reverses the last exclusion/reset; Reset exclusions restores every original row. Save is disabled while included invalid rows remain or output is empty; saving revalidates strictly and checks both indexes by reading the produced file. The suggested filename follows the language labels (for example German-Spanish.dict).

Import and build run in background workers. Failed imports/builds retain your review work. Replacing/closing unsaved work asks for confirmation. Output uses staged atomic replacement and never the source path.

ZIP members are read directly, never extracted onto disk. Choose one TXT/TSV member if several are present. Encrypted members, unsupported compression, corrupt ZIPs and invalid UTF-8 are rejected. Bounds are 64 MiB compressed archive, 64 MiB selected uncompressed text, 256 archive members and 250,000 vocabulary rows. No automatic network downloads. Respect source licensing: dict.cc private-use exports and derived dictionaries must not be published or given away.

<!-- PAGEBREAK -->

## 8. Files, limits and saving

### TXT lists

Saving writes the boxes in order, Box 1 to Box 5, with exactly one empty line between boxes (four in total, also around empty boxes), using CRLF line ends. This keeps the lists dict.cc compatible; the dict.cc website may drop empty boxes on a round trip. A legacy footer line such as `# gbavocab: front=en; back=de` is moved into `GBAVOCAB.SYS` and removed from the TXT on the next save.

Both fields must contain text. A whole row may have at most 191 bytes including the TAB and extra columns; accented and other non-ASCII characters use several bytes. A list holds up to 15,000 entries. Invalid or overlong rows make a list read-only with a warning rather than losing them on a save. Filenames over 54 bytes need renaming on a PC before saving.

RAM holds up to 128 added or edited rows that are not yet saved; deleting uses none. When full, “RAM FULL - save list first” keeps your draft: note it, go back to learning, save with Start, and enter it again.

### Settings: GBAVOCAB.SYS

`/gbavocab/GBAVOCAB.SYS` holds two copies of the settings (`CONFIG0.DAT`, `CONFIG1.DAT`). Each save writes the older copy, so an interrupted save keeps the previous settings. For every list (by filename, up to 192 lists; the one used longest ago makes room) it stores the two language codes, the learning mode, Type first and the linked dictionary, plus the last opened list. Renaming a list on a PC starts it with default settings. Deleting the folder resets all settings; lists and dictionaries are not affected. Old per-list `.sav` files from earlier versions are no longer read.

### Saving safely

A save writes a temporary file, checks it, then replaces the list. Temporary `name.txt.gbv1.tmp` / `.bak` / `.txn` files (slots 1–9) are used for recovery after an interruption; keep them after an error and back up before any manual repair. Never remove power or the card while saving, and do not change or swap the open list on a PC. Saves check writes and scan the installed TXT; same-size external edits can escape detection, so keep backups. See `docs/file-io-simple-save.md` for tested failure cases.

Test backed-up copies of your lists on your own SD card and firmware: emulator and host checks do not prove real flashcart storage.

### Fonts on screen

Latin Extended, Greek, Cyrillic, Japanese kana, CJK and composed Hangul are shown from the SuperFW font packs; coverage is large but not every Unicode character. Arabic is described in the next section.

<!-- PAGEBREAK -->

## 9. Imported Arabic

Arabic may appear in either field, mixed with Latin transliteration, punctuation and numbers. Ghoulam supplies contextual Arabic letter forms and lam-alef ligatures. Logical text wraps first; each resulting line arranges Arabic runs right to left, with Latin and number runs left to right. Paired parentheses, brackets and braces keep enclosed transliteration together within a line. Slash-separated alternatives remain separated.

Harakat are transparent to joining and hidden only in display. The original UTF-8 letters, marks and punctuation remain in the TXT and editor drafts. Ghoulam's empty Arabic-comma glyph displays using the ordinary SuperFW comma instead. Arabic-Indic digits display using SuperFW digits. Other unsupported meaningful characters display a visible question mark. This is bounded Arabic support, not a complete Unicode bidirectional or Arabic-script engine; Persian/Urdu extensions, presentation-form text and explicit bidi embedding controls are not comprehensive coverage. The one-line List entries and Dictionary list entries show Arabic letters unjoined.

### Editing imported text

The two-field editor and Delete preview use shaped Arabic too. No Arabic keyboard layout has been added. Start + Left / Right still means previous / next logical UTF-8 character, not necessarily left / right on screen. Start + Up / Down chooses a character boundary near the same visual horizontal position in the adjacent wrapped row. A lam-alef ligature has an interior caret position; deleting removes one logical character at a time. Hidden marks keep their logical positions and can require a navigation or deletion step without a visible movement.

The prompt and revealed answer still share a single screen. Short Latin cards retain their appearance; long cards may use reduced text. Arabic artwork is rasterized at the font's native 11-pixel scale before any whole-line reduction, retaining its baseline, bearings and connected advances. Reduced text is necessarily small on the GBA screen.

### Ghoulam extraction notice

For this app the supplied TTF's actual unencoded GSUB initial, medial, final and lam-alef glyphs were extracted as monochrome ROM bitmap tables at 11 pixels. This conversion, display-only mark filtering and punctuation/digit fallbacks are app changes, not a modified font endorsed by the author. SuperFW remains the font for Latin, numbers and ordinary punctuation. The extractor is included in the source code, and the generated table records the source font's SHA256. Credits and license: About / licence, last page.

<!-- PAGEBREAK -->

## 10. Import from the SD card

Press Start in the main menu. The browser starts at the root of the SD card and shows folders (with a `/`), TXT files and `.dict` files; folders come first. Up / Down move, Left / Right turn a page, A opens a folder or imports a file, B goes up a folder; B at the root returns to the main menu. Hidden files and the settings folder are not shown.

- A `.dict` file is copied into `/gbavocab` at once.
- A TXT file asks “Import as”: **List** or **Dictionary**.
- **List** copies the TXT into `/gbavocab`; it then appears in the main menu. A list holds at most 15,000 entries: a bigger file is refused with “Too big for a list: 15,000 entries max.” and you may import it as a dictionary instead.
- **Dictionary** asks “Create a dictionary from …? This can take some minutes.” A continues, B goes back.
- A TXT that is already in `/gbavocab` is a list already: choosing it goes straight to the dictionary question, which says that the list stays unchanged.

Nothing is ever replaced or deleted. When the name is already used in `/gbavocab`, the app asks to import a numbered copy instead, for example `Spanish (2).txt`. The original file always stays where it is. A list name may have up to 54 bytes; rename longer files on a PC.

### Building a dictionary on the GBA

The GBA builds the same version-4 `.dict` as the PC builder, named like the TXT (`German-English.txt` becomes `German-English.dict`). Large exports take minutes: the screen shows four steps (reading the file, sorting the first language, sorting the second language, writing the dictionary) with a percentage. Do not switch off. The rows are sorted on the SD card, so the card needs free space of about seven times the TXT size while building; the temporary files are removed afterwards.

Rows that a dictionary cannot use are skipped automatically, with no review screen: pairs over 191 bytes, missing or empty fields, control characters, invalid UTF-8, and exact duplicate pairs (the first copy is kept). Comments, blank lines, decorative lines and “Box 1”–“Box 5” lines are ignored, extra columns are dropped. The end screen shows the number of entries and how many rows were skipped as too long, invalid or duplicate.

A dict.cc export names its languages in its first lines (for example `# DE-EN vocabulary database … dict.cc`); the GBA reads them and names the dictionary, for example German-English. Without such a header you type the two language codes with the normal typing controls: the first language in step 1/2, Start + A, the second in step 2/2, Start + A builds; Start + B goes back. Codes are 1–11 lowercase letters, digits or hyphens, starting with a letter (for example `de`, `en`, `es`).

Exact duplicates are recognised among entries with the same first-language word (up to about 300 of them per word). Back up your files; a power cut while building leaves only temporary files in `/gbavocab/GBAVOCAB.SYS`, which the next build removes.

<!-- PAGEBREAK -->

## Settings

Select in the main menu opens **Settings** (called Help before): New List, Flashcard angle, Save settings and Hide elements, a line, then the help topics and Credits.

### Flashcard angle

Settings > Flashcard angle, the second item (the row shows the angle, for example "Flashcard angle: Normal"). A opens a list like gbareader's Angle screen: Normal, 90° right, 90° left and Upside down, the angle chosen ticked; under a line, what it does. Up / Down move, A chooses (saved at once in `GBAVOCAB.SYS`), B returns to Settings.

- Only learning turns: the cards, the language line, the box counts with their underline, “save...” and the green / red backgrounds, and the Shuffle question. At 90° the screen is laid out 160 wide and 240 high: lines are shorter, and the extra height (and a larger share of sprite memory for the card) keeps long cards at full size more often than upright.
- The D-pad turns with the screen (as in gbareader): with 90° right the D-pad is on top and its Up points right, so Up is the next box. A, B, R, L, Start and Select are unchanged.
- Main menu, Settings, the dictionary, Import and the Entry editor stay upright.

### Secret Settings

Like gbamp3's: Settings > Credits, turn to the last page (Secret Settings) and press A. The setting is at the top, a line under it, and below the line what the values do. A changes the value; B goes back. The choice is saved at once in `GBAVOCAB.SYS`.

- Letter Box: Off (default): no letter box. On: the box shows the current letter on L + Up / Down, on pages, on wraps and while the list scrolls fast. Only L+DPAD: the box shows only on L + Up / Down.

<!-- PAGEBREAK -->

### Save settings

Settings (Select in the main menu) > Save settings, the third item, under Flashcard angle. The two switches are at the top (Up / Down choose), a line under them, and below the line what they do. A changes the value; B returns to Settings. The choice is saved at once in `GBAVOCAB.SYS`.

- Save after entry changes: On (default): Add, Edit and Delete (Entry editor, List entries) save the list at once. Off: they wait in RAM (up to 128 rows) until Start.
- Save list on menu: On (default): Select from learning saves the open list before the main menu opens. Off: it is saved only by Start (and asked about when you open another list).

### Hide elements

Settings (Select in the main menu) > Hide elements, the fourth item, under Save settings. Three switches at the top (Up / Down choose), a line under them, and below the line what they do. A changes the value; B returns to Settings. The choices are saved at once in `GBAVOCAB.SYS`; all are Off by default.

- Hide Mode: On hides the asked language at the very top of the learning screen (for example `de > en`).
- Hide Total Word count: On hides the line `Box N/5 - T words`.
- Hide Box indicator: On hides the box counts and the underline at the bottom. The boxes still work as before: Left / Right (on a turned screen, the turned directions) change the box.
- A hidden part only leaves its place empty: the card and the other parts stay at exactly the same place, at every Flashcard angle.

### Notes

The main menu shows `gbavocab` without a version number; the version is on the first Credits page. Settings (Select in the main menu) is a list like the main menu: New List, Flashcard angle, Save settings and Hide elements first (a line under them), then Quick start, Learning, Entry editor, List entries, Dictionary, Typing, Import and Credits (Up / Down, A opens, B back). Its pages look like gbareader's About pages: the title at the top with the page number on the right, one line per row, grey subheadings and no instruction line; Left / Right turn the pages and B returns to the Settings list. Quick start includes a Controls page with the keys you need most. This manual is generated from `docs/full-controls.md` into `gbavocab-full-controls.pdf` for every release.

Back up your lists and dictionaries regularly. gbavocab comes with no warranty; see the GPL for details. Thank you for learning with gbavocab — and for trying GBAWriter for faster typing.

<!-- PAGEBREAK -->

## About / licence

**gbavocab 10.0** · (c) 2026 Halim Jarrar · halimj.itch.io · gba@halim-jarrar.de

gbavocab is free software: you can redistribute it and/or modify it under the terms of the GNU General Public License, version 3 or later (GPL-3.0-or-later). The full license text is in `LICENSE`. gbavocab comes with no warranty; see the GPL for details.

Source code: github.com/Xinon232/gbavocab

### Credits

Text renderer: SuperFW by David Guillen Fandos, © 2024–2025. License: GPL 3.0 or later. https://superfw.davidgf.net/

Fonts: the SuperFW font packs use UNSCII by Viznut (https://viznut.fi/unscii/) and GNU Unifont (https://www.unifoundry.com/unifont/), including the composed Hangul components. License: GPL 2.0 or later with the GNU font embedding exception.

Arabic font: Ghoulam Regular © 2025 Imad AlFil / mloukhiyye. License: CC BY 4.0 (https://creativecommons.org/licenses/by/4.0/). Changed: the actual contextual forms were converted to an 11 px bitmap font for gbavocab; this modified representation is not author endorsement. Source: https://mloukhiyye.itch.io/ghoulam-arabic-pixel-art-font-version-1

SD card drivers (gbaSD, github.com/Xinon232/gbaSD): SuperCard driver from SuperFW by David Guillen Fandos, GPL 3.0 or later; EZ-Flash Omega and EverDrive GBA drivers by Halim Jarrar (GPL 3.0 or later), written after the Omega kernel (Apache 2.0) and edio-gba by krikzz (MIT). File system: FatFs by ChaN. License: FatFs (BSD style).

Engine: Butano by Gustavo Valiente. License: zlib. Built with devkitPro / devkitARM.

dict.cc is a separate website; gbavocab only reads and writes its TAB-separated vocabulary format. Respect the dict.cc terms for your own exports.

In the app, Settings > Credits has the same order on seven pages: Credits (version, copyright, license, source code and contact), License, Text and fonts, Arabic font, SD card and files, Engine and Secret Settings (A opens them). Left / Right turn the pages; B goes back.
