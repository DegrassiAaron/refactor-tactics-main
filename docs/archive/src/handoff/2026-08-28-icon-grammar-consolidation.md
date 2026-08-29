# CLAUDE HANDOFF — RefactorTactics Icon Grammar Consolidation

> `HISTORICAL` · **Kit d'autore consumato**, non una fonte. · **Consumato**: 2026-08-28 · **Base**:
> `c1a7cd9d`, branch `feat/220-slot-consuma-catalogo` (`origin/main` `0f3f8882`).
>
> Archiviato da [`docs/archive/`](../../README.md): vale per la **provenienza** e il rationale, mai per la
> regola. Stava alla radice del repository come `CLAUDE_RT_IconGrammar_Consolidation_Handoff_2026-08-28.md`.
>
> **Che cosa ne è stato recepito**: [**D-231**](../../../decisions/RT_PDR_00_Decision_Log.md), con owner
> [`spec-icon-card-grammar.md`](../../../technical/systems/spec-icon-card-grammar.md). La revisione che ha
> deciso cosa consolidare e cosa no è in
> [`icon-card-grammar-spec-panel-2026-08-28.md`](../../../roadmap/plans/icon-card-grammar-spec-panel-2026-08-28.md).
>
> 🔴 **Quattro punti di questo documento NON sono stati recepiti, e chi lo rilegge non deve riapplicarli:**
>
> | §  | Diceva | Misura |
> |---|---|---|
> | §3.3 | esagono **flat-top** | **falso**: il gioco è `pointy-top` (`RTCellId.h:7,22`; test `Hex.CellCornersFormPointyTopHexagon`) e lo **SVG master allegato allo stesso handoff** è pointy-top (h/w = 1.1547) |
> | §4 | il colore va riservato alla **fase** | **non recepito**: rovescerebbe `09-alfabeto-fase-e-conseguenza.md`, la cui Fase 1 è implementata in `tools/hud-assets/generate_hud_assets.py` con i gate T1/T3/T5/T6/T7. Resta **open point** in D-231 |
> | §2.2 | `Dodge` non è una fase, `Dash` sì | **vero, ma già deciso**: è **D-230**, in volo sulla PR #1532. Non è stato registrato due volte |
> | §12.1 | palette delle **sei** macro-fasi | i marker iconografati sono **quattro**: `RequiredIconIds()` deriva le sole `VoluntaryPhases` (`RTIconLibrary.cpp:35-36`) |
>
> ⚠️ **E l'elenco fonti del §1 era incompleto**: nomina nove file di `visual-language/`, che ne contiene
> **quindici**. I due che decidono la materia — `09-alfabeto-fase-e-conseguenza.md` e
> `10-catalogo-sette-categorie.md`, entrambi del 2026-08-26 — sono fuori dall'elenco. Eseguire l'handoff
> leggendo solo ciò che elenca avrebbe rovesciato una spec implementata credendo di registrarne una nuova.

---


**Data:** 2026-08-28  
**Scopo:** recepire nel repository le decisioni più recenti sulla grammatica delle icone e della Skill Card, senza perdere il canone runtime esistente e senza confondere grammatica compositiva, catalogo semantico e HUD layout.

---

## 0. Obiettivo operativo

Devi consolidare nel repository **quattro cose distinte**:

1. **Decisioni** — aggiungere una decisione esplicita nel Decision Log per la grammatica della Skill Card radiale e per il significato delle forme satellite.
2. **Documentazione owner** — recepire la grammatica in un documento vivo/autorevole, mantenendo separato il catalogo runtime.
3. **Issue / checkpoint** — aggiornare le issue esistenti E20/E25 e, solo se serve davvero, creare una issue specifica per il recepimento della Card Grammar.
4. **Roadmap / epic** — aggiornare E20/E25 e le roadmap senza creare doppioni o nuove epic decorative.

**Non trasformare questo lavoro in un refactor del runtime icon catalog.** La grammatica nuova descrive come si compone una scheda/glifo; D-031 e `ERTIconCategory` continuano a governare la risoluzione runtime.

---

# 1. Fonti da leggere PRIMA di modificare

## Canone runtime / owner esistenti

- `docs/decisions/RT_PDR_00_Decision_Log.md`
- `Source/RefactorTactics/UI/RTIconCatalogData.h`
- `Source/RefactorTactics/UI/RTIconLibrary.cpp`
- `docs/technical/tooling/brief-icone-v01.md`
- `docs/technical/systems/progettazione-hud.md`

## Research / provenienza visuale

- `docs/research/design/icon/visual-language/00_README.md`
- `docs/research/design/icon/visual-language/01-principi.md`
- `docs/research/design/icon/visual-language/02-color-system.md`
- `docs/research/design/icon/visual-language/03-forme-e-primitive.md`
- `docs/research/design/icon/visual-language/04-regole-di-composizione.md`
- `docs/research/design/icon/visual-language/05-certainty-states.md`
- `docs/research/design/icon/visual-language/06-accessibilita.md`
- `docs/research/design/icon/visual-language/07-export-e-naming.md`
- `docs/research/design/icon/visual-language/08-catalogo-v0.1.md`

**Nota di statuto:** `docs/research/` è dichiarato non autorevole. Va usato come provenienza e materiale di design; dove diverge da Decision Log/codice, prevalgono Decision Log e codice.

## Issue / epic da leggere

- `#217` — `[EPIC] E20 · HUD Icon Language`
- `#219` — `CP 20.2 · Categorie della v0.1`
- `#220` — `CP 20.3 · I widget consumano il catalogo`
- `#637` — riconciliazione tassonomia manifest/runtime
- `#265` — `[EPIC] E25 · Icon Language completo`
- `#266` — `CP 25.1 · Tassonomia completa e governance`
- `#267` — `CP 25.2 · Catalogo completo, validator e authoring`
- `#268` — `CP 25.3 · Integrazione HUD, world-space, reaction, perception`
- `#269` — `CP 25.4 · Accessibility, Wiki e documentazione`

## Roadmap

- `docs/roadmap/roadmap-v0.1.md`
- `docs/roadmap/roadmap-post-v0.1.md`
- `docs/roadmap/roadmap-checkpoint.md`

---

# 2. Decisioni da recepire

Queste sono le decisioni più recenti da consolidare.

## 2.1 Grammatica e catalogo sono due assi distinti

**Grammatica compositiva** = come si disegna/compone una skill o un glifo.

**Catalogo semantico runtime** = come il widget risolve un asset tramite `UI.Icon.<Category>.<Name>`.

Non creare categorie runtime solo perché esistono primitive visuali come `Target`, `Shape`, `Delivery`, `Effect`, `HitRule`.

Esempio corretto:

```text
UI.Icon.Action.Hero.Gadget.LinearDischarge
```

L'icona può essere composta con primitive `Line + Electric`, ma non deve esistere per forza `UI.Icon.Geometry.Line`.

---

## 2.2 Macro-fasi reali

La grammatica deve usare le macro-fasi vere del round:

```text
Planning → Prep → Dash → Blast → Move → Cleanup
```

Regole:

- `Dodge` **non è una macro-fase**: è un'azione che vive in `Dash`.
- `Reaction` **non è una macro-fase**.
- `Overwatch` **non è un tipo di attacco**.
- `Overwatch` è una **Reaction preparata**: osserva un trigger e genera poi un attacco con proprie `Shape`, `Delivery`, `Effect`.

Non reintrodurre la vecchia lettura `Dodge / Blast / Move / Reaction` come quattro fasi equivalenti.

---

## 2.3 Formula di lettura della skill

Formula consolidata:

```text
PHASE → TARGET + SHAPE + DELIVERY + HIT RULE → EFFECT + VALUE → RANGE / DURATION + MODIFIERS
```

Non tutti i termini devono apparire contemporaneamente.

### Target

Card set consolidato:

```text
Self · Unit · Cell · Direction · Path
```

Per `Unit`:

```text
Ally · Enemy · Any
```

`Object` esiste nel research come primitiva, ma non considerarlo automaticamente parte del set card congelato: se serve, dichiaralo come estensione / open decision.

### Shape / AoE

```text
Single · Line · Ray · Cone · Circle · Ring · Cross · Arc · Rectangle · Wall · Chain · ConnectedRegion
```

Regole:

- `Line` ≠ `Move`.
- `Chain` deve mostrare nodi/salti; non può essere solo un fulmine.
- `ConnectedRegion` serve per propagazioni sistemiche su celle collegate.

### Delivery

```text
Direct · Projectile · Beam · Lob · Ground
```

### Hit Rule

Minimo consolidato:

```text
First · All · StopOnUnit · StopOnBlocker · PierceUnits · Chain
```

Modifier/varianti correlate emerse:

```text
Pierce · StopOnFirstTarget · HitsAll · Bounce · FriendlyFirePossible · IgnoreCover · DestroyCover · ArmorPiercing · ArmorShred
```

### Effect

Famiglie emerse:

```text
Damage · Shield · Armor · Heal · Push · Pull · Displace/Reposition
Water/Wet · Electric/Shock · Fire · Status
CoverModification · TerrainModification · GraphTraversalModification · Utility
```

Distinzioni obbligatorie:

- `Cover` ≠ `Guard` ≠ `Brace` ≠ `Shield`.
- `Electric` possiede il fulmine; `Reaction` non usa il fulmine come simbolo primario.
- `Dash` = movimento scelto; `ForcedMovement` = movimento subito.
- `ArmorPiercing` ≠ `ArmorShred`.

---

# 3. Skill Card radiale — decisione principale da registrare

## 3.1 Struttura

La Skill Card usa un **esagono flat-top centrale** come core.

Principio:

> **Centro = informazione fondamentale della skill / effetto principale.**  
> **Periferia = proprietà, condizioni, costi e modificatori.**

La forma del satellite comunica il tipo di informazione.

## 3.2 Significato delle forme

| Forma | Significato | Posizione preferita | Limite |
|---|---|---|---:|
| **Esagono grande** | core / effetto principale | centro | 1 |
| **Marker Phase** | macro-fase/timing reale | sopra | 1 |
| **Cerchio** | costo, cooldown, cariche, limite d'uso | alto-sinistra | max 1 |
| **Piccolo esagono** | proprietà intrinseca: Target, elemento, Shape/pattern, Delivery/uso | sinistra/destra/alto-destra | max 3 |
| **Quadrato** | modificatore esterno su unità/cella/contesto | basso-destra | max 2 |
| **Rombo** | modificatore applicato direttamente alla skill | basso-centro | max 1 |
| **Triangolo** | condizione, trigger, requisito, warning | alto-destra o basso-sinistra | max 1 |

Gerarchia di lettura:

```text
CORE > PHASE > proprietà native > condizione > modificatori
```

## 3.3 Reticolo geometrico

- esagono `flat-top`;
- centro come origine;
- linea orizzontale che passa per il centro;
- replica della stessa linea ogni **30°**;
- 6 assi completi / 12 raggi;
- satelliti agganciati a questa geometria, **mai piazzati a occhio**.

Il reticolo è una regola di authoring, non una texture obbligatoria da mostrare in HUD.

---

# 4. Canali visivi

Decisione della Card Grammar:

| Canale | Uso |
|---|---|
| colore | macro-fase / timing reale |
| geometria | Shape / AoE |
| glifo | effetto o proprietà |
| numero | quantità, sempre con contesto/simbolo |
| posizione | categoria dell'informazione |
| glifi piccoli | modifier / Hit Rule |
| bordi/connettori | relazioni, trigger, dipendenze |

## Conflitto da NON nascondere

`docs/research/design/icon/visual-language/02-color-system.md` usa ancora una palette semantica per `Movement`, `Attack`, `Defense`, `Reaction`, ecc.

La decisione più recente della Card Grammar riserva invece il colore alla **fase reale**.

Questa divergenza va registrata come tale.

**Non correggere automaticamente il research per “farlo tornare”.** Prima:

1. registrare la decisione;
2. identificare l'owner vivo;
3. aggiornare il research solo come provenienza/superato, se coerente con la governance documentale.

---

# 5. Vincoli visuali da mantenere

Questi sono già coerenti con il research esistente e vanno recepiti:

- silhouette/pattern prima del colore;
- test grayscale obbligatorio;
- `Ally` / `Enemy` distinguibili senza colore;
- `Line` / `Move` distinguibili senza testo;
- `Electric` / `Reaction` distinti;
- `Cover` / `Guard` / `Brace` / `Shield` distinti;
- `Dash` / `Sprint` distinti;
- niente glow indispensabile;
- niente 3D/prospettiva nei glifi base;
- niente testo nel glifo runtime;
- flat/vector, sci-fi tattico, minimal geometrico;
- max 2–3 colori nell'esplorazione;
- leggibilità reale: 20–24 px a 1080p;
- glifo ability ~32–36 px dentro slot 56–60 px;
- un numero non appare mai senza contesto/simbolo.

---

# 6. Esempi da inserire nella documentazione

Inserire almeno tre esempi completi per rendere la grammatica falsificabile.

## Rail Shot

```text
Blast · Enemy/Direction · Line · Beam · Damage 5 · Pierce · Range 7
```

Card:

- core: `Damage 5`
- phase: `Blast`
- hex: `Enemy/Direction`, `Line`, `Beam`
- modifier: `Pierce`
- range: `Range 7` con simbolo, non numero isolato

## Chain Lightning

```text
Blast · Enemy Unit · Chain · Beam · Electric Damage 3 · Max Targets 3 · Range 5
```

Card:

- core: `Electric Damage 3`
- hex: `Enemy`, `Chain`, `Beam`
- valori: `Targets 3`, `Range 5` contestualizzati

## Overwatch

Lettura corretta:

```text
Reaction → Trigger enemy movement/entry → generated attack
```

Esempio attacco generato:

```text
Cone · Projectile · Damage 3 · Range 5
```

La documentazione deve mostrare chiaramente la differenza fra:

1. reazione armata;
2. trigger;
3. attacco prodotto.

---

# 7. Lavoro sulle decisioni

## 7.1 Prima di assegnare un nuovo D-ID

Il repository ha rimosso il vecchio script di prenotazione ID.

Procedura:

```bash
git fetch --prune origin
# leggere l'ultimo D-nnn in docs/decisions/RT_PDR_00_Decision_Log.md
gh pr list --state open
# verificare che nessuna PR aperta rivendichi lo stesso ID
```

Assegnare il **successivo ID libero** solo dopo questa verifica.

## 7.2 Decisione proposta

Titolo consigliato:

```text
Skill Card Grammar: core esagonale e satelliti tipizzati su reticolo radiale 30°
```

La decisione deve dire almeno:

- grammatica compositiva ≠ catalogo runtime;
- significato delle 6 forme (`hex`, `circle`, `small hex`, `square`, `diamond`, `triangle`);
- caps;
- reticolo 30°;
- Reaction/Overwatch non diventano fasi;
- colore della Card Grammar = fase reale;
- questa decisione **non modifica** i valori serializzati di `ERTIconCategory`.

### Impatto

- UI/HUD authoring;
- icon/skill visual language;
- documentazione;
- tool di generazione futuri;
- nessun impatto diretto sul resolver di gameplay;
- nessun impatto sulla privacy: la card rappresenta solo informazioni già autorizzate dal view model.

---

# 8. Documento owner da creare/aggiornare

## Regola

Non lasciare la nuova grammatica solo sotto `docs/research/`.

Prima fai un audit della IA corrente.

### Preferenza

Se esiste già un owner vivo adeguato in `docs/technical/`, **estendilo**.

Se non esiste, crea **un solo documento owner**, con nome coerente con la IA corrente, per esempio:

```text
docs/technical/systems/icon-skill-card-grammar.md
```

**MA NON creare quel path alla cieca.** Prima verifica struttura e naming reale di `docs/technical/`.

Il documento owner deve contenere:

1. scope e non-scope;
2. distinzione grammar/catalog;
3. macro-fasi reali;
4. formula completa;
5. forme della card e caps;
6. reticolo 30°;
7. esempi Rail Shot / Chain Lightning / Overwatch;
8. accessibilità;
9. open points;
10. puntatori a D-031, nuova D-xxx, E20/E25.

### `brief-icone-v01.md`

Aggiornarlo **solo** per:

- link al nuovo owner;
- regole v0.1 che sono effettivamente consumate;
- eventuale nota che la grammar della card non cambia il naming `UI.Icon.<Category>.<Name>`.

Non trasformare il brief v0.1 nel documento enciclopedico dell'intero sistema se non è il suo ruolo.

---

# 9. Issue / Epic — cosa aggiornare

## #217 — E20 HUD Icon Language

Aggiungere una nota/sezione che dica:

- la Card Grammar è una **grammatica compositiva**, separata dal catalogo runtime;
- E20 continua a possedere il minimo v0.1 del catalogo;
- la nuova grammar non cambia le 12 categorie runtime;
- link al documento owner e alla nuova decisione.

**Non aumentare il numero di categorie runtime.**

## #219 — CP 20.2

Aggiornare solo se il lavoro v0.1 di disegno/asset usa direttamente la nuova grammar.

Aggiungere:

- la produzione delle icone deve rispettare silhouette-first e la grammar owner;
- la Card Grammar non cambia `RequiredIconIds()`;
- nessuna nuova categoria è richiesta da questo consolidamento.

## #220 — CP 20.3

Aggiornare solo per ricordare che il widget riceve un `IconId` / dati e non hardcoda la composizione semantica.

La card può essere costruita da un view model, ma **non deve ricalcolare gameplay**.

## #637 — tassonomia manifest/runtime

Aggiungere una nota molto importante:

> `Target`, `Shape/Geometry`, `Delivery`, `HitRule`, `Effect` possono esistere nella **grammatica compositiva** senza richiedere una categoria `ERTIconCategory`.

Questo riduce il rischio di “risolvere” #637 aggiungendo categorie runtime solo perché servono alla card.

## #265 — E25 Icon Language completo

Aggiungere la Card Grammar come fondazione della parte visuale estesa.

E25 deve possedere:

- governance completa;
- authoring;
- tool/export;
- integrazione world-space / reaction / perception;
- accessibility.

La Card Grammar non deve diventare una seconda tassonomia runtime.

## #266 — CP 25.1

Questa è la issue più naturale per recepire governance e separazione degli assi.

Aggiungere acceptance criteria:

- un owner vivo descrive la Card Grammar;
- grammar e catalog hanno ownership separate ma collegate;
- le forme satellite e caps sono dichiarate;
- gli open point restano espliciti;
- nessuna categoria runtime nasce senza consumer.

## #267 — CP 25.2

Collegare la grammar agli strumenti di authoring futuri:

- eventuale JSON/schema della Card Grammar;
- validator di caps/slot/forme solo se esiste un consumer reale;
- nessun campo introdotto “perché il modello lo prevede”.

## #268 — CP 25.3

Dichiarare che HUD/world-space consumano una **vista sanitizzata** della grammar.

Per Overwatch/reaction la UI deve distinguere:

- armed;
- trigger/opportunity;
- generated attack.

## #269 — CP 25.4

Aggiungere esplicitamente test percettivi della Card Grammar:

- grayscale;
- 24 px;
- 32–36 px nello slot;
- card densa al cap massimo;
- `Electric` vs `Reaction`;
- `Cover` vs `Guard` vs `Brace` vs `Shield`;
- Rail Shot / Chain Lightning / Overwatch senza label.

---

# 10. Serve una nuova issue?

**Default: NO.**

Prima prova a far vivere il lavoro dentro E20/E25 esistenti.

Crea una issue nuova solo se, dopo l'audit, il lavoro non ha un owner eseguibile.

Se serve, titolo consigliato:

```text
[DESIGN] Consolidare la Skill Card Grammar radiale nel canone UI
```

Parent naturale:

- E25 se il recepimento riguarda governance/authoring esteso;
- E20 solo se è necessario per chiudere la produzione v0.1.

Non creare una nuova epic.

---

# 11. Roadmap

Aggiornare solo le sezioni che rappresentano stato reale.

## `roadmap-v0.1.md`

- E20: aggiungere puntatore al documento owner se la grammar è necessaria per v0.1.
- Non gonfiare la v0.1 con lavoro post-v0.1.

## `roadmap-post-v0.1.md`

- E25: aggiungere Card Grammar a governance/authoring/accessibility.

## `roadmap-checkpoint.md`

- aggiornare solo se è il punto attuale in cui lo stato dei checkpoint viene mantenuto;
- non duplicare testo lungo: puntare a issue e owner spec.

---

# 12. Open points da preservare

Queste NON sono decisioni chiuse:

1. palette HEX finale delle 6 macro-fasi;
2. marker geometrico finale di ogni fase;
3. posizione esatta dei piccoli esagoni quando ne compaiono 1/2/3;
4. resa densa di `Range / Radius / Duration / Targets`;
5. ingresso di `Object` nel Target set della card;
6. mapping formale Hit Rule → triangle/diamond/mini-glyph quando più regole coesistono;
7. dimensione finale della card composita nell'Action Dock;
8. eventuale schema runtime/data-driven per generare la card.

Non “chiuderli per simmetria”.

---

# 13. Assets / immagini / SVG

Se nel workspace sono disponibili i file di consolidamento visuale, inserirli come **reference/design asset**, non come runtime asset automatici.

Nomi da cercare/recepire se presenti:

```text
RT_Hex_Radial_30deg_Reference.svg
RT_Icon_Card_Master_2026-08-28.svg
RT_Icon_Card_Examples_2026-08-28.svg
```

Riferimenti storici da nominare come provenienza se esistono:

```text
A1_RadialHexGrammar.svg
A2_TacticalSyntaxStrip.svg
A3_ConcentricLayerGrammar.svg
A4_CompactHUDTiles.svg
RefactorTactics_HexGrid_Affinity_v2.svg
```

Non copiare immagini generate in directory runtime senza una decisione di authoring/import.

---

# 14. Verifiche obbligatorie

Prima del commit:

```bash
# 1. Non sono nate categorie runtime per errore
git diff -- Source/RefactorTactics/UI/RTIconCatalogData.h

# 2. Nessun hardcode texture introdotto
# usa i controlli già previsti dal progetto / grep mirato

# 3. Nessun link documentale evidentemente rotto nei file toccati

# 4. Controllare issue/epic esistenti prima di crearne di nuove
gh issue view 217
gh issue view 219
gh issue view 220
gh issue view 637
gh issue view 265
gh issue view 266
gh issue view 267
gh issue view 268
gh issue view 269

# 5. Rileggere il diff completo
git diff --check
git diff
```

Se vengono toccati file C++ per errore, fermati: questo consolidamento dovrebbe essere prevalentemente **Decision + Docs + Issues/Roadmap**.

---

# 15. Definition of Done

Il consolidamento è Done quando:

- [ ] nuova D-xxx registrata senza collisione ID;
- [ ] esiste un owner documentale vivo per la Skill Card Grammar;
- [ ] il documento dichiara grammar ≠ catalog runtime;
- [ ] macro-fasi reali corrette;
- [ ] forme satellite + significato + caps registrati;
- [ ] reticolo 30° registrato;
- [ ] esempi Rail Shot, Chain Lightning, Overwatch presenti;
- [ ] conflitto sul color system dichiarato, non nascosto;
- [ ] #217 / #265 aggiornate;
- [ ] #266 aggiornata come owner di governance, se coerente con lo stato corrente;
- [ ] #637 contiene la distinzione primitive visuali ≠ categorie runtime;
- [ ] roadmap aggiornata solo dove necessario;
- [ ] nessuna nuova epic creata;
- [ ] nessuna modifica a `ERTIconCategory` salvo decisione separata;
- [ ] nessun GameplayTag creato per soddisfare la grammar;
- [ ] nessuna texture hardcoded in widget;
- [ ] diff documentale leggibile e senza duplicazione di owner.

---

# 16. Piano commit consigliato

Preferire commit piccoli e leggibili.

```text
docs(icon): registra decisione skill card grammar radiale

docs(icon): aggiunge owner spec per card grammar

docs(roadmap): collega E20/E25 alla card grammar

chore(github): aggiorna issue E20/E25 icon grammar
```

Se gli update GitHub non fanno parte del commit, documentarli nel corpo PR.

---

# 17. Corpo PR consigliato

```markdown
## Cosa consolida
- Card Grammar radiale per skill icon
- significato delle forme satellite
- reticolo flat-top a 30°
- separazione grammatica compositiva / catalogo runtime
- macro-fasi reali e modello Overwatch/Reaction

## Cosa NON cambia
- `ERTIconCategory`
- `RequiredIconIds()`
- GameplayTag
- resolver / simulazione
- privacy degli intenti

## Issue
- #217
- #219
- #220
- #637
- #265
- #266
- #267
- #268
- #269

## Verifica
- diff documentale letto integralmente
- nessuna nuova categoria runtime
- nessun hardcode texture
- esempi Rail Shot / Chain Lightning / Overwatch inclusi
```

---

# 18. Regola finale per Claude

**Non reinterpretare la grammatica per renderla più elegante.**

Questo lavoro serve a conservare decisioni già prese. Se trovi una contraddizione:

1. misurala;
2. cita i due owner in conflitto;
3. registra l'open point o proponi una decisione;
4. non risolverla silenziosamente.

La priorità è evitare che, cancellate le chat, si perdano le decisioni o si torni a versioni precedenti della grammatica.
