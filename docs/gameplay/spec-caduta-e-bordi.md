# Caduta e bordi aperti — spec di gameplay

> `CURRENT` · **Owner semantico** della caduta gravitazionale e della qualificazione dei bordi.
> **Release**: v0.1, per [`D-332`](../decisions/RT_PDR_00_Decision_Log.md) (2026-09-05).
> **Capability**: `CR-VERT` · **Epic**: [#2388](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2388)
> **Cosa possiede**: le regole di risoluzione della caduta e il vocabolario del bordo.
> **Cosa NON possiede**: i numeri (materia `BAL-*`), il resolver (`ARTTurnManager`), l'authoring
> ([#1861](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1861)), la traccia
> ([#1881](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1881)).

---

## 1. Che problema risolve

La mappa è un grafo esagonale **multilivello** dal primo giorno: `FRTCellId{X, Y, Layer}`, A\* multilivello,
LOS che attraversa i layer, ponti di `CP 9.4`. Ma il piano verticale finora era **solo topologia**: si sale e
si scende dove un arco lo consente, e altrove non si passa.

Oggi una spinta che punta oltre il bordo di una piattaforma **si ferma**. `StepUntilBlocked`
(`RTHexCombatLibrary.cpp:540-554`) esce sulla cella libera precedente quando la successiva non esiste, e
costruisce ogni passo su `Target.Layer` **costante**.

⛔ **Non è un difetto**: è l'assenza di una regola. Questo documento è quella regola.

---

## 2. Vocabolario del bordo

Un lato di cella può essere, per il passaggio:

| Qualifica | Significato | Come si sa | Effetto sulla caduta |
|---|---|---|---|
| **percorribile** | adiacenza planare normale | già canone | nessuno |
| **bloccante** | muro, copertura alta, porta chiusa | già canone — `bBlocksMovement`, `FRTHexCover`, `FRTHexDoor` | nessuno |
| **aperto** | nessuna cella adiacente su quel lato, nello stesso layer | **derivato**: `URTHexLedgeLibrary::IsEdgeOpen` | uno spostamento che lo attraversa **cade** |
| **parapetto** | il lato dà sul vuoto ma è protetto | **autorato**: `FRTHexEdgeGuard` | lo spostamento **si ferma**, come oggi |

🔑 **Delle quattro qualifiche una sola è dato nuovo**, e la colonna «come si sa» è il motivo. Due esistono
già con i loro owner; *aperto* è **derivabile** dalla mappa — l'assenza di un vicino è scritta lì. Solo la
**negazione** di una caduta altrimenti implicita non è deducibile da niente, ed è lo stesso argomento che
`ERTHexBodyFill` usa per sé: un ponte e una collina hanno entrambi il vuoto sotto, e nessun segnale
geometrico li distingue.

⛔ **Non si autora l'apertura.** Sarebbe una seconda sede per un fatto che la mappa già contiene, e andrebbe
fuori sincrono al primo ridisegno.

🔑 **Dove vive**, e perché la sede ovvia è sbagliata. Non su `FRTHexEdge`: quella struttura dichiara di sé che
*«L'arco è ADDITIVO: crea un collegamento dove non c'era. È la ragione per cui le PORTE non stanno qui ma sui
bordi (CP 9.3) — negare un'adiacenza planare richiede un oggetto sottrattivo, e questo non lo è»*
(`RTHexCellData.h:457`). Un bordo aperto **qualifica** un lato planare, un parapetto lo **nega**: entrambi
vivono dove vivono `Covers` e `Doors`, cioè su `FRTHexCellData`.

⛔ **Nessun `BalconyCell`.** Balconi, tetti, passerelle e cornicioni non sono un tipo di cella: emergono da
cella, layer, bordo e relazione di atterraggio. Un tipo nuovo sarebbe una seconda rappresentazione
topologica, vietata da [`AGENTS.md`](../../AGENTS.md) §3.

---

## 3. La sequenza, e il suo ordine

L'ordine è la regola, non un dettaglio d'implementazione:

```text
spostamento forzato
  → attraversamento di un bordo aperto
       → lo spostamento orizzontale TERMINA        (i passi residui sono persi)
            → risoluzione della caduta             (gli effetti si applicano SEMPRE)
                 → [impatto sull'occupante]
                      → esito di atterraggio  →  posizione finale
```

⛔ **Mai il verso opposto** — *«prima decido dove atterra, poi vedo se la caduta è avvenuta»*. Gli effetti
non dipendono dalla disponibilità della cella finale: è ciò che rende leggibile il caso saturo del §4.3
invece che un difetto.

### 3.1 Lo spostamento termina, e i passi residui sono persi

Una spinta da 3 celle che incontra un bordo aperto al primo passo non prosegue dopo l'atterraggio. La caduta
**consuma** il resto dello spostamento.

### 3.2 Il percorso volontario è annullato

Un'unità spostata a forza mentre stava percorrendo il proprio piano **perde il resto del piano**. Nessuna
reinterpretazione dalla nuova posizione, nessun ricalcolo.

⚠️ È il divieto di auto-reroute già canone in [`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md)
§2 — `mai` sulle tre famiglie che percorrono celle, `n/a` sul Transfer — applicato a una causa nuova. Non è
una regola nuova: è la stessa regola che incontra la caduta.

---

## 4. I tre esiti di atterraggio

L'atterraggio primario è la cella sottostante il punto di uscita. Gli esiti sono **tre**, e si distinguono
nella traccia.

### 4.1 Primario libero

Effetti di caduta a chi cade, collocazione sul primario. È il caso normale.

### 4.2 Primario occupato, alternativa disponibile

1. chi cade riceve gli **effetti di caduta**;
2. l'occupante riceve gli **effetti d'impatto**;
3. **nessuna condivisione di cella**;
4. si scandiscono le celle adiacenti al primario a partire dal **Facing dell'occupante**;
5. in **un solo ordine rotazionale canonico**;
6. si sceglie la prima candidata legale, non-`Void`, raggiungibile e libera;
7. chi cade viene collocato lì.

🔑 **Il Facing è un tie-break locale, non una fonte d'ordine.** L'anello canonico esiste già ed è
`E → NE → NW → W → SW → SE` (`ERTHexDirection`, `RTCellId.h:11-19`): non va ri-derivato e non va riordinato.

### 4.3 Primario occupato, nessuna alternativa

Il caso saturo. Primario occupato e **tutte** le alternative indisponibili per unità, muri o bordi dinamici,
`Void` o altri blocchi legittimi.

1. la caduta **è avvenuta**, ai fini degli effetti;
2. chi cade riceve i normali effetti di caduta;
3. l'occupante riceve i normali effetti d'impatto;
4. l'occupante **resta** dov'è;
5. chi cade termina su `LastStableCell` — la cella stabile immediatamente prima del bordo aperto;
6. lo spostamento residuo è perso;
7. nessuna collisione ricorsiva, nessuna spinta a catena, nessun pathfinding remoto, nessun RNG, nessuna
   sovrapposizione persistente.

🔑 **`LastStableCell` non è codice nuovo.** È già ciò che `StepUntilBlocked` produce: *«ci si ferma sulla
cella libera precedente»*. Ciò che manca sono gli effetti e la traccia.

⚠️ **`LastStableCell` non va trattata come sicuramente libera** finché la risoluzione della caduta non
committa: nello stesso Blast un'altra unità può averla presa.

Modello di risoluzione consigliato:

```text
LastStableCell → caduta candidata → risolvi effetti ed esito → committa la posizione finale
```

Non committare la posizione in basso per poi risalire, a meno che l'architettura del resolver lo imponga e
determinismo e occupazione restino corretti.

---

## 5. Gli effetti non sono la posizione

```text
effetti di caduta  ≠  esito di atterraggio
```

Chi cade riceve gli effetti **anche** quando la cella finale non è occupabile. Sono due domande separate e
vanno risolte separatamente.

⛔ **Nessun numero è deciso qui.** Il danno da caduta, la sua scala con il dislivello e gli effetti d'impatto
sono materia di bilanciamento (`BAL-*`) e del catalogo. Questo documento dice **che** si applicano e
**quando**, non **quanto**.

Guardrail di baseline, finché il bilanciamento non dice altro:

- nessun `Stagger`/`Prone`/`Stun` automatico da **ogni** caduta;
- nessun ring-out universale;
- `Void`/KO solo dove è **authored**.

### 5.1 L'omonimo da non fondere

Nel repository *«la caduta»* è già una cosa: `Prone`, da [`D-319`](../decisions/RT_PDR_00_Decision_Log.md) —
chi subisce uno spostamento forzato **mentre è `Unbalanced`** finisce a terra.

⛔ **Le due regole non si fondono e non si annullano.** La caduta gravitazionale non applica `Prone` per sé.
Ma un'unità `Unbalanced` spinta oltre un bordo aperto le attraversa **entrambe**, e l'ordine è quello del
resolver: `D-319` agisce sullo **spostamento forzato**, questa spec su ciò che accade **dopo** che lo
spostamento è terminato.

---

## 6. La caduta non è un Transfer

Può condividere l'infrastruttura di ricollocazione, ma **conserva la propria causa e i propri eventi**.

⛔ Non eredita le immunità né i trigger del teletrasporto: un'abilità che protegge dal `Transfer` non
protegge dalla gravità, a meno che qualcuno lo decida esplicitamente.

Nella tassonomia di [`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md) la caduta appartiene al
**Traversal forzato** fino al bordo, e la discesa è un evento proprio.

---

## 7. Validazione d'authoring

L'authoring rifiuta normalmente un atterraggio **staticamente isolato**, con reason code e senza correzione
automatica.

Un'alternativa statica valida esiste, è legalmente occupabile, non è `Void`, ed è topologicamente
raggiungibile dall'atterraggio — **senza dipendere dall'occupazione a runtime**.

⚠️ Questa validazione **non** rimuove il fallback del §4.3: muri, bordi e unità creati in partita possono
chiudere un'area nata valida. Sono due garanzie diverse, in due momenti diversi.

---

## 8. Fuori scope in v0.1

Collisione ricorsiva · spinta a catena · `Momentum` generico · scivolamento casuale · modello del ghiaccio
nuovo · resistenza o distruttibilità del parapetto · `Hanging` · ring-out universale · input del giocatore
durante la risoluzione · policy `MOV-4` nuova ([chiusa da `D-325`](../decisions/RT_PDR_00_Decision_Log.md)) ·
auto-reroute · pathfinding tattico di atterraggio · evitamento hazard · fisica continua come autorità ·
seconda rappresentazione topologica · secondo resolver.

---

## 9. Che cosa deve dire la traccia

La catena causale è leggibile per intero:

```text
spostamento forzato → bordo aperto → caduta → [impatto] → esito di atterraggio
```

L'esito distingue almeno **primario**, **alternativa adiacente** e **fallback su `LastStableCell`**.

🔴 **I valori nuovi si aggiungono in coda.** `ERTMoveOutcome` ed `ERTDisplacementBlockReason` viaggiano nel
TurnLog come **indice** e il formato è oggi `ERTTurnLogFormatVersion::WithMicroStep` = **12**: estendere in
coda non è una migrazione, riordinare o cambiare esiti già prodotti sì, con rigenerazione del corpus golden
([`D-245`](../decisions/RT_PDR_00_Decision_Log.md)).

---

## 10. Documenti correlati

| Documento | Rapporto |
|---|---|
| [`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md) | famiglie di movimento e divieto di auto-reroute |
| [`brief-stati-unbalanced-prone.md`](brief-stati-unbalanced-prone.md) | `D-319`, l'omonimo del §5.1 |
| [`spec-terreni-e8.md`](spec-terreni-e8.md) | `ERTHexSurface`, incluso `Void` |
| [`spec-ponti-cp94.md`](spec-ponti-cp94.md) | archi di transizione e stato |
| [`../roadmap/plans/verticalita-ledge-fall-dual-roadmap-2026-09-05.md`](../roadmap/plans/verticalita-ledge-fall-dual-roadmap-2026-09-05.md) | audit e piano di esecuzione |
