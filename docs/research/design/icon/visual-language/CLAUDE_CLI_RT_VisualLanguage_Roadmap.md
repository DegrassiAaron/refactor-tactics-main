# CLAUDE CLI — RefactorTactics Tactical Visual Language Roadmap Handoff

## Missione
Integra nella repository reale la roadmap del **Tactical Visual Language**, verificando prima lo stato live di GitHub, Feature Registry, roadmap, source, test, wiki e asset. **Claude CLI crea/aggiorna Epic e Issue; Claude Design produce gli artefatti visuali.**

Non creare una roadmap parallela.

## Fonti note da verificare
La documentazione storica indica:
- `#217 — [EPIC] E20 · HUD Icon Language`
- `#218 — CP 20.1 · URTIconCatalogData`
- `#219 — CP 20.2 · Categorie della v0.1`
- `#220 — CP 20.3 · I widget consumano il catalogo`
- post-v0.1: candidato `E25 — Icon Language completo`

Questi riferimenti sono input di audit, non autorizzazione a duplicare issue.


## Regole globali LOCKED

- RefactorTactics è PC-first, tattico simultaneo, server-authoritative e deterministico.
- La UI è presentation-only: non decide path, hit, reaction, status o outcome.
- Gli intenti avversari privati non devono mai arrivare ai presentation model/client avversari.
- `Confirmed / Predicted / Uncertain` sono semantiche distinte e non dipendono solo dal colore.
- `Validity / Certainty / Knowledge` sono assi separati.
- Icone e visual token sono semantici e componibili, non un'immagine custom per ogni skill.
- Ally/Enemy devono differire per forma oltre che per colore.
- Evitare rosso/verde come coppia primaria Ally/Enemy.
- Default palette iniziale: Ally cyan/blue, Enemy orange; elementi mantengono il proprio colore semantico.
- Il sistema deve restare leggibile in grayscale; CVD e High Contrast rafforzano forma, pattern, luminanza e outline.
- `Electric` e `Reaction` non condividono il fulmine.
- `Line` e `Move` sono distinti: Line = origine + segmento + punta; Move = path con nodi.
- `Cover`, `Guard`, `Brace`, `Shield` sono concetti distinti.
- `Water` payload, `Wet` unit state e `Water Surface` cell state sono distinti tramite composizione/frame.
- Non creare Gameplay Tag solo per soddisfare un'icona.
- Non hardcodare texture/colori nei widget: usare semantic ID/catalog/theme.
- La versione Unreal reale va letta dalla repo; la documentazione corrente usa UE 5.8 come baseline.


## Prima di modificare
1. `git status`, branch, HEAD, fetch.
2. Leggi `AGENTS.md`, `CLAUDE.md`, README, Decision Log/ADR correnti.
3. Leggi `docs/roadmap/feature-registry.yaml` come fonte operativa; non modificare a mano le viste generate.
4. Leggi roadmap v0.1 e post-v0.1.
5. Cerca E20, E25 e tutte le issue icon/UI/accessibility già esistenti.
6. Cerca cataloghi, ViewModel, style/theme, widget, icon gallery, validation e test già implementati.
7. Confronta con `RT_VisualLanguage_Epics_Issues.yaml`.
8. Se c'è conflitto tra questo handoff e una decisione più recente della repo, prevale la repo e il conflitto va riportato.

## File del pack da usare
- `RT_VisualLanguage_Epics_Issues.yaml` — mappa macchina issue/epic/checkpoint.
- `RT_VisualLanguage_Epics_Issues.md` — vista leggibile.
- `CLAUDE_DESIGN_RT_VisualLanguage_MASTER.md` — regole globali del design.
- `ClaudeDesign/ICON-*.md` — brief per ogni checkpoint.
- `References/` — proof-of-concept e brief grafico precedente, solo reference.

## Lavoro richiesto a Claude CLI
Per ogni ICON-0…ICON-11:
1. Trova feature/epic/issue owner già esistenti.
2. Aggiorna quelle esistenti.
3. Crea solo le issue mancanti realmente necessarie.
4. Collega ogni issue al relativo prompt `ClaudeDesign/ICON-*.md`.
5. Aggiorna Feature Registry, roadmap, Wiki/spec owner, Scenario Map e Editor Map dove applicabile.
6. Se un deliverable richiede Unreal Editor o asset binari, registralo come manual/design task e non fingere di averlo completato.
7. Per issue di privacy, testare che il dato non esista nel DTO/ViewModel avversario, non solo che il widget sia hidden.
8. Per issue visuali, richiedere evidenza almeno a 1080p e grayscale/CVD quando applicabile.

## Distribuzione scope
### v0.1 / E20
E20 resta la fondazione minima. Preservare #217–#220 e non spostare arbitrariamente scope.
Priorità:
- catalog/resolver/fallback/validator;
- categorie v0.1;
- consumer widget minimo;
- proof-of-concept glyph;
- semantic theme foundation solo se necessario al consumer reale;
- planning/HUD consumer minimo richiesto dal vertical slice.

### post-v0.1 / E25
Verificare se esiste. Se manca, candidato:
`[EPIC] E25 · Icon Language completo`

Checkpoint candidati:
- `CP 25.1 · Tassonomia completa e governance`
- `CP 25.2 · Catalogo completo, validator e authoring`
- `CP 25.3 · HUD / world-space / reaction / perception integration`
- `CP 25.4 · Accessibility, Wiki e documentazione`

ICON-3…ICON-11 possono avere un minimo v0.1 quando già necessario, ma la loro integrazione/polish completa appartiene a E25 salvo roadmap live diversa.

## Definition of Done
Una feature visual-language è Done solo se:
- usa semantic ID/data e non asset path hardcoded;
- consumer reale presente;
- fallback/validation presenti;
- nessun leak di informazione;
- ordine deterministico quando esiste priority/stack;
- accessibilità non-color-only;
- debug/inspect disponibile;
- test pertinente;
- packaged verification quando richiesta;
- docs/roadmap/feature registry allineati;
- evidence visuale/manuale allegata quando la feature è artistica/editor-only.

## Output finale
Restituisci:
1. branch + HEAD iniziali;
2. issue/epic già esistenti trovati;
3. issue/epic creati con numeri reali;
4. issue aggiornati;
5. mapping `ICON-* -> issue -> feature -> spec -> design prompt`;
6. file roadmap/registry/wiki/scenario/editor map modificati;
7. test/build eseguiti e risultato reale;
8. prompt Claude Design pronti o aggiornati;
9. conflitti/debito/open decisions;
10. commit creati, se autorizzato.

Non usare “implementato” per confondere:
`SPECIFICATO` / `ASSET PRODOTTO` / `DATO PRESENTE` / `CONSUMATO A RUNTIME` / `TESTATO`.
