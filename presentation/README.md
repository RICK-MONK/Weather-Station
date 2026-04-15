# Presentation Setup

This folder contains an isolated **Marp** presentation for the ESP32 weather station project. It does not modify the main backend, frontend, or hardware project files.

## Files

- `slides.md`: main slide deck
- `theme.css`: custom Marp theme
- `package.json`: local Marp CLI scripts

## VS Code Preview

1. Install the **Marp for VS Code** extension.
2. Open [slides.md](./slides.md).
3. Run `Marp: Open Preview to the Side` from the VS Code command palette.

This gives you immediate in-editor slide preview.

## Terminal Commands

Run all commands from the `presentation/` folder.

### Install dependencies

```powershell
cmd /c npm install
```

### Preview locally in the browser

```powershell
cmd /c npm run preview
```

Then open the local URL shown by Marp, usually `http://localhost:8080/slides.md`.

### Export HTML

```powershell
cmd /c npm run export:html
```

### Export PDF

```powershell
cmd /c npm run export:pdf
```

### Export PowerPoint

```powershell
cmd /c npm run export:pptx
```

## Presentation Mode

For presenting directly in VS Code, use the Marp preview window in full screen.

For browser-based presenting, start:

```powershell
cmd /c npm run preview
```

and open the served deck in your browser.
