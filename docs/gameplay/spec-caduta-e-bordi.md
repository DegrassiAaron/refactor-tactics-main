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

⚠️ **I tre esiti presuppongono che un primario ESISTA**, e c'è un quarto caso che non è un esito di
atterraggio: una colonna **senza fondo**, dove `FindLandingCell` non trova nessuna cella sotto il punto di
uscita. Non è una caduta con un atterraggio brutto — è una caduta senza atterraggio, e la garanzia è
un'altra: l'unità **resta su `LastStableCell`** e non esce dal mondo. Lo copre
[#2402](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2402) D006, e il test è
`Fall.NoCellBelowStaysOnLastStable`.

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

### 4.3.1 L'invariante di occupazione

🔑 **Normativo è l'osservabile, non il modello di risoluzione.** Al termine del Blast:

1. **nessuna cella ha due occupanti**, qualunque sia il numero di cadute che il Blast contiene;
2. l'esito è **invariante per permutazione** dell'ordine in cui le cadute sono risolte — la stessa proprietà
   che [#2402](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2402) D002 promette per una
   caduta sola, estesa a N;
3. nessuna unità esce dal mondo: chi non ha atterraggio resta su `LastStableCell` (§4 ⚠️).

⛔ **Questo non vincola l'architettura del resolver.** Committare in basso e risalire resta legittimo se i
tre punti reggono. Il documento dichiara ciò che si misura da fuori, non come arrivarci: prescrivere la
sequenza interna sarebbe un secondo resolver scritto in prosa, vietato dal guardrail 2.

🔴 **Il blocco precedente non era falsificabile, ed è la ragione della riscrittura.** Diceva *«modello di
risoluzione consigliato»* e poi *«a meno che l'architettura del resolver lo imponga»*: un requisito con
un'eccezione discrezionale è soddisfatto da qualunque implementazione, inclusa quella che sovrappone.
Registrato in [`D-353`](../decisions/RT_PDR_00_Decision_Log.md).

Modello di risoluzione **raccomandato** — non normativo, e il più semplice a soddisfare l'invariante:

```text
LastStableCell → caduta candidata → risolvi effetti ed esito → committa la posizione finale
```

⚠️ **L'invariante non è coperto oggi.** Dei **14** test di `Tests/RTFallOverLedgeTests.cpp`,
`Fall.NeverOverlaps` (riga 641) mette in scena **una** caduta e un occupante fermo; nessuno ha **due** unità
che cadono nello stesso Blast. Lo chiude `Fall.TwoFallersSameLandingIsDeterministic`, con la mutazione
corrispondente nel gate [#2406](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2406).

---

## 5. Gli effetti non sono la posizione

```text
effetti di caduta  ≠  esito di atterraggio
```

Chi cade riceve gli effetti **anche** quando la cella finale non è occupabile. Sono due domande separate e
vanno risolte separatamente.

### 5.1 I numeri, decisi da `D-357`

| Chi | Subisce |
|---|---|
| chi cade | **5** danni · `Status.Exposed` per **1** turno |
| l'occupante centrato (§4.2) | **5** danni, **nessun** `Exposed` |
| dislivello | **nessuna scala**: il danno è piatto |

🔑 **Il danno è minimo perché il prezzo è già pagato.** Far cadere qualcuno richiede una condizione
posizionale rara: `Action.Push` ha **range 1**, **spinta 1** e **nessun danno proprio**, quindi il bersaglio
deve essere **già** adiacente al bordo. E chi cade perde la posizione — scende di un piano e deve cercare un
arco di risalita. **5 su 90-120 HP**, un quarto di `Charge`, dice *«è successo qualcosa»* senza spostare
l'aritmetica dello scontro.

🔑 **La minaccia viene da `Exposed`, non dal danno**: *«+5 al primo danno diretto»* significa che chi ti
butta giù ti **prepara per il colpo dopo**. Il bordo è pericoloso perché apre una combo. ⛔ `Slow` è stato
scartato perché dimezzare il movimento di chi deve risalire punisce due volte sullo stesso asse.

⚠️ **L'asimmetria sull'occupante è semantica**: `Exposed` significa *«hai perso l'equilibrio»*, e chi stava
fermo non l'ha perso. Prende l'urto, non il marchio.

🔑 **Nessuna meccanica nuova**: è la forma con cui `Action.Sprint` paga la propria corsa —
`FRTActionEffectSpec(ERTActionEffect::Status, TAG_Status_Exposed, 1)`. Sprint risolve nella fase `Move`,
anch'essa **dopo** il `Blast`.

⛔ **Perché nessuna scala.** Il dislivello **non è `Layer - 1`**: `FindLandingCell` scandisce la colonna e
tiene il massimo, perché *«la colonna può saltare dei piani»*. Una scala andrebbe definita sui **piani
attraversati**, e finché le mappe della v0.1 non dichiarano più di due layer sarebbe un numero fisso con più
codice attorno.

⚠️ **Ciò che resta materia `BAL-*`** è la revisione di questi numeri, non la loro esistenza: la taratura
torna in bilanciamento con l'evidenza di playtest, non con un'argomentazione.

🔴 **Lo scope è diviso, e la divisione è misurata.** `FallEffects`, `ImpactEffects` e `FallDamage` non
hanno **né definizione né uso** in `Source/`: il criterio *«chi cade prende gli effetti di caduta»* non è
testabile finché quel dato non esiste, e un test che lo asserisse misurerebbe la propria invenzione.

> 🔴 **La formula precedente si è auto-invalidata, e la correzione è la formula.** Diceva *«zero occorrenze
> in `Source/`»*; misurato su `13ddc500`, `FallDamage` è a zero ma `FallEffects` e `ImpactEffects` hanno
> **una occorrenza ciascuna** — `Tests/RTFallOverLedgeTests.cpp:12`, il commento che ripete questa stessa
> frase. È il difetto che [#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) rileva
> per `FastReactionDuration` (*«una sola occorrenza, dentro un commento»*), e la forma che regge nel tempo
> è quella usata dal Decision Log: **le sole occorrenze sono le note che lo dichiarano**.
∴ [#2402](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2402) consegna **posizione, esito e
traccia**; gli effetti sono [#2430](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2430).

Guardrail di baseline, finché il bilanciamento non dice altro:

- nessun `Stagger`/`Prone`/`Stun` automatico da **ogni** caduta;
- nessun ring-out universale;
- `Void`/KO solo dove è **authored**.

### 5.2 L'omonimo da non fondere

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

L'esito distingue **primario**, **alternativa adiacente** e **fallback su `LastStableCell`** — e il quarto
caso del §4 ⚠️, la colonna senza fondo, che non è un atterraggio brutto ma un'assenza di atterraggio.

### 9.1 In quale forma si distinguono

🔑 **Valori distinti in coda a `ERTMoveOutcome`, non un campo nuovo.** La forma non è libera: due coppie
dell'enum hanno già risolto lo stesso problema — `Displaced` / `DisplacementResisted` e `Slid` /
`SlideBlocked`. Uno spostamento che riesce e uno che si arresta sono **due esiti**, non un esito con un
attributo.

| Esito | Valore | Sede |
|---|---|---|
| primario libero (§4.1) | `Fell` — **esiste**, `RTTurnLog.h:670`, indice **15** | invariato |
| alternativa adiacente (§4.2) | `FellToAlternative` | in coda |
| fallback saturo (§4.3) | `FellToLastStable` | in coda |
| nessun atterraggio (§4 ⚠️) | `FellWithoutLanding` | in coda |

⛔ **Perché non un campo dedicato**: costerebbe un bump di `ERTTurnLogFormatVersion` — oggi
`WithSightBlocker` = **13**, misurato su `13ddc500` — e quindi una migrazione, per distinguere quattro casi
che l'enum distingue senza. ⛔ **Perché non `ERTDisplacementBlockReason`**: la caduta non è un blocco, e
riusarlo direbbe che lo spostamento è stato impedito quando invece si è concluso.

⚠️ **`Fell` si specializza, e questo non riscrive nessuna traccia.** Da qui in poi `Fell` significa
*«atterrato sul primario»*, non *«caduto»*. Nessuno degli otto scenari del corpus golden attraversa un bordo
aperto — `Combat.CounterStrikesBack`, `Movement.Basic`, `Movement.Collision`, `RT_Showcase_Relay_v01`,
`Spec.Environment.WaterQuenchesFire`, `Spec.Overwatch.HoldThenFire`, `Spec.Predictive.WhiffOnEmptyCell`,
`Visual.Combat.FallbackTargetMoved` — quindi nessuna traccia persistita contiene l'indice 15, e la
specializzazione non cambia il significato di niente di già scritto. La verifica definitiva è del gate
[#2406](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2406).

### 9.2 Il parapetto ha un esito proprio

Un bordo **aperto** fa cadere, un **parapetto** ferma: sono le due qualifiche del §2 che il vocabolario di
[#2401](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2401) ha creato, e la traccia le
distingue entrambe da un muro.

| Caso | Esito |
|---|---|
| l'unità si muove e il parapetto la tiene sul ciglio | `ERTMoveOutcome::StoppedByEdgeGuard` |
| il parapetto è **adiacente**: nessuna cella utile, l'unità non si muove | `ERTDisplacementBlockReason::EdgeGuard`, con `DisplacementResisted` |
| muro, unità, bordo mappa | invariati — `Displaced` o `NoDestination` |

🔑 **Due valori e non uno, perché lo spostamento forzato ha due rami** — gli stessi due che
[#2402](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2402) D006 ha dovuto gestire per la
caduta: quello che sposta e quello che finisce in `NoDestination` con `Dest == cella di partenza`. Un solo
valore ne coprirebbe uno e lascerebbe l'altro indistinguibile da un muro.

⛔ **`ERTDisplacementBlockReason::Guarded` è un'altra cosa** e non si riusa: significa *«`Action.Guard` ha
retto»*, cioè una **decisione dell'unità**, mentre il parapetto è **geometria della mappa**. La tassonomia
dell'enum separa già le due famiglie — *«due sono decisioni dell'unità, tre sono geometria del turno»* — e
fonderle cancellerebbe la distinzione che quel commento esiste per fare.

🔑 **La query esiste già**: `FRTHexCellData::HasGuardOn(Edge)` (`RTHexCellData.h:463`), e la direzione è già
ricostruita a valle da `DirezioneSpostamentoForzato`. Nessuna API nuova, nessuna firma allargata.

⚠️ **Il codice sapeva già che i casi erano tre, e li accomunava di proposito**: il commento al punto 3 di
`RisolviCadutaSeBordoAperto` dice *«Muro, unità e parapetto rispondono `false`, ed è la distinzione che
`StepUntilBlocked` da solo non può dare»*. Ciò che mancava non era la conoscenza — era un posto dove
scriverla.

🔴 **I valori nuovi si aggiungono in coda.** `ERTMoveOutcome` ed `ERTDisplacementBlockReason` viaggiano nel
TurnLog come **indice** e il formato è oggi `ERTTurnLogFormatVersion::WithSightBlocker` = **13**: estendere
in coda non è una migrazione, riordinare o cambiare esiti già prodotti sì, con rigenerazione del corpus
golden ([`D-245`](../decisions/RT_PDR_00_Decision_Log.md)).

> 🔴 **Questa riga diceva `WithMicroStep` = 12, e lo diceva da due giorni di troppo.** `WithSightBlocker` =
> **13** è entrato il **2026-09-06** con `0698594b` (#2534, *«il log nomina il muro che ferma il tiro»*).
> Corretto il 2026-09-08 in code review sulla PR **#2676** — che aveva **certificato** il 12 come *«ancora
> il massimo»* dopo averlo letto in un output troncato. La conclusione non cambia: estendere in coda resta
> non-migrazione, e un campo dedicato costerebbe **14**.

⚠️ **Questa sezione diceva *«distingue almeno»* e non diceva come.** Con un solo valore in codice, chi
implementava [#2403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2403) avrebbe scelto da
solo fra tre design incompatibili, su un formato di traccia che ha un owner. Deciso in
[`D-352`](../decisions/RT_PDR_00_Decision_Log.md).

---

## 10. Documenti correlati

| Documento | Rapporto |
|---|---|
| [`spec-tassonomia-movimento.md`](spec-tassonomia-movimento.md) | famiglie di movimento e divieto di auto-reroute |
| [`brief-stati-unbalanced-prone.md`](brief-stati-unbalanced-prone.md) | `D-319`, l'omonimo del §5.1 |
| [`spec-terreni-e8.md`](spec-terreni-e8.md) | `ERTHexSurface`, incluso `Void` |
| [`spec-ponti-cp94.md`](spec-ponti-cp94.md) | archi di transizione e stato |
| [`../roadmap/plans/verticalita-ledge-fall-dual-roadmap-2026-09-05.md`](../roadmap/plans/verticalita-ledge-fall-dual-roadmap-2026-09-05.md) | audit e piano di esecuzione |

---

## 11. Revisione del 2026-09-08 — spec panel su #2388

Referto: [`../roadmap/plans/2388-caduta-spec-panel-2026-09-08.md`](../roadmap/plans/2388-caduta-spec-panel-2026-09-08.md).
Misure su `origin/main` = `13ddc500`.

Tre correzioni al testo. **Nessuna cambia una regola di gioco**: due rendono falsificabile ciò che non lo
era, la terza decide una forma che mancava.

| § | Che cosa diceva | Perché è cambiato |
|---|---|---|
| §4.3 | modello di risoluzione *«consigliato»*, con eccezione discrezionale | non falsificabile → invariante osservabile ([`D-353`](../decisions/RT_PDR_00_Decision_Log.md)) |
| §5 | *«zero occorrenze in `Source/`»* | falsa su due simboli su tre: le occorrenze sono le note stesse |
| §9 | *«distingue almeno»*, senza dire in quale forma | un solo valore in codice → forma decisa ([`D-352`](../decisions/RT_PDR_00_Decision_Log.md)) |

⚠️ **Due lavori nascono da qui, e nessuno dei due è coperto oggi.**

1. `Fall.TwoFallersSameLandingIsDeterministic` — l'invariante §4.3.1 con **due** cadute nello stesso Blast.
   Appartiene a [#2402](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2402), che possiede la
   meccanica, e la sua mutazione al gate [#2406](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2406);
2. i tre valori in coda a `ERTMoveOutcome` del §9.1, che sono lavoro di
   [#2403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2403).

✅ **Ciò che il panel ha misurato e non ha toccato**: il vocabolario §2 — `FRTHexEdgeGuard` vive su
`Map/RTHexCellData.h:266` accanto a `Covers` e `Doors`, esattamente dove §2 argomenta che debba stare, e
**non** su `FRTHexEdge`; il tie-break §4.2, pinnato da `Fall.AlternativeFollowsCanonicalRingFromFacing`; e
l'asimmetria spinta/trazione, pinnata da `ForcedMovement.PullOverOpenLedgeStartsFall`.
