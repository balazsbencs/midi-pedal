---
version: 1
slug: "editor-src-app-tsx"
primary_target: "editor/src/App.tsx"
related_targets: ["editor/src/ui/workspace.css","editor/src/ui/theme/tokens.css","editor/src/ui/layout/AppHeader.tsx","editor/src/ui/banks/BankList.tsx","editor/src/ui/presets/PageMap.tsx","editor/src/ui/inspector/PresetInspector.tsx","editor/src/ui/layout/StatusBar.tsx"]
---

# Browser editor surface brief

- Mode: operate. A musician or builder must move from bank selection to preset editing and device synchronization without losing page or switch context.
- Direction: Studio Console, built literally from the approved concept. The full-width command bar, bank rail, oversized colored A–D deck, tall inspector, compact status floor, dark material, and dense sans-serif hierarchy are the surface—not loose inspiration.
- Memorable moment: four luminous, near-square cyan/coral/violet/yellow switch fields make the physical A–D pedal map unmistakable before any control is read; the selected color continues into the inspector.
- Primary task: choose a bank and page, select a switch, edit its presentation and ordered messages, validate, then sync or export.
- Constraints: preserve all current behavior and copy, 128 searchable banks, fixed A–D order, explicit light/dark themes, keyboard access, laptop and 200% zoom support, no backend, and no decorative charts or invented device capabilities.
- Responsive rule: keep rail, map, and inspector together on wide screens; place the inspector below the map on compact laptops; stack all regions on phones while keeping every device and file action reachable.
