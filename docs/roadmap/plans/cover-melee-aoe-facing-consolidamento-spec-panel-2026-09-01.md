# Cover · Melee · AoE · Facing — referto di consolidamento del kit d'autore v0.1

> `CURRENT` · **Tipo**: referto di spec-panel · **Data**: 2026-09-01 · **Misurato su**: `origin/main` `90cdc903`
> **Owner**: nessuno — questo documento **non è** una sede normativa. Ogni regola citata appartiene all'owner
> indicato; se una riga di questo referto e il suo owner divergono, **vince l'owner**.
>
> **Mandato**: consumare `RefactorTactics_Cover_Facing_AoE_Consolidation_for_Claudia_v0.1.md` (40 sezioni,
> 1171 righe), archiviato in [`../../archive/`](../../archive/RefactorTactics_Cover_Facing_AoE_Consolidation_for_Claudia_v0.1.md).
>
> **Esito in una riga**: il kit **non è un consolidamento**, e lo stesso difetto si ripete due volte su due.
> Il principio fondativo (§1) è la separazione che [`CP 9.2`](../../gameplay/spec-copertura-alta-cp92.md) ha
> **esplicitamente scartato** con la ragione scritta; la §19 chiama «legacy da refactor» un ramo che
> [`CP 9.1`](../../gameplay/spec-copertura-cp91.md) §4 **dichiara come regola**, sempre con l'alternativa
> scartata per iscritto; e due default (§10/§26) ribaltano canone implementato e testato senza nominarlo.
>
> 🔁 **Corretto il 2026-09-01, secondo passaggio — la §4 di questo referto era sbagliata.** Diceva che il ramo
> `Area → cover 0` è un difetto che contraddice [`D-302`](../../decisions/RT_PDR_00_Decision_Log.md). Non lo
> è: ha un owner, tre sedi che concordano, un'alternativa scartata e un test guardiano non vacuo. La
> correzione è in §4, e la ragione per cui l'errore era possibile in §4.4.

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

## 4. La §19 dice «legacy», e non lo è — ma apre una domanda vera

**§19 del kit**: *«Il repository contiene già un comportamento legacy in cui `Area` è accoppiata a una hit
rule che ignora cover; questo va trattato come accoppiamento da refactor, non come principio di design.»*

### 4.1 La riga esiste

```cpp
// Un'area investe la cella da ogni lato: nessun bordo da attraversare, nessuna copertura che tenga.
if (Map == nullptr || Shape == ERTAbilityShape::Area)
{
	return 0;
}
```
> `URTHexCombatLibrary::HexCoverDamageReduction`, `Source/RefactorTactics/Combat/RTHexCombatLibrary.cpp:179`

### 4.2 Ma è una regola dichiarata, non un accoppiamento residuo

🔴 **E questa è la parte che la prima stesura di questo referto ha sbagliato.** La regola vive in **tre sedi
che concordano**, e l'alternativa che il kit propone è **scartata con la ragione scritta** — esattamente come
in §3:

| Sede | Cosa dice |
|---|---|
| [`spec-copertura-cp91.md`](../../gameplay/spec-copertura-cp91.md) §3 punto (1) | `Shape == Area` → **0**, normativo |
| [`spec-copertura-cp91.md`](../../gameplay/spec-copertura-cp91.md) §4, **fra le decisioni** | *«Le aree non sono mai ridotte»*; alternativa scartata: *«trattare l'area come un proiettile con un'origine avrebbe aggiunto una geometria in più per non cambiare nessun esito»* |
| [`RT_ActionCatalog_v0.1.md`](../../balance/RT_ActionCatalog_v0.1.md) | `Circular AoE` — *«la copertura non riduce»* |

✅ **Ed è protetta da un guardiano non vacuo.** `Cover.LowCover.AoESameSide` copre il caso ambiguo — centro
dal lato riparato — e il suo commento registra la misura di [#1529](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1529):
togliendo il gate `Shape == Area`, *«con il facing di default la suite `Cover` restava 21/21 verde; con il
bersaglio rivolto verso l'attaccante questo test cade»*. Il mutation test è già nella tabella di `CP 9.1`.

### 4.3 La domanda che resta è di governance, non di codice

[`D-302`](../../decisions/RT_PDR_00_Decision_Log.md) punto (3) elenca la direzione d'impatto per categoria e
vi include l'area — *«area = centro d'impatto→bersaglio»* — una frase che presuppone che all'area **si
applichi** una mitigazione direzionale.

⚠️ **`D-302` non nomina mai `CP 9.1`**: zero occorrenze di `CP 9.1`, `9.1` o `cp91` nel corpo della voce. E la
frase nasce chiudendo `COV-5`, cioè dentro il modello di copertura **selezionabile**, che non è implementato.

Per [`D-282`](../../decisions/RT_PDR_00_Decision_Log.md) la precedenza è **tipizzata**: il Decision Log
possiede le decisioni esplicite, le **specifiche possiedono la semantica delle regole**. «L'area riceve
mitigazione?» è semantica → l'owner è `CP 9.1`. E il punto (5) della stessa voce chiude la questione di
metodo:

> *«quando due autorità realmente applicabili producono esiti incompatibili, il conflitto si **registra e si
> escala** all'owner competente. Non si scioglie con una gerarchia globale automatica.»*

∴ **Registrato e escalato**, non risolto qui: [**#2009**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2009)
con tre uscite dichiarate, e `COV-12` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

⚠️ **Una precisazione tecnica che vale per l'uscita «l'area diventa mitigabile»**: non basta togliere il ramo.
`EffectiveCoverReduction` applica `IsInFrontalArc(Target.Cell, Target.Facing, **Attacker.Cell**)` — quindi
senza il ramo, il test di `CP 16.2` userebbe la cella dell'**attaccante** anche per le aree, cioè proprio ciò
che `D-302` vorrebbe evitare. Serve un cambio di firma, non la rimozione di una riga.

### 4.4 Perché l'errore era possibile, ed è il punto di questo referto

La prima stesura ha concluso «difetto, e il codice va allineato a `D-302`». Il `grep` aveva confermato la
riga; il kit diceva «legacy»; le due cose insieme sembravano una misura.

🔑 **Verificare che una riga esista non verifica che sia legacy.** L'owner della regola era a un `grep` di
distanza — `CP 9.1` §4 — e conteneva anche l'alternativa scartata, cioè la risposta all'obiezione del kit
scritta prima che l'obiezione arrivasse.

🔴 **Ed è lo stesso pattern di §3, sulla stessa fonte**: il kit propone come nuovo ciò che il repository ha già
scartato **con la ragione scritta**, due volte su due — `CP 9.2` per la separazione dei blocker, `CP 9.1` per
l'origine della cover d'area. Un referto che avesse confermato la §19 avrebbe portato dentro, con l'autorità
di una misura, la stessa proposta che aveva appena respinto in §3.

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
| ⚠️ **Conflitto fra due sedi — registrato, non risolto** | 2 | §14, §19 *(vedi §4: la riga esiste, ma è regola dichiarata da `CP 9.1`)* |
| 🌱 **Greenfield: nessun codice, nessun owner** | 5 | §3, §5, §6, §7, §33 |
| 🟡 **Aperto altrove, non deciso qui** | 3 | §15/§16 *(propagazione per-target)*, §31 *(`NonDirectional` = `FAC-13`)*, §38 |
| 📋 **Backlog, non specifica** | 3 | §8, §36, §2.4 |

> ⚠️ **§31 conta due volte** ed è voluto: tre dei suoi quattro valori sono chiusi da `D-302`, il quarto è
> `FAC-13` e resta aperto. È la stessa riga in due stati.

> 🔁 **Aggiunto il 2026-09-01, secondo passaggio — una terza volta, e la decisione è in volo.** `COV-7` è
> stata chiusa in giornata da una decisione che rivendica `D-308` in
> [PR #2016](https://github.com/DegrassiAaron/refactor-tactics-main/pull/2016), **non ancora mergiata**, e che
> fissa il vault: *«`Low Cover` ⛔ non automaticamente scavalcabile»*, con il vault che esiste **solo dove un
> autore l'ha disegnato** nel tactical graph. Il kit dice l'opposto in due punti — §2.1 (`Vaultable = true` di
> default sulla Low Cover) e §7 (*«Se il Dash incontra Low Cover: `CanVault == true`»*).
>
> ∴ Il conteggio *«23 già canone»* di questo referto **resta vero alla sua misura** (`90cdc903`), ma §2.1 e §7
> si spostano da «backlog» a «conflitto» appena `D-308` entra in `main`. ⚠️ **Non si aggiorna qui**: finché la
> PR è aperta quell'ID vive solo nel suo diff, e il primo `D-nnn` libero è **`D-312`** — #2016 ne rivendica
> quattro, da `D-308` a `D-311`.

## 8. Che cosa si è fatto

| Azione | Dove |
|---|---|
| Kit archiviato **integralmente** | [`docs/archive/RefactorTactics_Cover_Facing_AoE_Consolidation_for_Claudia_v0.1.md`](../../archive/RefactorTactics_Cover_Facing_AoE_Consolidation_for_Claudia_v0.1.md) — stessa convenzione del precedente `…Replay_System_Claude_Consolidation_2026-08-10.md` |
| Due domande d'autore aperte | `COV-11` e `FAC-16` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) |
| Un conflitto registrato ed escalato | [**#2009**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2009) + `COV-12` — `D-302` dà una direzione d'impatto all'area, `CP 9.1` §4 la esclude dalla mitigazione (§4.3) |

### Che cosa **non** si è fatto, e perché

| Non fatto | Ragione |
|---|---|
| Nessuna riga di `Source/` toccata | non c'era un difetto da correggere: il ramo `Area` è regola dichiarata, e cambiarlo sarebbe un cambio di **regola di gioco** con corpus golden da rigenerare |
| Nessuna `D-nnn` scritta | il kit propone `FREEZE` su clausole che confliggono con canone testato. Ratificarle in un referto è deciderle per inerzia — `D-305` §5(b) |
| Nessuno dei 24 scenari §36 aggiunto | cinque non compilano, sette duplicano test verdi, e i restanti non hanno Given/When/Then |
| Nessuna Epic né issue-ombrello | il perimetro ha owner vivi e distinti: [#1833](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1833), [#339](https://github.com/DegrassiAaron/refactor-tactics-main/issues/339), [#1828](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1828) |
| Nessun tocco alle percentuali | materia `BAL-*`, e `D-271` tiene chiuso il vocabolario |

## 9. Prossimo passo

[**#2009**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2009) — **scegliere fra le tre
uscite** che la voce dichiara: `D-302` è inerte per l'area, oppure supera `CP 9.1` §4, oppure le due valgono
su oggetti diversi (area ambientale ≠ area selezionabile) e servono due nomi.

⚠️ **È la sola delle tre domande aperte che non costa nulla se la risposta è (1)**: una nota di
delimitazione, zero righe di `Source/`. Ma è anche la sola in cui *non rispondere* lascia in piedi una riga
di decisione **senza consumatori** — e questo repository ha già pagato quattro volte il campo che nessuno
legge.
