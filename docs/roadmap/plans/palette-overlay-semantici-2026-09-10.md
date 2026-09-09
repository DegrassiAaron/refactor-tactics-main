# La palette degli overlay semantici — analisi di `D-364`

> **Stato: ACCETTATA come [`D-364`](../../decisions/RT_PDR_00_Decision_Log.md) il 2026-09-10.**
> Questo documento e' l'**analisi lunga** dietro quella voce: la decisione vive nel Decision Log, qui stanno
> le misure, i comandi che le producono e le due domande che restano aperte.
> Aperta il **2026-09-10** dal brainstorm su #1941 (famiglia OVL, Epic #1769).
> ⏱️ **Nata in `docs/decisions/` come proposta, spostata qui all'accettazione**: `docs/decisions/` contiene
> ADR accettati e il Decision Log, non documenti di lavoro. Gli spec-panel della famiglia OVL stanno in
> questa cartella, ed e' qui che si cerca.

---

## Cosa decide, in una riga

Quali tinte portano i significati dell'overlay tattico, dopo che #1941 ha costruito la **sede** e nessun
valore è stato scelto.

## La forma di questo documento è imposta da `D-363`

`D-363` — *«una decisione nomina il parametro e la sua sede di configurazione; il valore è dato, non
decisione»* — ha un limite che dichiara da sé, ereditato da `D-307`:

> *«la regola non è «mai un numero»: è **distinguere determinato da scelto, e dichiarare quale dei due**. Un
> valore **determinato** si scrive col **vincolo che lo produce**; uno **scelto** si scrive col **nome e la
> sede**.»*

∴ questo documento è diviso in due: ciò che è **scelto** e ciò che è **determinato**. Le tinte esatte stanno
nella seconda metà, e non sono la decisione.

---

# Parte 1 — Ciò che è SCELTO

Sono le tre voci che richiedono un giudizio d'autore. Senza di esse la Parte 2 non ha input.

## S1 · La sede

`URTOverlayPalette::ColorFor(ERTOverlayMeaning)` — **già in `main`** (#1941, PR #2808, commit `f40abc5c`).

Verificabile: `git grep` di un valore della palette dà **una** occorrenza, non una per consumatore. La sede
non è oggetto di questa proposta: esiste, ed è il motivo per cui i valori si possono cambiare in un punto.

## S2 · Lo sfoltimento — quali voci sono `Meaning` e quali no

Due delle tinte disegnate oggi non sono significati semantici, e toglierle dalla palette **fa sparire due
collisioni invece di risolverle**:

| Voce | Proposta | Perché |
|---|---|---|
| `FriendlyFire` | **modificatore** su `Attack`, reso come forma | Non è un'area: è un **sottoinsieme marcato** di `PreviewHitCells`, stesso produttore (`MakeBlastPreview` restituisce `HitCells` e `AllyCells` insieme). L'invariante è **già asserita** da `RefactorTactics.Preview.AllyInAreaIsFlagged` — *«resta comunque fra le celle colpite»* |
| `Hover` | **Interaction Context**, non area semantica | È guidato dal puntatore, non dall'autorità di gioco. Owner: #1614 |

⚠️ `PathTrace` **resta** un `Meaning`, e la simmetria che sembrava ovvia non regge: le raggiungibili si
calcolano **dopo** i waypoint — *«il ventaglio risponde a «quanto mi **resta**», non a «quanto avevo»»*
(`RTPlayerController.cpp`, #877). I due insiemi non sono annidati.

⛔ **`D-363` avverte che un nome può mentire.** Se `FriendlyFire` diventa una forma, la voce
`ERTOverlayMeaning::FriendlyFire` porta un nome che dichiara un significato mentre ne rappresenta un
modificatore. Va rinominata insieme alla decisione, non dopo.

## S3 · `D-146` vale meno sugli overlay che sulle superfici

`D-146` — *colore **E** forma, mai solo il colore* — è stata misurata sulle **superfici**. La proposta è di
**restringerla**, con questa ragione:

> Una superficie si legge **senza un prima** — *«cos'è questa cella?»*. Un overlay compare **perché l'hai
> chiesto tu**: quando vedi il ventaglio verde è perché hai appena chiesto *«dove posso andare»*. Il
> giocatore ha un prior che sulla superficie non ha.

🔴 **Perché la restrizione serve: senza, non esiste alcuna soluzione.** Se il colore dovesse portare da solo
il canale in scala di grigi per sette overlay contro nove superfici più il marcatore di blocco:

```
17 luminanze pairwise >= 20  ->  span minimo richiesto = 16 x 20 = 320
                                 span disponibile (luma 0..255)   = 255   -> IMPOSSIBILE
```

E non è un margine stretto: le nove superfici **non lasciano un buco** dove infilare un overlay —

```
luma superfici:  85 · 107 · 132 · 157 · 160 · 185 · 190 · 195 · 206
gap massimo fra due adiacenti: 25    (ne servirebbero 40: >= 20 da entrambi i lati)
-> restano due sole fasce libere: luma < 65 e luma > 226
```

Due fasce per sette significati. Senza `S3`, la Parte 2 non ha soluzioni — non «ne ha di brutte»: non ne ha.

### ⚠️ La crepa di `S3`, registrata e non risolta

`S3` regge **finché l'overlay è una risposta a una domanda del giocatore**. Ma #1944 vuole `Hazard` e
`Objective` visibili nella *default/neutral view*, **senza che il giocatore attivi nulla** — cioè senza un
prior. Per quei due l'asimmetria non copre.

Tre uscite, e **non ne scelgo una**:

1. `S3` si applica solo agli overlay **richiesti**; `Hazard` e `Objective` restano sotto `D-146` piena e
   ricevono una forma;
2. `Hazard` e `Objective` non stanno nella default view — ma è una modifica a #1944, non a questa proposta;
3. `S3` si applica a tutti, accettando che due significati siano più deboli in scala di grigi.

📌 **Questa è la domanda aperta principale del documento.**

---

# Parte 2 — Ciò che è DETERMINATO

Dati `S1`, `S2` e `S3`, le tinte **non si scelgono**: le produce un vincolo.

## Il vincolo

1. distanza **Manhattan RGB >= 60** fra ogni coppia di tinte, e fra ogni tinta e le nove superfici di
   `URTHexLibrary::SurfaceColor` più `URTHexLibrary::BlockedCellColor()`. Formula e soglia **non sono
   scelte qui**: sono quelle già in uso in `RefactorTactics.Hex.SurfaceColorsAreDistinguishable`;
2. a parità di vincolo soddisfatto, **deviazione minima dai valori della spec v0.2** — perché la spec è la
   proposta di partenza e ogni unità di scostamento va giustificata.

## Il risultato

| Meaning | v0.2 | determinato | scostamento |
|---|---|---|---|
| Movement | `#35C759` | `#35C759` | 0 |
| Vision / LOS | `#32ADE6` | `#32ADE6` | 0 |
| Ability Range | `#AF52DE` | `#AF52DE` | 0 |
| Attack | `#FF453A` | `#FF453A` | 0 |
| PathTrace | *(assente in v0.2)* | `(40, 220, 220)` — valore spedito | 0 |
| **Hazard** | `#FF9F0A` | **`#F79F0A`** | **8** (solo R) |
| Objective | `#FFD60A` | `#FFD60A` | 0 |
| **Invalid** | `#8E8E93` | **`#828E93`** | **12** (solo R) |

**Venti unità in tutto.** La spec v0.2 non era sbagliata: era corta di due ritocchi su un canale ciascuno.

🔑 **Il ritocco di `Hazard` ne ripara due con una mossa**, perché gli `8` entrano in entrambe le distanze:

```
Hazard vs Objective            55 -> 63
Hazard vs superficie Fire      59 -> 67
Invalid vs superficie Floor    49 -> 61
```

Esito sulla palette completa: **nessuna coppia sotto soglia**, né fra le tinte né contro le nove superfici e
il marcatore di blocco.

## ⚠️ Il margine più stretto non è nessuno di questi

```
  60  PathTrace ~ superficie Conductive     <- esattamente sulla soglia
  61  Invalid   ~ superficie Floor
  63  Hazard    ~ Objective
  63  Vision    ~ superficie ShallowWater
```

`PathTrace (40,220,220)` contro `Conductive (80,230,230)` vale **60 esatti**: passa il test (`>= 60`) con
margine **zero**. È una coppia **già spedita** che nessuno aveva misurato, e la propagazione elettrica è un
sistema vivo (`RefactorTactics.*` elettrico, scenari `WaterElectric`).

📌 **Seconda domanda aperta**: dentro o fuori da questa decisione. È lo stesso difetto del fuoco amico sulla
superficie di fuoco — *l'overlay che serve di più proprio su quel terreno è quello che vi si legge peggio*.

---

# Cosa questa proposta NON decide

- ⛔ **La forma** che porta il canale non cromatico dove serve ancora — la faccia della cella è già occupata
  dal glifo ad anelli (`D-183`, `9,7%`–`32,9%` dell'area) e dal velo di conoscenza (`D-227`, RGB × `0,35`).
- ⛔ **Il depth test** della ribbon: è #1942, e quella issue dichiara che l'oracolo è il PIE.
- ⛔ **Quando** un significato è Primary o Secondary: è #1943.

# Cosa costa accettarla

⚠️ `PIE-PREVIEW-AREA` è ✅ con un giudizio umano firmato sull'arancione del fuoco amico. Cambiando canale la
voce va **rimisurata**.

🔑 **È un costo di verifica, non un veto** — e questo corregge una cautela scritta nel body di #1941 e che
avevo ripetuto. Il criterio **6** di #956 (issue **CLOSED**, quindi accettato) dice:

> *«I colori restano placeholder sostituibili: il vincolo è la **ridondanza**, non la tavolozza.»*

∴ nessuno ha imparato dei placeholder. *(Reperto della sessione `1ca3c609`, 2026-09-09, su #1941.)*

# Perché il numero è arrivato tardi

La prima stesura del brainstorm annunciava questa decisione come `D-363`, misurato come primo libero il
2026-09-09. **Il numero è stato consumato da un'altra sessione mentre il lavoro era in corso**, ed è toccato
proprio a `D-363` — la voce che prescrive di non incidere i valori volatili.

Le occorrenze pubblicate sono state ripulite (body di #1941 e i commenti di quella sessione). Il numero e'
stato **rimisurato all'inserimento** nel Decision Log — `D-364`, massimo registrato `D-363` — che e' la sola
regola che tiene: si misura quando si scrive, non quando si progetta.
