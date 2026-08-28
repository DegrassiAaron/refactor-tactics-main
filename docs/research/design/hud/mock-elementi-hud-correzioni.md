# Mock elementi HUD — correzioni e set di asset

> **Statuto**: sorgente di design, non canone. Le regole di gioco restano negli owner documentali e nel
> codice; qui c'è cosa il mock sbaglia, cosa si tiene, e quali asset ne sono usciti.
>
> **Sorgente**: [`source/mock-elementi-hud-2026-08-26.png`](source/mock-elementi-hud-2026-08-26.png) —
> 23 riquadri già ritagliati ed etichettati (`01_character_status_panel` … `23_wait_button`).
>
> **Stato misurato**: 2026-08-26, branch `claude/mock-elementi-huf-9aug0o`, HEAD `f71f81c`.

## 1. Il mock non è sbagliato dove sembra

Lo **stile** regge quasi per intero: taglio d'angolo invece del raggio, squadrette agli angoli, testa
esagonale sui bottoni, fondo profondo e tratto luminoso. È la stessa lingua di
[`progettazione-hud.md` §32](../../../technical/systems/progettazione-hud.md) — cyan per il focus, violet
per la reazione, amber per l'impegno, red per il critico. Non c'è niente da rifare lì.

A essere sbagliata è la **semantica**: nomi di eroi e abilità che non esistono, un modello di turno che il
gioco non ha, e quattro numeri che nessun campo produce. Un mock che porta uno stile giusto e una semantica
sbagliata è il caso peggiore, perché sembra pronto per l'import.

## 2. Verdetto per elemento

`TIENI` = si porta in produzione così. `CORREGGI` = la forma resta, il contenuto cambia. `RIMUOVI` = non
deve arrivare a un widget.

| # | Elemento | Verdetto | Perché |
|---|---|---|---|
| 01 | `character_status_panel` | **CORREGGI** | Le due barre sono legittime — `FRTUnitCardView` espone `Health`/`MaxHealth`, `Shield` **e** `Energy`/`MaxEnergy`, quindi le barre sono **tre**, non due. `VOLT RUNNER` è fuori roster (**D-120**). Il badge `12` non ha sorgente: non esiste un livello di unità. |
| 02 | `selected_ability_detail` | **CORREGGI** | Vedi §3.3: quattro campi su otto non esistono. |
| 03 | `round_turn_timeline` | **RIMUOVI** | Vedi §3.2: è una timeline a iniziativa, e il gioco non ha iniziativa. |
| 04 | `objectives_battlefield_panel` | **CORREGGI** | Gli obiettivi con contatore `2/6` vanno bene. `+20% Ranged Advantage` da terreno elevato no: in v0.1 l'High Ground **non dà bonus numerici**. |
| 05 | `movement_group` | **TIENI** | `Move · Sprint · Dash` è il raggruppamento giusto, e le tre card sono già distinte per forma. |
| 06 | `character_kit_group` | **CORREGGI** | Cinque slot numerati sono la forma giusta; i nomi dentro no (§3.1). Manca la nozione di **slot del turno** (§3.4). |
| 07 | `reaction_group` | **CORREGGI** | `Overwatch` è reazione. `Brace` **no**: è una delle sette generiche di **D-025**, `ERTActionSlot::Main`. Sta nella corsia difesa, non in quella di reazione. |
| 08 | `certainty_legend` | **CORREGGI** | I tre livelli e i tre tratti combaciano con `05-certainty-states.md` §2. Mancano `Invalid` e `Disabled`, che quel documento tiene separati apposta, e manca l'asse `Knowledge` (`CellOnly`: si mira alla cella, mai all'unità). |
| 09 | `context_actions_ready` | **CORREGGI** | `Interact` e `Wait` esistono. Il `READY` va bene come forma. Manca `Guard`, che è generica quanto le altre. |
| 10 | `bottom_right_utility_buttons` | **TIENI** | Quattro utility neutre, nessuna pretesa semantica. |
| 11 | `move_card` | **TIENI** | — |
| 12 | `sprint_card` | **CORREGGI** | Disegnata come «Move più veloce». `Action.Sprint` è `ERTActionSlot::MovementAndMain`: consuma **entrambi** gli slot. Il glifo deve chiudere, non allungarsi. |
| 13 | `dash_card` | **TIENI** | — |
| 14 | `basic_attack_card` | **TIENI** | — |
| 15 | `piercing_shot_card` | **RIMUOVI** | `PiercingShot` è di **Wraith**, non di questo kit: il mock mescola due eroi (§3.1). |
| 16 | `chain_discharge_card` | **CORREGGI** | Rimappata su `Hero.Gadget.LinearDischarge`. I salti erano la primitiva `Chain`; questa è `Line`. |
| 17 | `static_field_card` | **CORREGGI** | Rimappata su `Hero.Gadget.ConductiveNode`. |
| 18 | `overload_card` | **TIENI** (glifo ridisegnato) | `Hero.Gadget.Overload` esiste con questo nome. Il disegno passa alla grammatica `Damage`: massimo quattro diramazioni. |
| 19 | `brace_card` | **CORREGGI** | Tinta e corsia: difesa (`#56B4E9`), non reazione. |
| 20 | `overwatch_card` | **TIENI** | — |
| 21 | `ready_button` | **TIENI** (forma) | La parola `READY` non si rasterizza: è testo dinamico e localizzato. |
| 22 | `interact_button` | **CORREGGI** | La mano non è nel vocabolario di `03-forme-e-primitive.md` §2. `Interact` si disegna come `Object` + ingaggio. |
| 23 | `wait_button` | **TIENI** | — |

Su tutte e 23: i **keybind rasterizzati** (`Q`, `W`, `E`, `1`–`5`, `R`, `T`, `F`, `G`, `Space`) escono dagli
asset. Un binding è rimappabile, e cuocerlo dentro una texture significa una texture per ogni tasto —
`07-export-e-naming.md` §2.

## 3. Le quattro correzioni strutturali

### 3.1 Il kit è di un eroe che non esiste, e i nomi vengono da due eroi diversi

`VOLT RUNNER` non è nel roster: gli eroi sono `Gadget`, `Phase`, `Riktor`, `Wraith` (**D-120**), e i nomi
legacy sono **usciti dal repository** con **D-130**, senza redirect (**D-134** ha cancellato
`ResolveLegacyActionId`). Non c'è una doppia verità da risolvere in lettura: un nome fuori roster è un
difetto.

⛔ E non lo intercetta più nessuno: il gate che lo controllava, `scripts/check-docs-naming.py`, è uscito con
**D-182** il 2026-08-21. Questo mock è il primo caso in cui quell'uscita si vede.

Il kit elettrico è di **Gadget**. La rimappatura:

| Mock | Reale | Nota |
|---|---|---|
| Chain Discharge | `Hero.Gadget.LinearDischarge` | i salti non ci sono: `Line`, non `Chain` |
| Static Field | `Hero.Gadget.ConductiveNode` | — |
| Overload | `Hero.Gadget.Overload` | già corretto |
| Piercing Shot | — | è `Hero.Wraith.PiercingShot`: **esce dal kit** |
| — | `Hero.Gadget.ArcPulse` | mancante nel mock |
| — | `Hero.Gadget.ReactiveCapacitor` | mancante nel mock |

Gadget ha **cinque** ability, e il mock ne mostrava tre sue, una di Wraith e un attacco base.

### 3.2 La timeline a iniziativa descrive un altro gioco

`ROUND 02 · 27 INIT` con i ritratti in ordine di iniziativa presuppone che le unità agiscano a turno. Non è
così: le fasi sono `Planning → Prep → Dash → Blast → Move → Cleanup` (`RTTurnRules.h:10`) e la risoluzione è
**simultanea per fase**. Non c'è un ordine di unità da mostrare, perché non esiste.

Va sostituita da una **banda di fase**: quattro chip — `Prep`, `Dash`, `Blast`, `Move` — con quello corrente
in evidenza. Sono esattamente le quattro fasi volontarie che `URTIconLibrary::RequiredIconIds()` pretende;
`Planning` e `Cleanup` non sono fasi in cui il giocatore agisce, e la reazione non è una quinta fase.

⚠️ `Phase.Dash` e `Action.Dash` sono due chiavi diverse e non possono avere la stessa silhouette: una dice
«in che fase siamo», l'altra «cosa sto per fare». Il differenziatore è il **chip esagonale**, non il glifo
interno. Un widget che mostra un glifo di fase senza chip sta dicendo un'altra cosa.

### 3.3 Il tooltip abilità inventa quattro campi

Confronto fra il riquadro 02 e ciò che `URTActionData` espone davvero:

| Nel mock | Nel codice | Esito |
|---|---|---|
| `3 AP` | `EnergyCost` (`RTActionData.h:137`) | **rinomina**: la risorsa è **Energia** (`MaxEnergy` 100, `EnergyPerTurn` 25, `EnergyOnHit` 15). «AP» non esiste. Per il movimento la valuta è un'altra ancora: `CostMP` contro `MoveBudget` |
| `Rank 2` | — | **rimuovi**: non esiste alcun rank. Ciò che esiste sono le `Variants`, che si **escludono** nel loadout e dichiarano un compromesso — non un livello che sale |
| `Initial Damage 130` / `Chain Damage 75` | `Power` | **collassa in uno**: `Power` è un valore solo |
| `Max Jumps 3` | — | **rimuovi**: `PropagationLimit` esiste ma governa la propagazione di **superficie** (acqua, elettricità), non i salti di un'abilità |
| `Range 12m` | `RangeCells` | **converti**: la portata è in **celle** (distanza di Manhattan). I metri sono un'unità che il gioco non ha |
| `Applies Conduit for 1 turn` | tag `Status.*` | **rimappa**: `Conduit` non esiste. Gli undici stati sono `Braced Burning Electrified Exposed Guarded Marked Obscured Reveal Root Slow Wet`; il vicino è `Status.Electrified` |
| `+25% damage vs Shielded targets` | — | **rimuovi**: non c'è un tag `Shielded`. `Shield` è una risorsa su `FRTUnitCardView`, non uno stato bersagliabile |
| `Target preview 100% / 80% / 55%` | — | **rimuovi**: non esiste un modello di probabilità di colpire. Mostrare tre percentuali comunica una precisione statistica che il gioco non produce |

Restano invece disponibili e non usati: `Shape`, `AreaRadius`, `bSelfTarget`, `CooldownTurns`, `bIgnites`,
`Fallback`, `ResolutionPhase`. Un tooltip corretto è **più informativo** di quello del mock, non meno.

### 3.4 Manca la struttura che il piano ha davvero

Il mock mostra una hotbar di cinque slot numerati. Il piano di un turno ha invece **tre slot**:
`Movement`, `Main`, `Reaction` (`ERTActionSlot`, `RTActionDef.h:71`), e `FRTPlannedSlotView` li espone già
al pannello — con una distinzione che nessun riquadro del mock rappresenta:

> uno slot occupato **non ha sempre un `ActionId`**: un movimento normale è una lista di waypoint, non
> un'azione scelta. Un widget che leggesse solo `ActionId` mostrerebbe lo slot movimento vuoto proprio per
> chi ha appena tracciato un percorso — il caso più comune del gioco.

E `Action.Sprint` occupa `MovementAndMain`: selezionarlo deve **spegnere** entrambe le corsie, non
accendere una card.

## 4. Gli asset

Prodotti da `tools/hud-assets/generate_hud_assets.py`, deterministico:

```bash
python3 tools/hud-assets/generate_hud_assets.py          # -> Content/RT/UI/_Generated/
```

Output ignorato da git: è **derivato**, si rigenera, e un PNG in repository invecchia in silenzio quando la
grammatica cambia. Chi importa in Unreal esegue lo script e trova:

```text
Content/RT/UI/_Generated/
├── Icons/    67 master SVG + PNG RGBA (16/20/24/32/48, sopra la soglia leggibile di ciascuno)
├── Frames/   9 cornici SVG + PNG @1x/@2x
├── Review/   contact sheet a colori e in scala di grigi
└── manifest.json   scheda di consegna per asset (07-export-e-naming.md §3)
```

**67 icone**, che coprono **tutte e 61 le chiavi** che `URTIconLibrary::RequiredIconIds()` pretende: le 37
azioni del catalogo generico, gli 11 tag `Status.`, le 4 fasi volontarie, i 3 livelli di certezza, i 4
eroi più `Ally`/`Enemy`. Più le 5 ability di Gadget (chiave regolare sotto `Action.`, ma non nel catalogo
generico: servono alla skill bar, non alla copertura) e `MissingIcon`, che non è una chiave ma il campo
senza cui il catalogo non valida.

Il set del mock ne toccava **16**. Le altre 45 non sono «extra»: sono il debito che il mock nascondeva
mostrando una hotbar di cinque slot dove il gioco ha 37 azioni generiche, 11 stati e un roster.

### 4.0 Il generatore verifica la copertura, non la dichiara

Lo script legge le stesse sorgenti che legge `RequiredIconIds()` — il catalogo generico, i tag `Status.`,
le fasi volontarie, il roster — e confronta con ciò che disegna. Se una chiave resta scoperta **esce con
codice 1** e la nomina. Resta un surrogato (l'autorità è la funzione C++, e qui non gira Unreal), e per
questo il `manifest.json` dichiara sempre da quali file ha letto: un disallineamento si vede invece di
restare implicito.

**9 cornici**: pannello, slot (base e selezionato), bottone primario e secondario, portrait esagonale, nodo
di timeline, cornice di barra, chip di keybind. Tutte **vuote**: nessun fill composito, nessun gradiente
cotto, nessun testo. Il riempimento e la tinta li mette UMG, ed è ciò che permette a un asset solo di
coprire i nove stati di `07-export-e-naming.md` §4 invece di produrne nove.

Il master è **monocromatico**: la tinta semantica (Okabe-Ito, `02-color-system.md` §2) la applica il widget.
`manifest.json` porta `SuggestedTint` per asset, ma è un suggerimento per il widget — non una decisione
cotta nel PNG.

### 4.1 Cosa è cambiato disegnando, e perché

Tre glifi sono stati rifatti dopo il test di collisione di `03-forme-e-primitive.md` §7, che si fa
**guardando la contact sheet in scala di grigi**, non ragionando:

- **`Guard`** aveva due archi concentrici e leggeva come onde di segnale — cioè come `ArcPulse`. Ora è un
  arco frontale con i due terminali che dicono dove finisce, che è poi ciò che ADR-0005 §4a descrive: la
  difesa decade fuori dall'arco.
- **`Overload`** era due arcate contrapposte, e insieme formavano una lente. Una lente è un occhio, e un
  occhio è `Overwatch`. Ora è un anello tratteggiato che cede in quattro punti.
- **`ReactiveCapacitor`** leggeva come un tasto di accensione, e a 16 px spariva.

Le fasi hanno soglia **24 px** e la certezza **20 px**: sotto, il chip esagonale mangia il glifo. Lo script
non esporta i PNG sotto soglia — un asset illeggibile in cartella è un invito a metterlo dove non regge.

## 5. Cosa resta fuori

- **Il `DA_IconCatalog`.** Nessuno esiste ancora nel repository, e finché non esiste
  `FindMissingRequiredIcons(nullptr)` restituisce l'intera lista. Il passo è ora **scriptato**:
  `URTBuildIconCatalogCommandlet` (modulo Editor) importa i PNG e costruisce il data asset derivando ogni
  chiave da `RequiredIconIds()`. La procedura è in
  [`guida-catalogo-icone.md`](../../../technical/runbooks/guida-catalogo-icone.md).
  ⚠️ **Non è stato eseguito**: richiede Unreal, e il commandlet **non è mai stato compilato**.
- **I ritratti.** Le sei chiavi `Identity.*` sono coperte da marche astratte — badge esagonale, sigillo
  per eroe, rounded contro angular per `Ally`/`Enemy`. I **ritratti** veri restano lavoro d'autore.
- **L'import in Unreal.** Texture group, compressione, `Draw As: Box` e i margini 9-slice del manifest sono
  passi Editor: qui restano dichiarati, non eseguiti.
- **`08-catalogo-v0.1.md` è invecchiato** su due punti misurati oggi: dice che `Action.Overwatch` «non è
  ancora nel catalogo» (c'è, `RTCatalogLibrary.cpp:1127`, CP 14.5) e che `Certainty` «non è una categoria di
  catalogo» (lo è: `RequiredIconIds()` pretende le sue tre chiavi). E il conteggio: quel documento dice
  **55**, la misura sul branch corrente dà **61** — 37 azioni (non 36), 11 stati, 4 fasi, 3 certezze, 6
  identità. Non l'ho corretto lì: è un documento di cui non sono owner in questo lavoro.

## 6. Conflitti aperti dall'handoff Action Phases del 2026-08-26

[`RefactorTactics_ActionPhases_Dodge_Guard_Brace_Overwatch_Epics_v1.0_2026-08-26.md`](../../../archive/src/handoff/2026-08-26-action-phases-dodge-guard-brace-overwatch.md) — *da `docs/research/handoff/`, archiviato col proprio verdetto e rimosso dall'inbox il 2026-08-27; il link è stato ripuntato il 2026-08-28* —
è entrato nel repository come **input**, non come autorità: lo dice `CLAUDE.md` («un handoff non è
autorità») e lo dice il documento stesso, che impone di auditare `main` prima e di aprire una Decision
Issue sui conflitti invece di sovrascrivere.

Tre punti toccano il set di icone. Nessuno dei tre è stato applicato.

| Punto | Handoff | Repository misurato | Conseguenza sul set |
|---|---|---|---|
| §4.1 `Dash` → `Dodge` (`LOCKED_CHAT`) | `Dash` è **solo** macro-fase; il movimento generico si chiama `Dodge` | `Action.Dash` è nel catalogo generico (`RTCatalogLibrary.cpp`), e `ERTMatchPhase::Dash` è la fase: **lo stesso nome per due cose** | Il glifo di `Action.Dash` esiste e resta richiesto **oggi**. Il giorno del rename, `RequiredIconIds()` chiederà `Dodge` da sola e il generatore segnalerà la chiave scoperta |
| §4.1 vs catalogo | il nome proposto per lo scarto rapido è `Dodge` | esiste già **`Action.Evade`**, spedita, con lo stesso significato di scarto | Due nomi per la stessa cosa. Va deciso da una persona: `Dodge` sostituisce `Evade`, o convivono con significati diversi? Il set oggi disegna `Evade` |
| §4.7 Reaction come risorsa | `Guard` e `Interact` **spengono** la reazione | `ERTActionSlot::Reaction` è indipendente da Movimento e Principale | Nessun impatto sul disegno; impatto forte sulla **skill bar**, che deve poter mostrare una corsia spenta da un'altra scelta — è lo stesso requisito che `Sprint` (`MovementAndMain`) ha già oggi e che il mock non rappresentava (§3.4) |

⚠️ Il conflitto `Dodge`/`Evade` è quello da portare a una persona per primo: è l'unico dove l'handoff
propone un nome che il repository **ha già assegnato a qualcos'altro**, e nessuna delle due fonti ha
gerarchia sull'altra.
