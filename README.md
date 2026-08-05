# RefactorTactics

Gioco tattico PvP a **turni simultanei** ispirato ad *Atlas Reactor*, su **Unreal Engine 5.8.1**, sviluppato
da un dev singolo.

Ogni turno: **pianificazione simultanea** delle mosse → risoluzione a macro-fasi
**Prep → Dash → Blast → Move**, calcolate simultaneamente e applicate in ordine deterministico.
Griglia **esagonale** multilivello (`FRTCellId`, coordinate assiali/cubiche) con editor mappa data-driven.

> **Stato attuale (2026-08-05)**
>
> - **Fondamenta esagonali complete e testate**: coordinate, asset mappa, A\* multilivello, snapshot,
>   budget di movimento, collisioni simultanee, TurnLog con hash e replay, LOS e forme di targeting, bot
>   utility, editor mode della mappa.
> - **Nessuna partita gira ancora sull'esagonale**: il turn loop giocabile è quello **quadrato** dell'MVP
>   (2v2 offline contro bot, movimento con conflitti, combat, HUD, vittoria). Colmare questo divario è la
>   milestone **M6** ≡ epic **E2** della v0.1.
> - **172 test automatici** (`Source/RefactorTactics/Tests/`, 25 file), ultima esecuzione riportata 172/172 verdi.
> - Packaging Windows verificato sull'MVP quadrato (Development e Shipping).
>
> Dettaglio: [roadmap di release v0.1](docs/design/roadmap-v0.1.md) ·
> [stato per checkpoint](docs/design/roadmap-checkpoint.md).

---

## Obiettivo: la release v0.1

Un *vertical slice* giocabile **2v2 offline contro bot** su griglia esagonale multilivello, con:

- **4 eroi** distinti (Flux, Riva, Bastion, Vektor), 4 abilità ciascuno + una variante;
- **catalogo azioni** completo (~35 azioni con ID stabile, fase, priorità intera, fallback, cooldown);
- **reazioni** preparate in pianificazione (una attivazione per turno);
- **terreni attivi** (acqua, fuoco, elettricità, fumo, ghiaccio) con stati e propagazione deterministica;
- **coperture direzionali e strutture** (porte, ponti, pannelli) che cambiano la topologia;
- **obiettivi dinamici** e partita a 12 turni massimi;
- **HUD** con intenti alleati e livello di certezza, combat log e comandi `rt.Debug.*`;
- **determinismo verificato** (100 ripetizioni a seed fisso, checksum identico) e build packaged giocabile.

Multiplayer in rete, 4v4, GAS, progressione e modding restano
[visione post-v0.1](docs/design/piano-canonico-mvp.md#8-north-star-post-mvp-dai-prd).

## Stack tecnico

| | |
|---|---|
| Motore | Unreal Engine **5.8.1** (bloccata) |
| Linguaggi | **C++** (regole, dati, resolver, test) + **Blueprint** (UI, VFX, camera, presentazione) |
| Ability system | Data-driven (`UPrimaryDataAsset`) — **GAS rimandato** |
| Griglia | Esagonale assiale/cubica multilivello (`FRTCellId`) |
| Versionamento | Git + **Git LFS** |
| IDE | Visual Studio 2022 (workload *Game development with C++*) |
| Piattaforma | Windows |

## Struttura del repository

```
RefactorTactics.uproject       # descrittore progetto Unreal (radice del repo)
Source/
  RefactorTactics/             # modulo runtime C++
    Ability/ Bot/ Camera/ Combat/ Core/ Grid/ Map/
    Pathfinding/ Player/ Selection/ Terrain/ Turn/ UI/ Unit/ Tests/
  RefactorTacticsEditor/       # modulo editor-only (Hex Map Editor Mode)
Config/                        # DefaultEngine.ini, DefaultGame.ini, input
Content/
  RT/                          # asset proprietari, organizzati feature-first
  Paragon*/                    # asset di terze parti (non versionati)
docs/
  design/                      # canone, roadmap, ADR, spec, verifiche PIE
  PDR/                         # requisiti di lungo periodo (fasi F0-F6)
  guides/                      # guide operative
  src/                         # PDF sorgente (visione e cataloghi)
  archive/                     # materiale superato
CLAUDE.md · AGENTS.md          # guide operative per assistenti di codice
```

## Documentazione — da dove partire

| Documento | Cosa contiene |
|---|---|
| [`docs/design/piano-canonico-mvp.md`](docs/design/piano-canonico-mvp.md) | ⭐ **Canone**: decisioni vincolanti, invarianti, regole numeriche |
| [`docs/design/roadmap-v0.1.md`](docs/design/roadmap-v0.1.md) | Release **v0.1**: 12 epic, 59 checkpoint, stato feature → file |
| [`docs/design/v0.1-definition-of-done.md`](docs/design/v0.1-definition-of-done.md) | Gate di release, KPI, checklist di contenuto |
| [`docs/design/roadmap-checkpoint.md`](docs/design/roadmap-checkpoint.md) | Milestone M6–M11 con DoD misurabile e stato |
| [`docs/design/architettura-codice.md`](docs/design/architettura-codice.md) | Mappa delle classi C++ |
| [`docs/design/convenzioni-contenuti-ue.md`](docs/design/convenzioni-contenuti-ue.md) | Struttura di `Content/`, naming, dipendenze |
| [`docs/design/test-manuali-pie.md`](docs/design/test-manuali-pie.md) | Verifiche interattive in editor, per sessioni |
| ADR [0002](docs/design/adr-0002-griglia-esagonale.md) · [0003](docs/design/adr-0003-modello-azioni-v01.md) | Pivot esagonale · modello azioni della v0.1 |

> ⚠️ I PDF in `docs/src/` e `docs/PDR/` descrivono un prodotto più ambizioso dello scope corrente e in parte
> si contraddicono. Le decisioni effettive sono riconciliate nel **piano canonico**, che ha la precedenza.

## Come compilare ed eseguire

1. Installare **Unreal Engine 5.8.1** e **Visual Studio 2022** (workload *Game development with C++*).
   Installare **Git LFS** (`git lfs install`) e clonare il repository.
2. Aprire **`RefactorTactics.uproject`** — o generare i file di soluzione (tasto destro sul `.uproject` →
   *Generate Visual Studio project files*) — e compilare il target **Development Editor**.
3. Aprire un livello e premere **Play**:
   - `Content/RT/Maps/Dev/L_Prototype` — demo della partita 2v2;
   - `Content/RT/Maps/Dev/L_DevSandbox` — sandbox con mappa esagonale.

   I livelli sono **vuoti nell'editor**: griglia, luce, unità e turn manager li allestisce a runtime il
   `RTGameMode`. Viewport nera prima del Play è normale.
4. Comandi: **WASD** pan camera · **rotellina** zoom · **Home** ricentra · **F** centra sull'unità selezionata ·
   **click** su unità propria = selezione · **click** su cella = movimento · **click** su nemico = attacco ·
   **Spazio** = risolvi il turno (o attendi il timer di 30 s) · **R** = riavvia la partita.
5. Test: **Tools → Session Frontend → Automation** → `RefactorTactics` → *Start Tests* (172 test).
   Guida al debug in [`docs/guides/debug-vs-unreal.md`](docs/guides/debug-vs-unreal.md).

## Principi di sviluppo

- **Le regole decidono l'esito** (C++ autoritativo); animazioni e VFX non decidono nulla.
- **Determinismo**: stessa snapshot e stesso seed ⇒ stesso risultato. Coordinate e costi **interi**, nessuna
  dipendenza dall'ordine di container non ordinati, ogni formato serializzato versionato.
- **Privacy dell'intento**: le intenzioni di pianificazione non raggiungono gli avversari — mai occultamento
  solo grafico.
- **Server-authority-ready** già in offline: l'autorità è isolata nel turn manager.

## Note

- L'ispirazione ad *Atlas Reactor* è solo a livello di **meccaniche**. Per una pubblicazione servono nomi,
  personaggi e asset **originali**.
- Documentazione e commenti sono in **italiano**; il codice in inglese.
- Il progetto è nato (fino al 2026-08-05) anche come percorso di apprendimento di UE partendo da C#: quella
  fase è **chiusa** e il relativo materiale è storico.

## Licenza

Non ancora definita.
