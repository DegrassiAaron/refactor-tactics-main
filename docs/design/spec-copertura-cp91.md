# Copertura bassa direzionale — CP 9.1

> **Stato**: chiuso il 2026-08-07 · **Issue**: [#69](https://github.com/DegrassiAaron/refactor-tactics-main/issues/69) · **Epic**: E9 (#23)
> **Fonti**: `docs/design/roadmap-v0.1.md` §E9 · `piano-canonico-mvp.md` (invariante #4: ogni formato serializzato è versionato) · ADR-0005 (orientamento)
> **Codice**: `Map/RTHexCellData.h`, `Map/RTHexMapAsset.*`, `Combat/RTCombatLibrary.h`, `Combat/RTHexCombatLibrary.*`

## 1. Cosa fa

La copertura bassa è un riparo **su un bordo di cella**: toglie **10** al danno diretto che attraversa
quel bordo, e non fa nulla dagli altri cinque. Il dato entra nel formato mappa (v2 → **v3**) e nel calcolo
del Blast; nessun'altra parte del gioco cambia comportamento.

## 2. Modello dati

```
FRTHexCover { Edge: ERTHexDirection, Type: ERTHexCoverType, Integrity: int32 = 30 }
FRTHexCellData.Covers: TArray<FRTHexCover>   // 0..6 voci, al più una per bordo
```

**Array sparso, non sei slot fissi.** Una mappa reale è quasi tutta scoperta: con l'array sparso le celle
senza copertura non serializzano nulla, non entrano nell'hash con campi vuoti e restano confrontabili con le
mappe scritte prima di questo checkpoint. Sei slot fissi avrebbero aggiunto 6 voci a ognuna delle centinaia
di celle per rappresentare "niente".

**`ERTHexCoverType` ha oggi due valori** (`None`, `Low`). `High` arriverà con CP 9.2 **in coda all'enum**:
aggiungere un valore non è una migrazione di formato, mentre inventare oggi un valore che nessuna regola sa
applicare sarebbe stato un dato senza consumatore.

**L'integrità 30 sta nel dato**, come default della struct, non in una costante di `URTCombatLibrary`:
`RTHexCellData.h` è sotto `Map/` e non può dipendere da `Combat/` senza invertire il verso delle dipendenze.
La **riduzione di 10**, invece, sta in `URTCombatLibrary::LowCoverDamageReduction`, accanto a `Guard` 15,
`Deflect` 20 e `Brace` 10: è la stessa famiglia di numeri — quanto danno diretto si toglie — anche se la
condizione che la attiva è geometrica invece che di stato.

## 3. La regola

`URTHexCombatLibrary::HexCoverDamageReduction(Map, From, Target, Shape)`:

1. `Shape == Area` → **0** (vedi §4);
2. cella bersaglio assente o senza coperture → 0;
3. stessa cella assiale di chi attacca → 0 (nessun bordo attraversato);
4. altrimenti il bordo è quello dell'**ultimo passo della linea attaccante → bersaglio**, cioè lo stesso
   criterio con cui `HexKnockbackDestination` decide la direzione di una spinta. Se quel bordo ha una
   copertura `Low` → `LowCoverDamageReduction`, altrimenti 0.

L'applicazione avviene in `CollectHexAttacks`, **sul singolo colpo** e non sull'intento: due bersagli della
stessa azione possono essere riparati in modo diverso. Il danno si ferma a 0 con `FMath::Max`, e il colpo
resta **avvenuto** — è la disciplina di `Deflect`, dove il clamp è sul valore e non sulla voce, così trigger
e marchi continuano a contare.

## 4. Decisioni e alternative scartate

| Decisione | Alternativa scartata | Perché |
|---|---|---|
| **Le aree non sono mai ridotte** | Ridurre l'AoE quando il centro sta dal lato **opposto** alla copertura | La DoD nomina solo il caso «centro dallo stesso lato» perché è l'unico ambiguo: col centro dall'altro lato la copertura non è comunque interposta. Trattare l'area come un proiettile con un'origine avrebbe aggiunto una geometria in più per non cambiare nessun esito |
| Bordo = ultimo passo della linea | Confrontare gli angoli fra i centri-cella | La linea esiste già, è intera e deterministica; gli angoli avrebbero introdotto float in una decisione di gioco (invariante #4) |
| Array sparso `Covers` | Bitmask `uint8` a 6 bit | La bitmask non porta l'integrità, che CP 9.2 deve scalare per distruggere il riparo |
| `Integrity` default 30 nella struct | Costante in `URTCombatLibrary` | Dipendenza `Map/ → Combat/` invertita. Il valore è verificato da un test, così non deriva in silenzio |
| Migrazione in `PostLoad` | `FCustomVersion` registrata sull'archivio | v2 → v3 non converte dati: il campo nuovo nasce vuoto. Un custom version è l'infrastruttura giusta il giorno in cui una migrazione dovrà **trasformare** valori, non oggi (vedi limite §6.1) |
| `Action.CreateCover` **fuori** da CP 9.1 | Cablarla qui, ora che le coperture esistono | Il catalogo la dichiara con **durata 2 turni**; le coperture temporanee sono CP 9.5 e useranno l'infrastruttura del terreno dinamico di E8 (stato corrente sulla mappa, originale e durata nel `TurnManager`). Crearla adesso significherebbe crearla **permanente**, cioè un'altra regola. Il test che ne fissa l'assenza (`RTEnvironmentActionTests.cpp`) resta valido e non è stato toccato |

## 5. Il confine col facing (non unificare)

Due direzionalità **ortogonali**, come dichiara la roadmap E9 e ribadisce il commento di emendamento su #69:

- la **copertura** dipende da quale lato della **cella** attraversa il colpo. Il facing dell'unità non la
  ruota: girarsi non sposta un muretto;
- il **retro scoperto** (#177, CP 16.2) dipende da dove **guarda l'unità**.

L'unico punto di contatto sarà additivo: un colpo fuori dall'arco frontale **annulla** la riduzione da
copertura. Nessuna delle due si implementa dentro l'altra.

## 6. Limiti dichiarati

1. **Il default di `FormatVersion` è cambiato nel CDO** (2 → 3). Unreal non serializza le property uguali al
   default: un asset salvato quando il default era 2 non contiene affatto il campo, quindi al caricamento
   prende direttamente 3 e `MigrateToCurrentFormat` non ha nulla da fare. L'esito è identico **perché questa
   migrazione non converte dati**; per una futura migrazione che debba trasformare valori servirà una
   `FCustomVersion`, altrimenti il ramo di conversione non verrebbe eseguito.
2. **`DA_HexMap_Sandbox` è vuoto** (0 celle, 0 transizioni) — lo stesso difetto già annotato in
   `RTMatchSetupWorldTests.cpp`. La migrazione ci passa senza errori, ma su un asset vuoto non dimostra la
   preservazione: la prova sta in §7.
3. **Il bot non conosce le coperture.** `HexBot.ScoreThreatRespectsCover` usa "copertura" nel senso di linea
   di tiro bloccata; la riduzione di 10 non entra nella sua stima del danno. Cambiare le premesse del bot è
   CP 13.5, che le cambia comunque una seconda volta.
4. **Le reazioni che redirigono un colpo** (`Intercept`, `Bastion.Interposition`) agiscono **dopo** la
   raccolta: il colpo conserva la riduzione calcolata sul bersaglio originale, non su chi lo incassa davvero.
   Con la copertura non c'è oggi nessun caso in campo (nessuna azione la crea), ma quando CP 9.5 le renderà
   piazzabili il punto andrà deciso esplicitamente.
5. **Nessun'azione crea coperture**: si scrivono a mano nel pannello proprietà del data asset. Il pennello
   dell'editor mappa per le coperture non è richiesto da questo checkpoint.
6. **L'hash della mappa cambia** (`FormatVersion` vi entra): i checksum di replay registrati prima di questo
   commit non sono confrontabili con quelli dopo. È il caso previsto dal rischio «il golden hash cambia a ogni
   epic che atterra» (roadmap §E15).

## 7. Verifiche

### 7.1 Test automatici (7 nuovi, suite **394/394** verde)

| Test | Cosa fissa |
|---|---|
| `Cover.DirectionalDamageReduction` | −10 dal lato protetto, danno pieno nella stessa scena senza copertura |
| `Cover.LowCover.WrongSideNoReduction` | bordo opposto e due bordi **adiacenti** a quello attraversato: danno pieno. Anche la copertura sulla cella dell'attaccante non protegge |
| `Cover.LowCover.AoESameSide` | area centrata dal lato riparato: nessuna riduzione |
| `Cover.LowCover.NeverHealsTarget` | colpo da 4 contro copertura 10 → 0, e il colpo resta nel piano |
| `HexMap.FormatMigrationPreservesCells` | v2 → v3 non perde celle, altezze, superfici, costi, flag né transizioni; idempotente |
| `HexMap.CoverHashDeterminism` | bordo, tipo e integrità entrano nell'hash; l'ordine dell'array **no** |
| `HexMap.CoverValidation` | bordo doppio, integrità ≤ 0 e voce senza tipo sono errori |

### 7.2 Verifiche di mutazione

Ogni riga: implementazione rotta di proposito, ricompilata, suite completa rieseguita. Mutazioni che colpiscono
test diversi eseguite insieme; quelle sullo stesso test una per volta.

| # | Mutazione | Test caduto | Altri |
|---|---|---|---|
| 1 | rimosso il gate `Shape == Area` | `Cover.LowCover.AoESameSide` | nessuno |
| 2 | la riduzione vale per una copertura **qualsiasi** della cella, non per il bordo attraversato | `Cover.LowCover.WrongSideNoReduction` | nessuno |
| 3 | rimosso il clamp `FMath::Max(0, …)` | `Cover.LowCover.NeverHealsTarget` | nessuno |
| 4 | `MigrateToCurrentFormat` non alza la versione | `HexMap.FormatMigrationPreservesCells` | nessuno |
| 5 | le coperture non entrano in `ComputeHash` | `HexMap.CoverHashDeterminism` | nessuno |
| 6 | `ComputeHash` non ordina i bordi | `HexMap.CoverHashDeterminism` | nessuno |
| 7 | `ValidateMap` non rileva i bordi duplicati | `HexMap.CoverValidation` | nessuno |

`Cover.DirectionalDamageReduction` non ha una riga sua perché è caduto nel **RED**, prima che la regola
esistesse: `atteso 20, era 30`.

### 7.3 Migrazione del formato sull'asset reale

`DA_HexMap_Sandbox` **non è nel repository** (`Content/**/*.uasset` è ignorato salvo due eccezioni), quindi la
verifica non è ripetibile in CI e non ha senso come test versionato: in un clone pulito non avrebbe niente da
caricare. È stata eseguita **una volta, headless**, con una probe usa-e-getta, in due giri:

1. binario **pre-CP 9.1**: scrive un `.uasset` popolato (26 celle con altezze, superfici, costi e flag
   distinti, su due layer, più una transizione bidirezionale) → il file finisce su disco **in formato v2**;
2. binario **CP 9.1**: ricarica quel file.

| | celle | transizioni | versione | digest (soli campi v2) | errori |
|---|---|---|---|---|---|
| scritto (v2) | 26 | 2 | 2 | `4144344470` | — |
| riletto (v3) | 26 | 2 | **3** | `4144344470` | 0 |

Digest identico attraverso la serializzazione vera: la migrazione non perde nulla. Sull'asset di sandbox reale
la stessa probe ha misurato `cells=0 … version=2` prima e `cells=0 … version=3` dopo — migrato, ma vuoto in
partenza (limite §6.2).

Resta all'editor una sola cosa, registrata in [`test-manuali-pie.md`](test-manuali-pie.md) come
**PIE-V01-COVEREDIT**: la migrazione avviene in memoria a ogni caricamento finché l'asset non viene
**risalvato** dall'editor, e le coperture si disegnano solo dal pannello proprietà.

## 8. Cosa apre

- **CP 9.2**: `High` in coda a `ERTHexCoverType`, e `Integrity` che comincia a scendere davvero.
- **CP 9.5**: `Action.CreateCover`, `Bastion.KineticPanel` e `Gadget.PortableCover` sulle coperture
  temporanee — durata sul modello del terreno dinamico di E8.
- **CP 16.2**: l'arco frontale che **annulla** la riduzione, come regola additiva.
