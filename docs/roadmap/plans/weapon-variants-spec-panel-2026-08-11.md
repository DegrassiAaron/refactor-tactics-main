# Spec Panel — Varianti d'arma, affinità e fasce di danno

> `CURRENT` · **Creato**: 2026-08-11 · **Owner** di **una sola domanda**: la specifica delle varianti
> d'arma — atterrata lo stesso giorno con [D-085](../../decisions/RT_PDR_00_Decision_Log.md)…
> [D-089](../../decisions/RT_PDR_00_Decision_Log.md) — regge alla revisione, e che cosa resta scoperto
> fra ciò che i documenti promettono e ciò che il codice fa.
>
> Non è una specifica e non decide: è la **revisione** che segue il consolidamento. Gli owner restano il
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md),
> [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md) e il
> [catalogo equipaggiamento](../../balance/RT_EquipmentCatalog_v0.1.md).

## 1. Verdetto

Il sorgente `RefactorTactics_WeaponVariants_Claude_Consolidation.md` **era già stato consumato**, lo
stesso giorno, e archiviato in
[`docs/archive/src/`](../../archive/src/RefactorTactics_WeaponVariants_Claude_Consolidation.md). La copia
ricomparsa in `docs/src/` è il **medesimo sorgente senza banner**: corpo byte-identico
(`git hash-object` → `5a03eb3` su entrambi i lati), unica differenza le 23 righe di ritiro in testa.
Non c'era nulla da consolidare una seconda volta.

Questo panel guarda quindi **ciò che è atterrato**, non ciò che il sorgente proponeva. Il consolidamento
regge: le quattro `Locked` sono decisioni distinte e motivate, le `Provisional`/`Open` sono tracciate
come tali, e il difetto che D-085 ha scoperto nel resolver è stato corretto con un test scritto **prima**
e visto fallire. Tre rilievi restano, e il primo è quello che conta.

| # | Rilievo | Dove | Priorità |
|---|---|---|---|
| 2.1 | Applicata una variante, il ponte verso i campi legacy che il **bot** legge esiste solo in un helper di test | `RTCatalogLibrary.cpp` · `RTEquipmentTests.cpp` | 🔴 **alta** |
| 2.2 | Il catalogo owner **si contraddice** sul default per eroe: §1 lo dichiara non deciso, §4 lo assegna | `RT_EquipmentCatalog_v0.1.md` | 🟡 media |
| 2.3 | `D-089` è l'unica decisione concreta senza marcatore di stato d'implementazione | `RT_PDR_00_Decision_Log.md` | 🟢 bassa |

## 2. Panel — modalità critique

Esperti convocati sul dominio: **Fowler** (confini e duplicazione del contratto), **Nygard** (modi di
fallimento silenziosi), **Wiegers** (contraddizioni fra requisiti), **Crispin** (che cosa l'oracolo
copre davvero), **Adzic** (esempio eseguibile). Ogni rilievo porta l'evidenza misurata sul branch.

### 2.1 🔴 ALTA — applicare una variante è un contratto scritto **due volte a metà**

**FOWLER**: «`ApplyWeaponVariant` prende un `FRTActionDef` e restituisce un `FRTActionDef`. È una firma
onesta e pura — ma è **metà** dell'operazione che il gioco richiede, e la metà mancante vive oggi in un
file di test.»

`URTActionData` porta campi *legacy specchiati* (`RangeCells`, `CooldownTurns`, `Power`, `Shape`) —
la duplicazione è dichiarata in `RTHeroCatalogLibrary.cpp:13`, che li descrive come «quelli che
`ARTTurnManager` legge». `ApplyWeaponVariant` non può toccarli: vede solo la `Def`.

La produzione **sa** specchiare — `MakeHeroAction` (`RTHeroCatalogLibrary.cpp:35-36`) e
`MakeEquipmentAction` (`RTCatalogLibrary.cpp:545-546`) lo fanno — ma sempre **alla costruzione, da una
`Def` grezza**. Nessuna di quelle strade passa da una `Def` già modificata da una variante: quella è
un'operazione *successiva*, e l'unico codice che la completa sono **due righe dentro un helper di test**:

```cpp
// Source/RefactorTactics/Tests/RTEquipmentTests.cpp:429-431
Basic->Def = URTCatalogLibrary::ApplyWeaponVariant(Basic->Def, Variant);
Basic->RangeCells   = Basic->Def.RangeCells;    // <-- il ponte
Basic->CooldownTurns = Basic->Def.CooldownTurns; // <-- esiste solo qui
```

**NYGARD**: «Il modo di fallimento è silenzioso e cade sul bot, cioè sull'avversario di *ogni* partita
della v0.1.» Le due letture divergono:

| Chi legge | Campo | Dove |
|---|---|---|
| **Bot**, generazione candidati d'attacco | `Ability->RangeCells`, `Ability->Power` *(legacy)* | `PlanBots` — `RTTurnManager.cpp:406`, `:460` |
| **Resolver**, costruzione dell'**intento** | `Ability->Shape`, `Ability->RangeCells`, `Ability->AreaRadius` *(legacy)* | `RTTurnManager.cpp:2703-2705` |
| **Resolver**, danno di fallback | `Ability->Power` *(legacy)* | `RTTurnManager.cpp:2723` |
| **Resolver**, validazione in esecuzione | `Instance.Def.RangeCells` | `URTActionFallbackLibrary::ValidateInstance` (`RTActionFallbackLibrary.cpp:46-50`), chiamata da `RTTurnManager.cpp:2623` |

E la divergenza **non** viene richiusa dal ponte che il resolver ha già: `RTTurnManager.cpp:2604` ricade
sul campo legacy soltanto se `ActionId` è vuoto **o** `RangeCells <= 0`. Un attacco base modificato da una
variante non soddisfa nessuna delle due, quindi valida sulla `Def` giusta — mentre il bot ha pianificato
sulla vecchia.

> ⚠️ **Le prime due righe sono un'aggiunta del 2026-08-11, e correggono per difetto questo stesso panel.**
> La prima stesura contrapponeva soltanto *bot* e *validazione*, come se la superficie fosse la portata del
> bot. Non lo è: l'**intento** — ciò che finisce nel TurnLog e nel replay — nasce anch'esso dagli specchi,
> e con esso `Shape` e `AreaRadius`, che nessuna variante odierna tocca ma che la prima variante di forma
> toccherebbe. Trovato da chi ha implementato la correzione (`fix/variant-bridge`) **rileggendo il
> resolver invece di fidarsi di questo documento**, che è esattamente il modo in cui andava usato.

Scenario concreto, se `#63` (`E7.4` — Loadout, **OPEN**) cablerà il loadout assegnando solo `->Def`:

- **`Weapon.Precision`** (+1 portata): il bot pianifica alla portata **vecchia, più corta** → non usa mai
  la cella che ha pagato 4 danni per ottenere. Il vantaggio della variante è invisibile a chi la porta.
- **`Weapon.Impact`** (−1 portata): il bot pianifica alla portata **vecchia, più lunga** → genera un
  candidato che il resolver poi **rifiuta** con `«fuori portata»` (`ERTActionInvalidReason::OutOfRange` →
  `RTTurnLogLibrary.cpp:138`). È un attacco pianificato che
  fallisce in silenzio — cioè il *pulsante finto* che il catalogo, in `ApplyWeaponVariant:495-497`,
  dichiara esplicitamente di voler evitare.
- **`Weapon.Overcharge`**: il gate `CanUseAbility` (`:405`) legge `AbilityCooldowns`, alimentato dal
  legacy `Ability->CooldownTurns` (`RTUnit.cpp:491`). Il costo della variante non verrebbe mai applicato
  — ed è il caso che [D-090](../../decisions/RT_PDR_00_Decision_Log.md) ha appena reso **più grave**, non
  meno: chiudendo `WV-1` ha portato il costo a `CooldownDeltaTurns = +2` e il bonus a `+18/+14/+8` per
  fascia. Senza lo specchio, `Def.CooldownTurns` vale 2 mentre il legacy resta **0**: nessuna ricarica, e
  fino a **+18 danni gratis** su un attacco `High` invece dei +6 di prima.

**CRISPIN**: «E nessun test diventerebbe rosso. L'helper *fa* la cosa giusta, quindi ogni test verde
continua a esserlo — sta verificando un ponte che la produzione non attraverserà.»

**Raccomandazione**: promuovere il ponte da helper di test a funzione di produzione, p.es.
`URTCatalogLibrary::EquipWeaponVariant(URTActionData*, const URTEquipmentData*)`, e far chiamare a
`RTEquipmentTests.cpp:429` **quella**. Un solo posto sa dello specchio; `#63` non deve riscoprirlo. Il
costo è di poche righe **oggi**, contro un difetto che a `#63` si presenterà come «la variante non fa
niente» senza un solo test rosso a indicare dove guardare.

> ⚠️ Non è un difetto del codice spedito: **nessun percorso di produzione equipaggia oggi una variante**
> — `MakeWeaponVariants` e `DefaultWeaponVariantFor` non hanno chiamanti fuori dai test, e `#63` è
> aperta. È un rilievo **prospettico** sul lavoro che quella issue sta per fare.

### 2.2 🟡 MEDIA — il catalogo owner si contraddice sul default per eroe

**WIEGERS**: «Due affermazioni incompatibili nello stesso documento owner, a settanta righe di distanza.
Chi legge in ordine si ferma alla prima.»

- `RT_EquipmentCatalog_v0.1.md:52` — «Il **default per eroe** non è deciso (`WV-3`): la tabella §4 qui
  sotto assegna gadget e reazioni, **mai varianti d'arma**.»
- `RT_EquipmentCatalog_v0.1.md:121-126` — §4 ha una colonna **«Variante d'arma** *(D-089)*», che assegna
  Gadget→Precisione, Phase→Impatto, Riktor→Impatto, Wraith→Soppressione.

La nota §1 è **residuo del consolidamento**: D-089 ha chiuso `WV-3` e aggiunto la colonna, ma la frase
che dichiarava la domanda aperta è rimasta. `OPEN_DECISIONS.md:344` è invece corretto (`WV-3` barrata,
«✅ Chiusa il 2026-08-11»).

**Raccomandazione**: riscrivere la nota §1 perché rimandi a D-089 invece di negarlo. È l'unico punto del
consolidamento in cui un owner afferma il falso.

### 2.3 🟢 BASSA — `D-089` non dichiara il proprio stato d'implementazione

**ADZIC**: «Le sue quattro sorelle lo fanno; leggendole in fila, il silenzio della quinta si legge come
"fatta".»

| Decisione | Marcatore |
|---|---|
| D-085 | «**implementata**» |
| D-086 | *(nessuno — è un principio di design, accettabile)* |
| D-087 | «**numeri provvisori**, non implementata» |
| D-088 | «**non ancora implementata**» |
| **D-089** | **— assente —** |

Lo stato reale non è nessuno dei due estremi: `DefaultWeaponVariantFor` **esiste ed è pinnata** da
`Equipment.DefaultVariantPerHero` (cinque asserzioni, incluso che nessun default usi `Overcharge`), ma
**non ha chiamanti di produzione**: i default non sono assegnati a nessuna unità in partita.

**Raccomandazione**: aggiungere «*implementata come funzione, nessun consumatore in partita —* `#63`».
Costa una frase e toglie l'unica lettura ottimistica possibile.

## 3. Cosa il panel **non** rileva

Per equità verso il consolidamento, tre cose che sarebbe stato facile sbagliare e che sono giuste:

- **D-085 è verificata sul percorso reale.** `Equipment.PushTwoSeparatesGuardFromBrace` costruisce mondo,
  mappa, unità e `ARTTurnManager`, e fa girare un turno vero: non è un test in memoria sui dati. E ha il
  **gemello di controllo** (`Brace` regge dove `Guard` cede), senza il quale proverebbe solo che
  «qualcosa si è mosso».
- **La ricaduta di D-085 su D-074 è stata scritta, non nascosta.** La premessa «ogni spinta vale 1» è
  caduta, e D-074 lo dichiara nel proprio corpo, riaprendo esplicitamente l'opzione che aveva precluso in
  `BAL-1`/[#403](https://github.com/DegrassiAaron/refactor-tactics-main/issues/403). Una decisione che
  invalida la motivazione di una precedente è il caso in cui è più facile lasciare il documento vecchio
  a mentire.
- **Le §18–§29 del sorgente non sono state applicate, ed è corretto.** Prescrivevano dieci issue e
  un'epic nuova sopra una fotografia del repository più arretrata del repository stesso: E7 esisteva già,
  `E7.1` era in `main`, il validator già imponeva lo svantaggio. Un pacchetto di consolidamento si
  filtra, non si applica.

## 4. Raccomandazioni, in ordine

| # | Azione | Costo | Stato |
|---|---|---|---|
| 1 | `EquipWeaponVariant` di produzione; il test chiama quella | poche righe | 🔄 **scritta** su `fix/variant-bridge` — ⚠️ **non compilata né eseguita** (Live Coding di un'altra sessione). È il gate prima del merge |
| 2 | Riscrivere la nota §1 del catalogo perché citi D-089 | una frase | ⏳ aperta — un owner che afferma il falso |
| 3 | Marcatore di stato su D-089 | una frase | ⏳ aperta — allinea la quinta decisione alle sorelle |

Nessuna delle tre tocca un numero di bilanciamento, e nessuna riapre una decisione `Locked`.

> La **1** non è più solo una raccomandazione: `fix/variant-bridge` porta `EquipWeaponVariant` in
> produzione, fa chiamare quella all'helper di test invece di duplicarla, e aggiunge
> `Equipment.VariantReachesWhatTheGameActuallyReads` — che verifica `Def` e specchi allineati, la ricarica
> di `Overcharge` che arriva allo specchio, e che riequipaggiare due volte non accumuli `Power`.
> ⚠️ **Il commit dichiara sé stesso non verificato**: finché build e suite non girano, «scritta» non è
> «funzionante», e questa riga non va letta come chiusa.

## 5. Domande che restano all'autore

Non sono rilievi: sono le uniche cose che questo panel non può decidere.

- ~~**`WV-1`**~~ **è stata chiusa mentre questo panel era in revisione**, da
  [D-090](../../decisions/RT_PDR_00_Decision_Log.md) ([#510](https://github.com/DegrassiAaron/refactor-tactics-main/issues/510),
  PR #518, mergiata quattro minuti prima di questa). La traduzione letterale «+1 turno di ricarica» è
  stata **scartata perché misurata a costo zero** — `TickCooldowns()` gira nel Cleanup dello stesso turno,
  quindi `CooldownTurns = 1` significa «ogni turno». Il costo è `+2`, il bonus per fascia `+18/+14/+8`.
  ⚠️ Questo **non** rilassa il rilievo 2.1: lo aggrava, perché un costo che ora esiste davvero è anche un
  costo che il campo legacy non applicato manda perduto. E resta vero che nessun eroe ha mai avuto
  l'attacco base in ricarica: quel percorso del gate `CanUseAbility` **non è mai stato percorso**.
- **`WV-2`**, le soglie delle fasce, si chiude con una partita e non con un documento — ma D-089 ne
  mostra già l'urgenza: con i delta assoluti di oggi Riktor ha **una sola** scelta sensata su quattro,
  ed è misurato nel corpo della decisione.
