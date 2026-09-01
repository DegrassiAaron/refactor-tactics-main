# Cover · Melee · AoE · Facing — referto di consolidamento del kit d'autore v0.1

> `CURRENT` · **Tipo**: referto di spec-panel · **Data**: 2026-09-01 · **Misurato su**: `origin/main` `90cdc903`
> **Owner**: nessuno — questo documento **non è** una sede normativa. Ogni regola citata appartiene all'owner
> indicato; se una riga di questo referto e il suo owner divergono, **vince l'owner**.
>
> **Mandato**: consumare `RefactorTactics_Cover_Facing_AoE_Consolidation_for_Claudia_v0.1.md` (40 sezioni,
> 1171 righe), archiviato in [`../../archive/`](../../archive/RefactorTactics_Cover_Facing_AoE_Consolidation_for_Claudia_v0.1.md).
>
> **Esito in una riga**: il kit **non è un consolidamento**. Il suo principio fondativo (§1) è la separazione
> che [`CP 9.2`](../../gameplay/spec-copertura-alta-cp92.md) ha **esplicitamente scartato** con la ragione
> scritta; due dei suoi default **ribaltano** canone implementato e testato; e la sola clausola che il
> repository conferma parola per parola — §19, il legacy `Area → cover 0` — è anche **l'unico difetto reale
> che il kit trova**, perché quel codice contraddice [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md)
> accettata il giorno prima.

## 1. Perché questo referto non ratifica niente

Il kit chiude dichiarando otto voci `PROPOSED TO FREEZE`. Questo referto **non ne congela nessuna**, e la
ragione è scritta dal kit stesso:

> §40.3 — *«Se esiste un conflitto, mantenere il conflitto esplicito e creare una decisione da approvare.»*

I conflitti ci sono, sono tre, e due sono contro codice con test verdi. Congelarli qui li deciderebbe per
inerzia — il difetto che [`D-305`](../../decisions/RT_PDR_00_Decision_Log.md) §5(b) ha appena scartato per
nome sullo stesso perimetro.

⚠️ **Il precedente è di dodici ore fa e va letto**: il mandato d'autore del 2026-08-31 aveva ventuno clausole
e **undici erano già canone**. Qui il rapporto è peggiore, non migliore: su 40 sezioni, **23 sono già canone**
e sei di quelle il kit le riscrive come se fossero nuove.

## 2. Misura

| Voce | Valore |
|---|---|
| Ref misurato | `origin/main` `90cdc903` (checkout allineato, `0 ahead / 0 behind`) |
| Metodo | `git grep` su `Source/` per **simbolo**, non per prosa; lettura delle firme pubbliche e degli owner |
| ⚠️ Precondizione | il checkout era **46 commit indietro** all'apertura della sessione. Misurare lì avrebbe prodotto un referto vero di uno stato che nessuno sta per mergiare |
| Non misurato | le §20–§30 (Facing) sono verificate contro [`cover-facing-consolidamento-spec-panel-2026-08-31.md`](cover-facing-consolidamento-spec-panel-2026-08-31.md) e `OPEN_DECISIONS`, **non** ri-misurate riga per riga nel codice — con l'eccezione di §9/§10/§26, misurate direttamente perché confliggono |

### 2.1 Il vocabolario del kit non è il vocabolario del repository

`git grep -l` su `Source/`, per simbolo:

| Simbolo del kit | File in `Source/` |
|---|---|
| `BlocksLOS` · `BlocksProjectiles` · `BlocksMeleeContact` · `Vaultable` · `CoverArc` · `CoverLevel` · `MeleeReach` · `HasValidMeleeContact` | **0** ciascuno |
| `BlocksTraversal` | **15** |

Non è una questione di nomi. `BlocksTraversal` è **uno** e i simboli del kit sono **cinque**, ed è la
differenza fra i due modelli — §3 qui sotto.

## 3. Il conflitto centrale: §1 propone ciò che CP 9.2 ha scartato per iscritto

**Il kit §1** («Core Principle») chiede che `CoverLevel` non deduca `BlocksLOS`, `BlocksProjectile`,
`BlocksTraversal`, `BlocksMelee`, e che le cinque domande si valutino **separatamente**.

**Il repository** ha un solo predicato, e il commento che lo introduce dichiara il perché:

```cpp
// L'OR e' RESTRITTIVO: se un bordo porta sia un muro alto sia una porta, aprire la porta non buca il muro.
// […] Questa e' l'UNICA funzione che vista, grafo e combat interrogano.
bool URTHexCoverLibrary::BlocksTraversal(const URTHexMapAsset* Map, const FRTCellId& From, const FRTCellId& To)
{
	return CoverBetween(Map, From, To) == ERTHexCoverType::High
		|| URTHexDoorLibrary::BlocksBetween(Map, From, To);
}
```
> `Source/RefactorTactics/Map/RTHexCoverLibrary.cpp:61`

E [`CP 9.2`](../../gameplay/spec-copertura-alta-cp92.md) riga 29–31 scrive l'argomento contro la
separazione, prima ancora che qualcuno la proponesse:

> *«la vista (`URTHexVisionLibrary`), il grafo di traversata (`URTHexPathLibrary`) e il combat […] una
> copertura che togliesse la vista ma non il passo sarebbe un **difetto invisibile** finché qualcuno non ci
> cammina attraverso.»*

🔑 **Le due posizioni non si sono incontrate**: il kit propone come principio fondativo l'ortogonalità che
CP 9.2 ha rifiutato **nominando il modo in cui fallisce**. Il kit non cita CP 9.2 e non risponde a
quell'argomento; CP 9.2 non contempla i preset del kit.

⛔ **Questo referto non sceglie fra i due.** È una decisione d'autore ed è aperta come `COV-11`.

### 3.1 E i preset sono quattro dove i valori canonici sono due

| | Kit §2 | Repository |
|---|---|---|
| Valori | `Low` · `High` · `Wall` · `No-Cross` | `ERTHexCoverType { None, Low, High }` |
| Chi lo chiude | — | [`D-271`](../../decisions/RT_PDR_00_Decision_Log.md): *«nessun livello Medium, nessuna scala continua»* |
| Segmento | quattro preset | `FRTGeometrySegment::WallType ∈ {Low, High}` |

E `RTGeometryGrammar.h:172` vieta esplicitamente la mossa che i preset richiederebbero:

> *«Riusa `ERTHexCoverType` invece di introdurre un `ERTWallKind` parallelo […] Un secondo enum con gli
> stessi due valori sarebbe la "seconda rappresentazione dello stesso oggetto" che il corpo di `#621` vieta.»*

⚠️ **`Wall` non è un preset mancante: è `High`.** Il kit scrive `Wall != High Cover` (§2.3) e definisce
`High` come *non* bloccante — ma nel repository `High` **è** il muro pieno (`nega vista, passo e proiettili`,
integrità 50, abbattibile). Il kit e il repository usano la stessa parola per due oggetti diversi, e questo
è il modo in cui la §2.3 diventa pericolosa: applicata alla lettera **declassa** ogni muro alto esistente.

## 4. Il difetto reale — ed è uno solo, ma è vero

**§19 del kit**: *«Il repository contiene già un comportamento legacy in cui `Area` è accoppiata a una hit
rule che ignora cover.»*

✅ **Vero, misurato, e alla riga esatta**:

```cpp
// Un'area investe la cella da ogni lato: nessun bordo da attraversare, nessuna copertura che tenga.
if (Map == nullptr || Shape == ERTAbilityShape::Area)
{
	return 0;
}
```
> `URTHexCombatLibrary::HexCoverDamageReduction`, `Source/RefactorTactics/Combat/RTHexCombatLibrary.cpp:179`

🔴 **E contraddice una decisione accettata il giorno prima.** [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md)
punto (3), chiudendo `COV-5`, ha deciso la direzione d'impatto per categoria:

> *«diretto/mischia = **sorgente→bersaglio**; linea/traiettoria = **direzione d'impatto**; area = **centro
> d'impatto→bersaglio**.»*

Un'area che azzera la copertura **non ha** una direzione d'impatto: la riga di codice non è una diversa
implementazione della regola, è la sua assenza. E la firma lo mostra — `HexCoverDamageReduction(Map, From,
Target, Shape)` riceve la cella dell'**attaccante**, non il centro d'impatto, quindi non potrebbe applicare
`D-302` nemmeno togliendo il ramo `Area`.

| | |
|---|---|
| **Chi ha ragione** | `D-302`. Il codice è precedente alla decisione e non è stato allineato |
| **Perché non si è corretto qui** | è una modifica di `Source/` che cambia **esiti serializzati** (`Power` nel TurnLog di ogni intento `Area`), quindi rigenera il corpus golden. Non è lavoro da referto |
| **Dove va** | [**#2009**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2009) — issue nuova, unica del pass. `SEARCH → REUSE` fatto: delle dodici issue aperte su copertura/area nessuna la copre, e [#1392](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1392) è la più vicina ma riguarda la **traccia** del colpo di boundary, non la regola |

🔑 **Il kit ha trovato questo perché guardava il codice e non i documenti.** È il suo contributo netto, e da
solo giustifica l'archiviazione del pacchetto.

## 5. I due default che ribaltano canone implementato

### 5.1 §10 / §26 — «forced movement → Facing unchanged» è **falso** oggi, ed è falso per decisione

Il kit propone `unchanged` come default e `FaceSource` come opt-in esplicito. Il codice fa l'opposto:

```cpp
ERTHexDirection URTFacingLibrary::FacingAfterDisplacement(const FRTCellId& LandedCell,
	const FRTCellId& SourceCell, ERTDisplacementCause Cause, ERTHexDirection Current)
{
	if (Cause != ERTDisplacementCause::Forced) { return Current; }          // ← §9 del kit: già canone ✅
	ERTHexDirection Towards = Current;
	if (URTHexLibrary::DirectionTowards(LandedCell, SourceCell, Towards)) { return Towards; }  // ← FaceSource
	return Current;
}
```
> `Source/RefactorTactics/Turn/RTFacingLibrary.cpp` · owner [`ADR-0008`](../../decisions/adr-0008-rotazione-e-policy-di-facing.md) §3 · test `RTFacingTests`

⚖️ **Il kit ha ragione su metà, e la metà giusta è già vera**: `Teleport → Facing unchanged` (§9) è
esattamente il ramo `Cause != Forced`. È il **forced movement** che diverge, e solo quello.

⚠️ Applicare §10 alla lettera invertirebbe un default con test verdi **senza che il kit sappia di farlo**:
non nomina `ADR-0008`, non nomina `ERTDisplacementCause`. È `FAC-16`.

### 5.2 §11 — non propone numeri diversi, propone un **modello** diverso

| | Kit §11 | Repository |
|---|---|---|
| Forma | moltiplicatore: `None 100% · Low 75% · High 50%` | sottrazione: `LowCoverDamageReduction = 10` |
| Livelli che mitigano | due | **uno** — `High` non mitiga perché **blocca**, e non arriva colpo |
| Dove vive | — | `Source/RefactorTactics/Combat/RTCombatLibrary.h:157` |

⚠️ Il kit dichiara i numeri «placeholder di balance» (§38.1) ed è corretto — ma la **forma** non è un
placeholder. `75%` e `50%` presuppongono che `High` lasci passare metà del danno, cioè presuppongono §2.2,
cioè presuppongono §1. I tre stanno o cadono insieme, ed è la ragione per cui `COV-11` è **una** domanda e
non tre.

⛔ Le percentuali restano materia `BAL-*` e fuori da qui, come [`D-305`](../../decisions/RT_PDR_00_Decision_Log.md)
§6 ha già stabilito per lo stesso perimetro.

## 6. Panel

Sei letture indipendenti dello stesso documento, in modalità `critique`. Sono giudizi sul **documento**, non
sul merito delle scelte di design — quello è dell'autore.

### 📊 WIEGERS — qualità dei requisiti

🔴 **CRITICO — le §2.x sono presentate come default e sono ambigue sull'agente.** *«`Vaultable = true`, se la
unit/abilità possiede traversal compatibile»* mette una condizione dentro un valore di default. Un default è
un valore; questo è una regola travestita.
📝 Separare `Vaultable` (proprietà del segmento: *ammette* il vault) da `CanVault` (proprietà dell'attore:
*sa* farlo). §8 lo sa già — *«la traversal speciale appartiene all'abilità»* — ma §2.1 lo contraddice.
🎯 **Alta** — è l'ambiguità che porta la condizione nel dato di mappa.

⚠️ **MAGGIORE — otto voci `PROPOSED TO FREEZE` senza criterio di uscita.** §38 elenca cosa non è deciso; lo
`Status` finale elenca cosa si propone di congelare. Nessuna delle due dice **chi approva** né **contro cosa
si verifica**. Il repository ha entrambe le sedi (`OPEN_DECISIONS.md`, Decision Log) e il kit non le nomina.

### 🔨 ADZIC — specificabilità per esempi

✅ **Il documento è forte dove diventa concreto.** §16 (`Blast Shadow`) è l'unico punto con un diagramma, ed è
anche l'unico requisito del kit che si potrebbe scrivere come test senza chiedere altro all'autore.

⚠️ **MAGGIORE — le §36 sono 24 nomi senza scenari.** `Combat.Melee.SameCellBlockedByInternalWall` è un buon
nome; ma nessuna riga dice *quale* mappa, *quale* segmento, *quale* esito atteso. Il repository chiede
Given/When/Then in `Scenarios/`, e senza quelli i 24 nomi sono un backlog, non una specifica.
📝 Dei 24, **cinque descrivono comportamento che oggi non compila** (nessun `MeleeContact` esiste) e **sette**
duplicano test verdi esistenti sotto altro nome — vedi §7.

### 🏗️ FOWLER — confini e interfacce

🔴 **CRITICO — §37 disegna nove servizi dove il repository ne ha già sei, e li chiama diversamente.** La
tabella *Implementation Boundary* è quasi giusta: `Map/Grid`, `LOS Service`, `Trajectory/Propagation`,
`Turn Resolver` **esistono** come `URTHexCoverLibrary`, `URTHexVisionLibrary`, `URTHexPathLibrary`,
`RTHexSimLibrary`. Ma il kit li presenta come da costituire.
📝 Riscrivere §37 come **mappatura** sui simboli esistenti, non come architettura target. Nella forma
attuale un lettore ne dedurrebbe che il confine va creato, e lo creerebbe accanto a quello che c'è.

💡 **E il vero disaccordo architetturale è uno solo**: `BlocksTraversal` è un predicato *composto* letto da
tre consumatori. Il kit vuole cinque predicati *atomici*. Questo è il trade-off, ed è legittimo — ma va posto
come tale, non come igiene.

### ⚙️ NYGARD — modi di fallimento

✅ **§35 è la sezione migliore del kit.** I reason code espliciti (`BlockedByWall` ≠ `Damage × 0%`) sono
esattamente la distinzione che rende un log diagnosticabile, e il repository la condivide già:
`ERTMoveOutcome`, `ERTFacingOutcome` (11 valori), `FRTCoverDamageResult`.

🔴 **CRITICO — il kit non dice cosa succede quando le cinque domande di §1 si contraddicono.** Un segmento con
`BlocksLOS = true, BlocksProjectiles = false` è dato coerente o incoerente? Il repository ha
`URTHexMapAsset::ValidateMap` che **rifiuta** dati incoerenti (`Integrity <= 0`, cover `None` in tabella): con
cinque flag indipendenti lo spazio dei dati incoerenti si moltiplica, e §1 non nomina nessun validatore.
📝 Ogni flag nuovo deve arrivare con la sua riga in `ValidateMap`, o l'ortogonalità è un generatore di mappe
rotte.

### 🧭 COCKBURN — attore e scopo

⚠️ **MAGGIORE — il documento non ha un attore primario.** È scritto per un consumatore («for Claudia»),
non per un giocatore o un designer. §40 lo conferma: dieci istruzioni di processo, zero obiettivi di
gioco. La conseguenza è misurabile — nessuna delle 40 sezioni dice **quale problema di partita** risolve la
separazione di §1, il che rende impossibile giudicarne il costo.

💡 Il caso d'uso che regge da solo è §4 (muro interno che blocca la melee nella stessa cella): è concreto,
è un paradosso vero, e ha già una sede — [`spec-cover-placement-intra-hex.md`](../../technical/systems/spec-cover-placement-intra-hex.md).

### 🔍 CRISPIN — validabilità

🔴 **CRITICO — cinque scenari di §36 non sono scrivibili oggi**, e non per pigrizia: `Melee` non esiste come
categoria nel resolver. `git grep -il "melee\|mischia" -- Source/` dà **10 file**, e sono catalogo d'arma,
bot e test — **zero** in `Combat/`. Un test che asserisse `Combat.Melee.IgnoresLowCover` asserirebbe su una
regola che nessuno ha scritto.
📝 §5, §6 e §33 non sono consolidamento: sono **greenfield**, e vanno detti tali. Il kit li presenta con lo
stesso peso di §20 (Facing), che è implementato e ha 13 test.

✅ **La buona notizia**: `Combat.BlockedByWall` esiste già e copre `Combat.Cover.Wall.BlocksProjectile`.

## 7. Le 40 sezioni, per stato

| Stato | N | Sezioni |
|---|---|---|
| ✅ **Già canone** — il kit conferma, non aggiunge | 23 | §9, §12, §13, §17, §18, §20, §21, §22, §23, §24, §25, §27, §28, §29, §30, §31 *(3 valori su 4, `D-302`)*, §32, §34, §35, §37 *(come mappatura)*, §39, §40, §4 *(parziale: `InteriorWalls` esiste)* |
| 🔴 **Conflitto con canone implementato** | 4 | §1, §2 *(tutti i preset)*, §10/§26, §11 *(la forma, non i numeri)* |
| ⚠️ **Vero e non risolto — difetto misurato** | 2 | §14, §19 |
| 🌱 **Greenfield: nessun codice, nessun owner** | 5 | §3, §5, §6, §7, §33 |
| 🟡 **Aperto altrove, non deciso qui** | 3 | §15/§16 *(propagazione per-target)*, §31 *(`NonDirectional` = `FAC-13`)*, §38 |
| 📋 **Backlog, non specifica** | 3 | §8, §36, §2.4 |

> ⚠️ **§31 conta due volte** ed è voluto: tre dei suoi quattro valori sono chiusi da `D-302`, il quarto è
> `FAC-13` e resta aperto. È la stessa riga in due stati.

## 8. Che cosa si è fatto

| Azione | Dove |
|---|---|
| Kit archiviato **integralmente** | [`docs/archive/RefactorTactics_Cover_Facing_AoE_Consolidation_for_Claudia_v0.1.md`](../../archive/RefactorTactics_Cover_Facing_AoE_Consolidation_for_Claudia_v0.1.md) — stessa convenzione del precedente `…Replay_System_Claude_Consolidation_2026-08-10.md` |
| Due domande d'autore aperte | `COV-11` e `FAC-16` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| Una issue per il difetto misurato | [**#2009**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2009) — l'allineamento di `HexCoverDamageReduction` a `D-302` (§4) |

### Che cosa **non** si è fatto, e perché

| Non fatto | Ragione |
|---|---|
| Nessuna riga di `Source/` toccata | il solo difetto trovato cambia esiti serializzati e rigenera il corpus golden |
| Nessuna `D-nnn` scritta | il kit propone `FREEZE` su clausole che confliggono con canone testato. Ratificarle in un referto è deciderle per inerzia — `D-305` §5(b) |
| Nessuno dei 24 scenari §36 aggiunto | cinque non compilano, sette duplicano test verdi, e i restanti non hanno Given/When/Then |
| Nessuna Epic né issue-ombrello | il perimetro ha owner vivi e distinti: [#1833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1833), [#339](https://github.com/DegrassiAaron/refactor-tactics-main/issues/339), [#1828](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1828) |
| Nessun tocco alle percentuali | materia `BAL-*`, e `D-271` tiene chiuso il vocabolario |

## 9. Prossimo passo

[**#2009**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2009) — **l'allineamento di `HexCoverDamageReduction` a `D-302`** — è l'unica azione di questo pass che non richiede una
decisione d'autore: la decisione **c'è già** ed è del 2026-08-31. Finché il ramo `Shape == Area` resta,
ogni granata del gioco ignora la copertura contro una regola accettata.

⛔ **E non si può fare per intero prima di `COV-11`** se si vuole applicare §14 alla lettera — perché il
centro d'impatto non è un parametro che la firma riceve. Ma **si può fare subito** nella forma minima:
togliere il ramo speciale e trattare `Area` come le altre shape, cioè `sorgente→bersaglio`. È meno di
`D-302`, ma è strettamente più vicino di `return 0`.
