# Essence Designer

A small browser tool for authoring the Essence Mod's content: tags, damage types, essences and
powers. It is not part of the game; it edits the data file the game's content comes from.

- **Start it:** run `designer.bat` in the folder above this repository, or `npm run dev` here.
  It opens at http://localhost:5734. The first time, run `npm install` here.
- **Where content is saved:** `designer/essence_data.json` at the top of this repository.
  A safety copy of the previous version goes into `designer/backups/` (not committed) before
  each save, at most once a minute.
- **The saving helper** is the small plugin in `vite.config.js`. If the page shows a red
  "NOT SAVING" bar, the helper has stopped; start the designer again.
- **Rule checks** shown on each power live in `src/rules.js`. They mirror the drafting rules in
  `docs/MOD_DESIGN.md` ("Rules for drafting powers"); change the two together.
- **Scripts** in `tools/`: `add-drafts.mjs` adds drafted powers to an essence, and
  `fill-numbers.mjs` fills in first-pass numbers from one model.
- `SPEC.md` is the original brief for the tool.
