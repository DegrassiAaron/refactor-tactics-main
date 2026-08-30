# Tactical Camera — referto del consolidamento (2026-08-30)

> `CURRENT` · **Ultimo aggiornamento**: 2026-08-30
> **Cosa è**: il referto di consumo del prompt di consolidamento *«Tactical Camera, Epic e GitHub Issues»* —
> cosa è entrato, cosa esisteva già, cosa era prescritto e non poteva entrare così com'era.
> **Cosa non è**: una specifica. Gli owner sono
> [`../../technical/systems/spec-tactical-camera.md`](../../technical/systems/spec-tactical-camera.md),
> [`../../technical/systems/spec-domini-spaziali-mappa.md`](../../technical/systems/spec-domini-spaziali-mappa.md)
> e il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md).
> Il referto del consolidamento **precedente** resta
> [`camera-roadmap-v1-triage-2026-08-14.md`](camera-roadmap-v1-triage-2026-08-14.md), oggi storia
> (`D-254`).

## 1. Il verdetto in una riga

Il prompt descrive una camera in gran parte **già costruita** — pan relativo, zoom ancorato al cursore,
yaw continuo, pitch a runtime, orbita, focus che conserva lo zoom, soft bounds — e il valore del
consolidamento non sta nelle sue prescrizioni: sta nell'aver reso evidente che quel lavoro **non aveva una
sede documentale**, e che due delle prescrizioni non erano prescrizioni ma domande.

## 2. Il conto

| Oggetto | Quanti | Esito |
|---|--:|---|
| Requisiti del prompt (§1–§13) | 13 aree | 12 → issue, 1 assorbita (input dentro le altre) |
| Issue camera **esistenti** su GitHub | 6 | tutte **chiuse** — nessuna riaperta, nessuna duplicata |
| Issue camera **aperte** prima di oggi | **0** | — |
| Epic camera esistenti | **0** | creata **E49** ([#1769](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1769)) |
| Issue create | 12 | [#1770](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1770)–[#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781) |
| Decisioni nuove | 6 | `D-250`–`D-254`, più **`D-255`** dalla risposta a `CAM-A` (§9) |
| Domande aperte registrate | 2 | `CAM-A`, `CAM-B` — ✅ **entrambe chiuse lo stesso giorno** (§9) |
| Spec create | 2 | camera · domini spaziali |
| Prescrizioni **rifiutate** come tali | 2 | vedi §5 — poi *decise*, non applicate di lato |
| Issue diventate codice | 7 su 12 | §9 |

## 3. `CURRENT` — cosa il prompt chiedeva e il codice aveva già

Misurato su `origin/main` il 2026-08-30, non dedotto:

| § prompt | Chiede | Dove è già |
|---|---|---|
| §3 | zoom continuo con rotella | `AddZoom` · `ZoomTowards` ancorato al cursore |
| §3 | `Q`/`E` snap rotazione | `AddYaw`, `YawStep = 45°` (`D-142`) |
| §3 | `F` focus selezione | `FocusOn` + `FocusCameraOnUnit` |
| §3 | `Home` reset orientamento | `RecenterView` |
| §3 | `LMB` select, `RMB` cancel | `SelectAction` · `UndoAction` |
| §2 | yaw, pitch, zoom come stato | `CameraYaw` · `CameraPitch` · `TargetArmLength` |
| §5 | **zoom continuo, non tre modalità** | è già così: nessuna modalità esiste |
| §8 | focus conserva zoom | `Camera.FocusMovesToTargetAndKeepsZoom` |
| §12 | non usare la collisione SpringArm | `bDoCollisionTest = false` dal costruttore |
| — | input rimappabile ed Enhanced Input | `BuildInputMappings`, **interamente C++** |

⚠️ **Il §12 chiedeva di *«non usare la collisione classica della SpringArm … se provoca continui cambi di
zoom»*.** È la stessa premessa che il triage del 2026-08-14 aveva già misurato falsa: la collisione è
spenta dal costruttore, e il comportamento temuto non può verificarsi. Due kit consecutivi hanno posto la
stessa domanda; ora la risposta è una decisione (`D-253`) invece che una riga di codice da ritrovare.

## 4. Il difetto vero: nessuna sede

`RT-FEAT-UI-TACTICAL-CAMERA` non esiste più — il Feature Registry è uscito con `D-181` — e nessun
documento aveva preso il suo posto. La camera viveva in:

- **codice** (`RTCameraPawn.h`, che documenta bene ma solo ciò che c'è);
- **due decisioni** (`D-142`, `D-143`);
- **cinque voci PIE** verdi;
- **un referto di consumo** del 2026-08-14, che per costruzione descrive il passato di una decisione.

Chi voleva sapere *cosa deve fare la camera* non aveva dove guardare. `D-254` chiude il buco con due
owner, separati per consumatore: la spec camera la leggono programmatori, quella dei domini spaziali chi
costruisce livelli.

> 🔴 **Il triage era invecchiato in un modo ingannevole, e va detto.** Dichiara **quattro** automation test
> camera — oggi sono **19** — e indica come owner `docs/roadmap/feature-registry.yaml`, rimosso da
> `26f6955a`. Entrambe le affermazioni erano vere quando furono scritte, ed è esattamente la ragione per
> cui un referto non può fare da specifica: registra un istante, e nessun gate lo rimisura.

## 5. `REJECTED AS PRESCRIPTION` — due cose che non potevano entrare così

Il prompt le presentava come requisiti. Applicarle sarebbe stato introdurle di nascosto.

### 5.1 `CAM-A` — il picking layer-aware **tocca il gameplay**

§7 prescrive `World Hit → X/Y → ActiveLayer → FRTCellId`. Il codice fa l'opposto, e lo dichiara:
`RTPlayerController.cpp:502` — *«Il layer viene dalla QUOTA del punto colpito»*.

Cambiarlo cambia **quale cella un click seleziona**, quindi quale cella si pianifica. Non è presentazione.
Richiede inoltre un `ActiveLayer` di gioco che **non esiste** (quello che c'è è authoring, guidato
dall'editor mode) e la matrice del puntatore appartiene a
[#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705).

→ registrata come domanda in [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md), lavorata da
[#1776](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1776), con commento di legame su #705.

### 5.2 `CAM-B` — il rebinding `MMB` **invalida una verifica passata**

§3 prescrive `MMB drag → Pan` e `Alt+LMB drag → Orbit`. Oggi `MMB` **è** il modificatore d'orbita, e
`PIE-CAM-ORBIT` è verde dal 2026-08-16 descrivendolo tasto per tasto — quella stessa sessione ha chiuso
una decisione aperta (`bInvertOrbitPitch`, provato con le mani).

Non è un'aggiunta: è un cambio con un costo. → domanda in `OPEN_DECISIONS.md`, blocca
[#1771](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1771).

## 6. `PROPOSED` — le cinque decisioni

| ID | Cosa fissa | Perché ora |
|---|---|---|
| `D-250` | i quattro domini spaziali della mappa | il vocabolario non esisteva: **zero** occorrenze in `Source/` |
| `D-251` | limiti del pivot **viewport-aware** | il clamp fisso risponde a *«non perdere la mappa»*, non a *«non mostrare il vuoto»* |
| `D-252` | Strategic View come conseguenza dello zoom, con isteresi | fissa la forma **prima** che qualcuno implementi tre modalità perché sono più facili da nominare |
| `D-253` | occlusione in presentazione; SpringArm solo safety | promuove a vincolo una riga di costruttore che due kit hanno già frainteso |
| `D-254` | la camera prende un owner documentale | il difetto misurato al §4 |

## 7. Cosa questo consolidamento NON fa

- **Non tocca `test-manuali-pie.md`.** Il file ha modifiche non committate di un'altra sessione
  (rimisurazione delle voci del 2026-08-30, sessione MCP): due sessioni sullo stesso file sono un
  conflitto garantito, e la regola di `D-222` vale anche per i documenti. Le voci `PIE-CAM-*` che vanno
  riscritte sono nella DoD delle issue che le invalidano, non qui.
- **Non apre scenari camera.** Zero oggi, e i nomi non si inventano: la convenzione di ScenarioId manca, e
  scriverla è parte di [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780).
- **Non ripara i riferimenti al Feature Registry** sparsi in altri documenti — `spec-pointer-interaction.md`
  ne cita uno. È un debito reale e **non è di questo consolidamento**: toccarlo qui allargherebbe il diff
  su file di altri owner.
- **Non modifica `.uasset` né `.umap`.** `L_CameraFeatureLab` è lavoro Editor, e resta una verifica manuale
  finché non eseguita.
- **Non riapre** #863 #864 #865 #873 #874 #887.

## 8. Ciò che restava da decidere — ✅ risolto lo stesso giorno

> 🔄 **Questa sezione è stata scritta prima del §9 e superata poche ore dopo.** Resta com'era perché il
> punto del referto è *cosa l'audit aveva trovato*, e trovare due domande dove il prompt vedeva due
> prescrizioni è il risultato — non un passaggio da cancellare una volta che le domande hanno risposta.

Erano due, entrambe con un innesco scritto: `CAM-A` (picking layer-aware, insieme a #705) e `CAM-B`
(rebinding `MMB`). Tutto il resto era lavoro, non decisione.

✅ **Risposte dall'autore il 2026-08-30**: `CAM-A` → uscita (b), che diventa **`D-255`**; `CAM-B` →
«entrambi», che **elimina il costo** per cui era una domanda. Dettaglio al §9.

**Nessuna decisione camera resta aperta.** Ciò che resta è lavoro in Editor e tarature — §9.4.

---

## 9. L'implementazione, lo stesso giorno

Dopo l'audit l'autore ha risposto alle due domande del §5 e chiesto di lavorare l'epic. Sette issue su
dodici sono diventate codice: [#1770](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1770), [#1771](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1771), [#1772](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1772), [#1773](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1773) (in
parte), [#1774](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1774) (lo stato), [#1775](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1775) (lo stato), [#1776](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1776), [#1778](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1778).

### 9.1 Le due domande, e cosa hanno cambiato

| Domanda | Risposta | Conseguenza |
|---|---|---|
| `CAM-A` | **(b)** `ActiveLayer` | Diventa **`D-255`**, perché tocca il gameplay |
| `CAM-B` | **«entrambi»** | `MMB` resta l'orbita ⇒ `PIE-CAM-ORBIT` **non** va riscritta |

⚠️ **La risposta a `CAM-B` ha eliminato il costo che rendeva `CAM-B` una domanda.** Era aperta perché il
rebinding avrebbe invalidato una verifica PIE verde; scegliendo «entrambi» quella verifica resta valida e
non c'è niente da riscrivere. È l'esito migliore possibile, ed è arrivato per aver posto la domanda invece
di applicare la prescrizione.

### 9.2 La decisione che ha cambiato sede

`D-255` doveva decidere anche **dove vive** `ActiveLayer`, e la risposta non era libera: da quella
decisione il piano attivo determina *quale cella un click seleziona*. Metterlo su `ARTCameraPawn` avrebbe
reso la camera un'autorità sull'esito — cioè avrebbe violato `D-143` **nel commit che dichiara di
rispettarlo**. Vive quindi in `ARTPlayerController`, accanto a `PlayerTeamId`.

### 9.3 Il ritrovamento

`PlayerInput.HotkeysDoNotCollide` **era citato in un commento e non esisteva**: `grep -rn "DoNotCollide"
Source/` rispondeva una sola riga — quel commento. L'unica difesa contro due azioni sullo stesso tasto era
un elenco scritto a mano in un secondo commento, cioè una promessa che invecchia al primo `MapKey`
aggiunto — e `#1771` ne aggiunge quattro. Il test ora esiste e interroga il `UInputMappingContext` reale.

⚠️ È la stessa classe di difetto che il §4 di questo referto descrive per il triage del 2026-08-14: **un
documento che descrive uno stato invece di misurarlo**. Qui il documento era un commento C++.

### 9.4 Cosa resta, e perché non è codice

Le cinque issue non iniziate — [#1777](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1777), [#1779](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1779), [#1780](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1780), [#1781](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1781), più le
metà mancanti di #1773/#1774/#1775 — hanno in comune di **richiedere l'Editor**: buffer scenico, cutaway e
Feature Lab sono mappe, volumi e materiali.

🔴 **#1780 è la più bloccante, e non per la ragione ovvia.** Sette issue hanno lasciato **tarature aperte**
— soglia click/drag, limite e velocità del peek, sensibilità del precision pan, le due soglie strategiche,
`AllowedOutsideFraction` — e ognuna ha un default scelto perché plausibile. Finché non esiste un posto
dove guardarli, quei numeri diventano canone per inerzia: è esattamente il difetto che il principio 4
dell'epic vieta, e che rimandare la Feature Lab produce da sé.

### 9.5 La misura, e i due tentativi che non contano

| # | Filtro | Su | Esito | Registrabile |
|---|---|---|---|---|
| 1 | `RefactorTactics.Camera` | `56849dac` | 28/28, 0 fail · `VALIDA` | ✅ sì — ma non copriva l'ultimo commit |
| 2 | `RefactorTactics` | `51e6635d` | 1428/1428, 0 fail | ❌ **no** — `HEAD` cambiato a run iniziata |
| 3 | `RefactorTactics` | **`aedc4656`** | **1442/1442, 0 fail · `VALIDA`** | ✅ **sì** |

🔴 **La 2 è l'episodio che vale registrare**, e non perché sia andata male: il risultato era identico alla
3. A run iniziata un'altra sessione ha fatto checkout nella working directory condivisa, e `HEAD` è passato
sotto i piedi della misura. `scripts/rt-suite.ps1` l'ha rilevato da solo ed è uscito `3` — è esattamente il
caso per cui `D-222` esiste, ed è la prima volta in questa sessione che quel meccanismo ha *dovuto*
funzionare.

⚠️ **Non è stata riclassificata dopo.** Sapere che la 3 conferma la 2 non rende la 2 valida: una misura
vale o non vale **prima** di sapere come è andata, altrimenti il criterio diventa «era giusta, quindi
contava» — che non è un criterio.

⚠️ **E la 1 non bastava**, benché valida: era su `56849dac`, e il commit successivo aveva modificato i
test. Fra le due c'è la differenza fra *«la suite è verde»* e *«la suite è verde su ciò che sto per
mergiare»*.
