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
| 2.1 | Il ponte fra `FRTActionDef` e i campi legacy che il **bot** legge esiste solo in un helper di test | `RTCatalogLibrary.cpp` · `RTEquipmentTests.cpp` | 🔴 **alta** |
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

L'unico codice che colma la distanza sono **due righe dentro un helper di test**:

```cpp
// Source/RefactorTactics/Tests/RTEquipmentTests.cpp:429-431
Basic->Def = URTCatalogLibrary::ApplyWeaponVariant(Basic->Def, Variant);
Basic->RangeCells   = Basic->Def.RangeCells;    // <-- il ponte
Basic->CooldownTurns = Basic->Def.CooldownTurns; // <-- esiste solo qui
```

**NYGARD**: «Il modo di fallimento è silenzioso e cade sul bot, cioè sull'avversario di *ogni* partita
della v0.1.» Le due letture divergono:

| Chi legge | Campo | Riga |
|---|---|---|
| **Bot**, generazione candidati d'attacco | `Ability->RangeCells` *(legacy)* | `RTTurnManager.cpp:406`, `:460` |
| **Resolver**, validazione in esecuzione | `Def.RangeCells` | `RTTurnManager.cpp:1650` |

Scenario concreto, se `#63` (CP 7.4 — Loadout, **OPEN**) cablerà il loadout assegnando solo `->Def`:

- **`Weapon.Precision`** (+1 portata): il bot pianifica alla portata **vecchia, più corta** → non usa mai
  la cella che ha pagato 4 danni per ottenere. Il vantaggio della variante è invisibile a chi la porta.
- **`Weapon.Impact`** (−1 portata): il bot pianifica alla portata **vecchia, più lunga** → genera un
  candidato che il resolver poi **rifiuta** con `«fuori portata»` (`:1652`). È un attacco pianificato che
  fallisce in silenzio — cioè il *pulsante finto* che il catalogo, in `ApplyWeaponVariant:495-497`,
  dichiara esplicitamente di voler evitare.
- **`Weapon.Overcharge`** (+1 ricarica): il gate `CanUseAbility` (`:405`) legge `AbilityCooldowns`,
  alimentato dal legacy `Ability->CooldownTurns` (`RTUnit.cpp:491`). Il costo della variante — che è
  l'intero suo prezzo, `WV-1`/[#510](https://github.com/DegrassiAaron/refactor-tactics-main/issues/510)
  — non verrebbe mai applicato. `+6` danni gratis.

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
  Flux→Precisione, Riva→Impatto, Bastion→Impatto, Vektor→Soppressione.

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
  CP 7.1 era in `main`, il validator già imponeva lo svantaggio. Un pacchetto di consolidamento si
  filtra, non si applica.

## 4. Raccomandazioni, in ordine

| # | Azione | Costo | Perché ora |
|---|---|---|---|
| 1 | `EquipWeaponVariant` di produzione; il test chiama quella | poche righe | **Prima** che `#63` cabli il loadout: dopo, il difetto è già dentro |
| 2 | Riscrivere la nota §1 del catalogo perché citi D-089 | una frase | Un owner che afferma il falso |
| 3 | Marcatore di stato su D-089 | una frase | Allinea la quinta decisione alle sorelle |

Nessuna delle tre tocca un numero di bilanciamento, e nessuna riapre una decisione `Locked`.

## 5. Domande che restano all'autore

Non sono rilievi: sono le uniche cose che questo panel non può decidere.

- **`WV-1`** ([#510](https://github.com/DegrassiAaron/refactor-tactics-main/issues/510)) resta il vincolo
  che blocca di più: finché «+1 turno di ricarica» non ha semantica, `Weapon.Overcharge` è un vantaggio
  senza prezzo — ed è la ragione per cui D-089 non l'ha dato come default a nessuno. Il rilievo 2.1
  aggiunge un dato al problema: nessun eroe ha mai avuto l'attacco base in ricarica, quindi quel percorso
  **non è mai stato percorso** dal gate `CanUseAbility`.
- **`WV-2`**, le soglie delle fasce, si chiude con una partita e non con un documento — ma D-089 ne
  mostra già l'urgenza: con i delta assoluti di oggi Bastion ha **una sola** scelta sensata su quattro,
  ed è misurato nel corpo della decisione.
