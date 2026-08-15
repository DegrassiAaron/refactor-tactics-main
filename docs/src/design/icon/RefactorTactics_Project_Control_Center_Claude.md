# RefactorTactics — Claude Design HUD & Icon Pack v0.1

## Scopo

Questo pacchetto è l'handoff operativo per **Claude Design Web / UI Designer AI** per produrre il set visuale statico della HUD e il linguaggio iconografico della **v0.1 di RefactorTactics**.

Obiettivo: avere un sistema coerente, leggibile, accessibile e riutilizzabile in:

- Character Skill Bar;
- Planning HUD;
- Ghost Timeline;
- Reaction / Fast Reaction;
- roster e portrait;
- tooltip / skill card;
- warning;
- objective;
- combat log;
- tactical marker;
- Wiki e tutorial;
- debug/icon gallery.

Non creare una collezione di icone decorative indipendenti. Le icone devono comunicare semantica e devono poter essere risolte tramite ID/catalogo.

---

# 1. Fonti visuali da usare

## Primary visual reference

Usare come fonte visiva primaria il PNG presente nel repository:

`Guida visiva HUD tattica fantascientifica.png`

Questa tavola definisce:

- pannelli;
- pulsanti;
- action slot;
- portrait frame;
- resource bar;
- timeline;
- warning;
- tactical marker;
- palette;
- tipografia;
- densità e linguaggio dei bordi.

## Secondary visual reference

Se presente, usare anche:

`Interfaccia tattica sci-fi futuristica.png`

per phase banners, decision boundary, Fast Reaction, FIRE/HOLD e mission/objective states.

## Concept render incluso

`RefactorTactics_HUD_SkillBar_Concept_Reference.png`

È una **reference di composizione e atmosfera**, non una pixel-spec e non deve sostituire la style guide ufficiale.

La direzione estetica è:

- premium tactical sci-fi/fantasy;
- ispirazione generale Paragon-era, ma identità originale RefactorTactics;
- pannelli scuri traslucidi;
- cornici metalliche sottili;
- cut angolari/esagonali controllati;
- glow solo dove funzionale;
- centro schermata libero;
- niente cockpit, MMO chrome o neon ovunque.

---

# 2. Baseline HUD / Skill Bar da preservare

La barra del personaggio è organizzata in tre gruppi:

```text
MOVEMENT
Move | Sprint | Dash

CHARACTER KIT
Basic Attack | Skill 1 | Skill 2 | Skill 3 | Skill 4

REACTION
Brace | Overwatch
```

Azioni contestuali separate:

```text
Interact | Wait | Ready
```

Regole:

- `Dash` è una action/movement identity distinta da Sprint e serve anche come grammatica per dodge/reposition rapido.
- `Basic Attack` viene mostrato visivamente nel Character Kit anche se è una categoria universale.
- `Brace` e `Overwatch` sono separati.
- `Ready` non è una skill.
- non comunicare due action economy indipendenti fra Universal Actions e Hero Kit.

---

# 3. Roster v0.1 corrente

Usare i nomi RefactorTactics:

- Flux
- Riva
- Bastion
- Vektor

Le immagini/asset Paragon possono essere usate come base visuale dove previsto dal progetto, ma **non rinominare** i personaggi RT con i nomi asset.

---

# 4. Ordine di prevalenza per Claude Design

Se esiste un conflitto:

```text
1. Decisioni/ADR correnti del repository
2. Cataloghi e Wiki v0.1 correnti
3. RefactorTactics_VisualLanguage_IconDesign_Claude.md
4. progettazione-hud.md
5. Guida visiva HUD tattica fantascientifica.png
6. Questo pacchetto
7. Concept render / riferimenti storici
```

Non correggere un conflitto di gameplay inventando una soluzione grafica.

---

# 5. File del pacchetto

- `CLAUDE_DESIGN_01_Icon_Language_and_Visual_Rules_v0.1.md` — regole grafiche, griglia, palette, composizione e accessibilità.
- `CLAUDE_DESIGN_02_Icon_Manifest_v0.1.md` — inventario esteso delle icone statiche v0.1.
- `CLAUDE_DESIGN_03_Static_HUD_Assets_v0.1.md` — pannelli, slot, frame, marker, overlay e distinzione statico/dinamico.
- `CLAUDE_DESIGN_04_Production_Batches_Prompts_QA_v0.1.md` — roadmap di produzione, prompt pronti per Claude Design e QA.
- `RefactorTactics_UI_Icon_Manifest_v0.1.csv` — manifest tabellare importabile/filtrabile.
- `RefactorTactics_HUD_SkillBar_Concept_Reference.png` — reference visiva corrente.

---

# 6. Risultato finale richiesto

Claude Design deve consegnare:

1. master glyph vettoriali/tintabili;
2. PNG trasparenti alle dimensioni richieste;
3. artboard di review;
4. action slot e HUD chrome statico;
5. stati Hover/Selected/Planned/Disabled/Cooldown/Invalid/Warning;
6. versioni Default Accessible / CVD / High Contrast;
7. test grayscale e background torture;
8. naming coerente con Unreal;
9. inventario finale `DONE / DEFERRED / NOT USED`;
10. nessuna informazione competitiva trasmessa solo tramite colore.
