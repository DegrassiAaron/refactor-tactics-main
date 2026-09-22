# Tassonomia icone — il manifest di design contro `ERTIconCategory`

> **Tipo**: documento owner di riconciliazione · **Creato**: 2026-09-22 · **Owner della domanda**: *quali
> categorie del materiale di design esistono nel runtime, quali no, e che cosa si fa di quelle che non ci
> sono* · **Issue**: [#637](https://github.com/DegrassiAaron/refactor-tactics-main/issues/637) ·
> **Epic**: [#217](https://github.com/DegrassiAaron/refactor-tactics-main/issues/217) (E20)
>
> ⛔ **Questo documento non decide come si compone una card** — quella è
> [`../systems/spec-icon-card-grammar.md`](../systems/spec-icon-card-grammar.md)
> ([D-231](../../decisions/RT_PDR_00_Decision_Log.md)) — **né quali icone servono alla v0.1**, che è
> [`brief-icone-v01.md`](brief-icone-v01.md). Possiede una cosa sola: la **corrispondenza fra i segmenti
> del manifest e i valori di `ERTIconCategory`**, e lo statuto della sorgente da cui il manifest viene.

**Convenzione sui numeri.** Ogni conteggio di questa pagina porta accanto il comando che lo produce ed è
datato. Un numero senza comando non va copiato da qui: si rimisura.

---

## 1. Perché esiste

⚠️ **Nessun codice legge il manifest.** Misurato:
`git grep -lni 'CLAUDE_DESIGN_02|UI_Icon_Manifest' -- Source/` risponde **zero**. Il catalogo non si
importa da quel file: lo **genera** `RTBuildIconCatalogCommandlet` a partire da
`URTIconLibrary::RequiredIconIds()` (`:132`), che deriva le chiavi dai dati di gioco.

∴ **una chiave del manifest col segmento fuori dall'enum non viene rifiutata: non ha una strada per
arrivare.** La differenza non è sottile — sposta la domanda da *«come la si fa passare»* a *«quel segmento
deve diventare qualcosa che `RequiredIconIds()` produce, oppure no»*, che è la domanda di questa pagina.

I tre punti del codice che nominano l'enum, e cosa fanno **davvero**:

| Dove | Che cosa fa |
|---|---|
| `CategoryForIcon()` — `Source/RefactorTacticsEditor/Private/Content/RTBuildIconCatalogCommandlet.cpp:36` | ⛔ **Guardia fail-closed sull'output di `RequiredIconIds()`, non un filtro sul manifest.** Scorre le chiavi **richieste** (`:252`) e, se una non cominciasse per una categoria dell'enum, il commandlet **ABORTISCE** — `return 1` a `:260`, con *«e' un errore della chiave, non dell'import»*. Non scarta una voce: ferma la build del catalogo |
| `URTIconLibrary::ValidateIconCatalog` | confronta il segmento dell'`IconId` con la `Category` **dichiarata nella voce**. ⚠️ **Sul catalogo generato è vero per costruzione**: il commandlet *deduce* la categoria dall'id invece di dichiararla — *«Dedurla qui invece di dichiararla evita l'unico errore che un catalogo scritto a mano fa davvero: chiave giusta, categoria sbagliata»*. Morde su un catalogo scritto a mano |
| `URTIconLibrary::IsDeclaredIconCategory` (`RTIconLibrary.cpp:29`) | dice se un capo è una categoria dichiarata. ⛔ **Non scarta niente**: a `:71` il falso fa cadere nel ramo che **ri-qualifica** il percorso sotto `Action.`. Solo `:136` (`MakeActionIconFallbackId`) restituisce `NAME_None` |

🔴 **Questa sezione ha sbagliato tre volte, e vale la pena scrivere come.** (1) Attribuiva a
`ValidateIconCatalog` un'enumerazione dell'enum che non fa. (2) Correggendola, attribuiva a
`IsDeclaredIconCategory` uno scarto che a `:71` non avviene, e non nominava il commandlet — che vive in un
**altro modulo**, `RefactorTacticsEditor`, e cercandolo in `RTIconLibrary.cpp` non si trova. (3) Nominatolo,
lo descriveva come *«scarta la voce»*: legge `Required`, non il manifest, e **aborta**. Ogni volta avevo
verificato che il simbolo **esistesse**, mai che facesse ciò che scrivevo.

Il materiale di design usa segmenti che il runtime non ha, quindi una parte del manifest **non è innestabile
così com'è**.

D-031 dà il criterio, e non è estetico: *il catalogo risolve ciò che il gameplay produce come chiave*. La
sua forma eseguibile è `URTIconLibrary::RequiredIconIds()`, che deriva le chiavi da **cinque** famiglie:

| Famiglia | Da dove il codice la deriva |
|---|---|
| `Phase` | i quattro valori volontari di `ERTMatchPhase` |
| `Action` | `URTCatalogLibrary::GetCoreActionCatalog()` più le abilità dei kit d'eroe |
| `Status` | i figli registrati del tag `Status.` |
| `Certainty` | i tre livelli di CP 11.2 |
| `Identity` | `URTHeroCatalogLibrary::GetHeroIds()` più la relazione di squadra |

⚠️ **Due delle cinque restano elenchi SCRITTI A MANO, per scelta dichiarata**: `Certainty` è un letterale
di tre voci (`RTIconLibrary.cpp:220`) sopra cui il codice scrive *«la conclusione si ROVESCIA: l'elenco
resta scritto a mano, per una ragione che prima non esisteva»*, e lo stesso vale per `Ally`/`Enemy` di
`Identity` (`:253`). ⛔ **Il criterio non è dunque «esiste un'enumerazione automatica»** — letto così
squalificherebbe `Certainty`, che la v0.1 spedisce — ma «**una macchina PRETENDE quelle chiavi**».

`RefactorTactics.IconCatalog.V01CategoriesPopulated` pinna esattamente queste cinque come popolate e le
altre come **vuote di proposito**, e il suo commento dice perché: *«una chiave che comparisse qui
chiederebbe un disegno per un sistema che ancora non la consuma»*.

∴ la domanda «questa categoria entra nell'enum?» ha una forma operativa: **esiste una macchina che ne
deriverebbe le chiavi dai dati di gioco?** Non «il manifest la nomina».

---

## 2. Statuto della sorgente di design — risolto, e due premesse di #637 sono scadute

🔴 **I sorgenti di design sono versionati dal 2026-08-12 — e la misura di #637 è scaduta in CINQUE ORE,
non in una settimana.** La issue è aperta alle **09:05Z**; il commit che mette quel materiale in git —
`7dc8a25e`, *«wip(icone): metto al sicuro il consolidamento UI e Icon Visual Language»* — è delle
**14:04Z** dello **stesso giorno**. La riga della issue era vera quando è stata scritta.

~~I sorgenti di design sono versionati dal 2026-08-19.~~ #637 è nata dichiarando che
`docs/src/design/icon/` era *untracked* e che non c'era *«una fonte stabile da riconciliare»*. Quel blocco
non esiste più: [#1165](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1165) ha spostato
quell'area in `docs/research/design/icon/` con il commit `ce05ed76` — *«quattro aree di `docs/src` prendono
il nome che dice cosa sono — solo move»*.

```
git log --follow --format='%h %ad %s' --date=short -- docs/research/design/icon/CLAUDE_DESIGN_02_Icon_Manifest_v0.1.md | tail -1
git ls-tree -r --name-only ce05ed76^ -- docs/src/design/icon/ | head -3   # erano gia' tracciati li'
git ls-files docs/research/design/icon/ | wc -l
```

⚠️ **`--follow` non è un dettaglio: è la differenza fra la data giusta e quella sbagliata.** Senza, git
legge il rename come un'aggiunta e risponde **2026-08-19**, che è la data di `ce05ed76` — il commit che ha
**spostato** l'area e il cui messaggio dice *«solo move»*. La prima stesura di questa sezione pubblicava
quel comando **e** quella data, citando due righe sopra il «solo move» che la smentiva.

⚠️ **Versionati non significa vincolanti, e la differenza è la risposta alla domanda «con quale
statuto».** `AGENTS.md` §2 elenca `docs/research/` fra ciò che **non è source of truth per default**. Quindi:

- il materiale **si può citare** e non si muove più sotto i piedi di chi lo legge: è in `main`, ha una
  storia, e una modifica produce un diff invece di un errore silenzioso;
- ma **non è specifica**: un batch di produzione che lo citasse come tale citerebbe una fonte non
  normativa. Ciò che vincola è il documento owner che lo **recepisce** — `brief-icone-v01.md` per le chiavi
  e il colore della v0.1, questa pagina per la tassonomia.

Questo è lo statuto, e non richiede una decisione nuova: lo fissa già `AGENTS.md` §2.

⚠️ **Seconda premessa scaduta**: due caselle della DoD di #637 rimandano a `docs/src/README.md` per la
regola *«un sorgente recepito non resta più qui: si sposta in `../archive/src/`»*. Quel file è stato
cancellato da **`273c76a6`** il **2026-08-16** — tre giorni **prima** di `ce05ed76`, che quindi non può
averlo rimosso: `git cat-file -e ce05ed76^:docs/src/README.md` risponde già *«does not exist»*. Quelle caselle **non sono eseguibili come
scritte** — non si spuntano su un file che non esiste, e vanno riformulate sulla regola di `AGENTS.md` §2
prima di poter essere dichiarate verdi.

---

## 2-bis. Che cosa costa davvero un valore nuovo — e la premessa che questa pagina dava era falsa

🔴 **Ritirata il 2026-09-22.** Questa pagina, `OPEN_DECISIONS.md` e il docstring di
`RTIconCatalogData.h` dicevano tutti che *«i numeri di `ERTIconCategory` sono serializzati negli asset»*, e
ne ricavavano che una categoria sbagliata **mente in modo permanente**. È falso su entrambi i punti.

| | Misura |
|---|---|
| L'asset porta **nomi**, non numeri | `grep -a -oE 'ERTIconCategory[A-Za-z:]*' Content/RT/UI/DA_IconCatalog.uasset` → `::Action` `::Certainty` `::Identity` `::Phase` `::Status` — le cinque popolate, e **zero** occorrenze delle altre sette |
| Il catalogo è un **artefatto di build** | `RTBuildIconCatalogCommandlet` fa `Catalog->Icons.Reset()` e poi **rideriva** la `Category` dalla stringa dell'`IconId` (`CategoryForIcon`). Si rigenera per intero |

∴ **niente è permanente, e la classifica dei costi si capovolge.** La premessa falsa rendeva l'uscita (b)
più cara di (a) esattamente al contrario di come la misura le ordina.

⚠️ **Il costo vero c'è, ed è un SILENZIO.** `RefactorTactics.IconCatalog.V01CategoriesPopulated` elenca le
categorie **a mano**, in due cicli: cinque popolate (`RTIconCatalogTests.cpp:136-137`) e sette vuote
(`:146-148`), somma esatta **12**. Un tredicesimo valore non compare in nessuno dei due — il gate resta
**verde** e smette di coprirlo, senza che nulla lo segnali. ⛔ Chi sceglie (b) aggiunge la propria riga in
quel test **nello stesso commit**: è la condizione che rende (b) una scelta e non una perdita di copertura.

La regola «solo in coda» **resta**, perché costa nulla ed è la disciplina giusta per un enum `BlueprintType`.
Ciò che cade è la **ragione** che le veniva data, non la regola.

---

## 2-ter. Quale file è «il manifest» — sono TRE, e portano popolazioni diverse

🔴 **Trovato il 2026-09-22, e cambia la popolazione prima del merito.** Tutti e tre sono tracciati:

| File | Chiavi | Segmenti | Che cos'è |
|---|---:|---:|---|
| `docs/research/design/icon/CLAUDE_DESIGN_02_Icon_Manifest_v0.1.md` | 195 | 25 | il documento di **design** — la fonte che §3 misura |
| `docs/research/design/icon/RefactorTactics_UI_Icon_Manifest_v0.1.csv` | 160 | 22 | **sottoinsieme stretto** del precedente |
| 🔑 **`Content/Icons/manifest.json`** | **124** | **10** | **generato** da `tools/hud-assets/generate_hud_assets.py`, e i suoi dieci segmenti sono **tutti dentro l'enum** |

```
for f in docs/research/design/icon/CLAUDE_DESIGN_02_Icon_Manifest_v0.1.md \
         docs/research/design/icon/RefactorTactics_UI_Icon_Manifest_v0.1.csv \
         Content/Icons/manifest.json; do
  printf '%-64s %4s %3s\n' "$f" \
    "$(grep -ohE 'UI\.Icon\.[A-Za-z]+\.[A-Za-z0-9_.]+' $f | sort -u | wc -l)" \
    "$(grep -ohE 'UI\.Icon\.[A-Za-z]+\.'      $f | sort -u | wc -l)"
done
```

⚠️ **La differenza non è editoriale.** `Timing` ha **tre** righe nel MD e **zero** nel CSV: quale file sia
di record decide se quella riga abbia una popolazione da arbitrare. E il terzo file dimostra che **la
pipeline spedita ha già abbandonato il vocabolario esteso**: genera solo dentro i dodici valori.

⛔ **Prima di scrivere una voce di registro va dichiarato quale sia il manifest di record.** Questa pagina
misura il **MD**, che è la fonte più larga e quella su cui #637 è nata.

---

## 2-quater. Due righe su sette sono già eseguite, e la decisione è di ratifica

🔴 **Non sono scelte aperte: sono fatti committati.** Gli SVG sono tracciati e le chiavi stanno nel
manifest **generato**.

| Riga | Che cosa è già in `main` |
|---|---|
| `ICON-TAX-1` — **`Module` → `Reaction`** | `RT_UI_Icon_Reaction_{AllyIntercept,HazardEscape,EmergencyDash,ReactiveShield}.svg` tracciati, e `UI.Icon.Reaction.*` presenti in `Content/Icons/manifest.json` |
| `ICON-TAX-4` — la sola **`Cooldown` → `Warning`** | `UI.Icon.Warning.Cooldown` presente nel manifest generato, col suo glifo |

```
git ls-files 'Content/Icons/Icons/*Reaction*'
grep -c UI.Icon.Reaction.AllyIntercept Content/Icons/manifest.json   # e le altre tre
grep -c UI.Icon.Warning.Cooldown       Content/Icons/manifest.json
grep -c UI.Icon.Objective              Content/Icons/manifest.json   # 0 — vedi §5
```

⚠️ **L'owner non sceglie fra tre uscite su queste due: ratifica o revoca.** E revocare ha un prezzo che
nessuno ha contato — sono asset disegnati e committati.

⚠️ **E c'è un'asimmetria che vale per tutte le righe che puntano a `Reaction`**: `MakeActionIconId`
**non traduce** se l'id è già in una categoria dichiarata, e il suo commento nomina `Reaction.HazardEscape`
per esempio (`RTIconLibrary.cpp:69-74`). L'esito (a) su `Reaction` **si verifica da solo** il giorno in cui
un id di modulo raggiunge quel ramo: chi volesse (b) deve arrivarci prima.

---

## 2-quinquies. Lo SCOPE che il manifest dichiara, e che nessuno aveva letto

`Scope` è la prima colonna del CSV, e la legenda del MD la definisce: `CORE` = *«creare per la v0.1 HUD»*;
`CORE_IF_SHOWN` = *«creare se il widget è visibile nella build v0.1»*.

| Segmento | Scope dichiarato |
|---|---|
| **`Result`** | 🔴 **`CORE`** — `Result.Success` e `Result.Failure`, lo scope più forte |
| **`Decision`** | 🔴 `Decision.FastReaction` **`CORE`**; `Decision.FastAction` `CORE_IF_SHOWN` |
| `Gadget` · `Weapon` · `Stat` · `Module` | `CORE_IF_SHOWN` |
| **`Timing`** | ⚠️ **assente dal CSV** — esiste solo nel MD |

```
grep -nE '^[A-Z_/]+,UI\.Icon\.(Stat|Gadget|Module|Weapon|Decision|Timing|Result)\.' \
  docs/research/design/icon/RefactorTactics_UI_Icon_Manifest_v0.1.csv | cut -d, -f1,2
```

⚠️ **Conta perché le due righe marcate `CORE` sono quelle che l'istruttoria proporrebbe di mandare fuori
dal linguaggio.** Lo scope non vincola — il manifest non è fonte autorevole (§2) — ma una voce che lo
contraddice deve dire perché, invece di ignorarlo.

---

## 2-sexies. Il terzetto dell'equipaggiamento è UN dato, non tre

🔴 `ERTEquipmentSlot` ha **tre** valori — `WeaponVariant`, `Gadget`, `ReactionModule`
(`Source/RefactorTactics/Ability/RTEquipmentData.h`) — passano tutti da **un solo** costruttore
(`MakeEquipmentAction`, `RTCatalogLibrary.cpp:902-908`), e **nessuno dei tre può essere preteso dal
catalogo, per costruzione**: il ramo eroi di `RequiredIconIds()` aggiunge una chiave solo quando
`MakeActionIconFallbackId(Def).IsNone()`, e per un equipaggiamento `DerivedFromActionId` è **sempre** scritto.

∴ sotto il criterio operativo di §1, `ICON-TAX-1` (`Module`), `ICON-TAX-2` (`Gadget`) e `ICON-TAX-3`
(`Weapon`) sono **lo stesso caso**. ⛔ Se ricevono uscite diverse, la voce di registro deve scrivere il
**discriminante misurato** che li separa — e oggi ne esiste uno solo, ed è il **nome**, non la semantica.

⚠️ **E il motore converte attivamente il pezzo in gesto**: `MakeEquipmentAction` riscrive
`Def.ActionId` con l'`EquipmentId`, e `MakeActionIconId` porta `Gadget.Sprinkler` →
`UI.Icon.Action.Sprinkler`. Nessuna superficie mostra un pezzo **come pezzo**: in v0.1 non esiste una
schermata di loadout, e `git grep -c "URTEquipmentData" -- Source/RefactorTactics/UI/` risponde **zero**.

> 🔴 **Difetto trovato per strada, fuori dal perimetro di #637 e senza gate che lo veda.**
> `MakeEquipmentAction` **non scrive mai** `DisplayName`, e `Heroes.EveryActionHasADisplayName` itera il
> **catalogo eroi**, non le azioni equipaggiate. In ogni partita di default lo `Gadget.Sprinkler` di
> `Hero.Muiren` e il `Gadget.PortableCover` / `Reaction.Cleanse` di `Hero.Branth` compaiono nel dock
> **senza nome**: solo il tasto e la ricarica. Va tracciato a parte.

---

## 3. La misura — rifatta il 2026-09-22, non copiata

Sui due file del manifest insieme (`CLAUDE_DESIGN_02_Icon_Manifest_v0.1.md` e
`RefactorTactics_UI_Icon_Manifest_v0.1.csv`), contando i segmenti dentro gli `UI.Icon.<Segmento>.<Nome>`:

| | 2026-08-12 | **2026-09-22** |
|---|---:|---:|
| chiavi uniche | 195 | **195** |
| segmenti distinti | 25 | **25** |
| chiavi con segmento **dentro** l'enum | 84 | **84** |
| chiavi con segmento **fuori** | 111 | **111** |
| segmenti fuori dall'enum | 17 | **17** |

✅ **La misura di agosto non è invecchiata**, ed è un esito che va scritto: il manifest non si è mosso da
quando #637 l'ha misurato — si è mosso il suo *percorso* (§2), che è un'altra cosa.

⚠️ **Il segmento non è la colonna «Categoria» del manifest.** Quella colonna porta accent e famiglia di
design (`Movement`, `Defense/Reaction`, `Utility`…) e non ha nulla a che vedere con l'enum. `ValidateIconCatalog`
guarda il segmento dell'`IconId`. Confonderli fa contare le categorie sbagliate.

```
grep -ohE 'UI\.Icon\.[A-Za-z]+\.[A-Za-z0-9_.]+' \
  docs/research/design/icon/CLAUDE_DESIGN_02_Icon_Manifest_v0.1.md \
  docs/research/design/icon/RefactorTactics_UI_Icon_Manifest_v0.1.csv \
  | sort -u | cut -d. -f3 | sort | uniq -c | sort -rn
```

**Quattro valori dell'enum non compaiono mai nel manifest**: `Certainty`, `Identity`, `Information`,
`MapInteraction`. La copertura va nella direzione opposta a quella che sembra, ed `Identity` è il caso che
#637 racconta — il manifest colloca gli alleati sotto `Target.*` mentre il codice li vuole sotto
`Identity.*`.

---

## 4. Dei diciassette, quanti sono davvero una decisione

| Sorgente | Quante chiavi | Esito | Chi lo stabilisce |
|---|---:|---|---|
| `Intel.*` | 9 | → **`Information`** | la descrizione dell'enum: *«cosa la squadra SA»* |
| `Map.*` | 12 | → **`MapInteraction`** | *«porte, leve, ponti, ascensori»*, alla lettera |
| `Surface.*` | 8 | → **`Environment`** | *«superfici e terreni»* |
| `Role.*` | 4 | → **`Identity`** | *«chi è: personaggio, **ruolo**…»* |
| `Faction.*` | 2 | → **`Identity`** | idem — *«…**fazione**…»* |
| `Boundary.*` | 2 | → **`Phase`**, con una riserva | ⚠️ Le due chiavi sono `Boundary.Phase` e **`Boundary.Decision`**: solo la prima è un confine di fase. ⛔ Ed è la sola delle sei la cui ragione **non** è una citazione dell'enum — il commento di `ERTIconCategory::Phase` dice *«in quale fase del turno: Prep, Dash, Blast, Move»*, non parla di confini. `Boundary.Decision` tocca `ICON-TAX-5` e non si chiude qui |
| `UI.*` | 6 | **esce dal catalogo** | è chrome, non semantica di gioco: D-031 risolve ciò che *il gameplay produce come chiave*, e un pulsante Undo non lo è |
| `Effect.*` | 14 | **nessun valore d'enum richiesto** | [D-231](../../decisions/RT_PDR_00_Decision_Log.md): primitiva della **grammatica compositiva** |
| `Geometry.*` | 7 | **nessun valore d'enum richiesto** | idem (`Shape`/`Geometry`) |
| `Target.*` | 8 | **sei disposte, DUE NO** | `Ally`/`Enemy` → `Identity`; `Cell`/`Object`/`Direction`/`Structure` restano primitive di composizione ([D-231](../../decisions/RT_PDR_00_Decision_Log.md)). 🔴 **`Target.Self` e `Target.Objective` non le colloca nessuno**, e la seconda è la peggiore: `Objective` **è** un valore d'enum con un proprio segmento, quindi lo stesso concetto starebbe sotto due segmenti senza una regola che dica quale vince |

Restano **sette** segmenti su cui nessuna fonte normativa si è pronunciata: `Stat` · `Gadget` · `Module` ·
`Weapon` · `Decision` · `Timing` · `Result`.

⚠️ **`spec-icon-card-grammar.md` §1 dichiara `ERTIconCategory` e `RequiredIconIds()` esplicitamente FUORI
dal proprio scope.** `D-231` chiude la scorciatoia — *«non serve un valore d'enum per comporre una card»* —
ma **non arbitra** questi sette. Chi cercasse lì la risposta troverebbe un non-scope, non un silenzio.

---

## 5. I sette, misurati contro il criterio

Per ognuno: il gameplay produce quel nome come **id**? Misurato il 2026-09-22 su `Source/`, esclusi i test.

```
# gli alberi di test sono DUE: escluderne uno solo misura una popolazione diversa da quella dichiarata
git grep -ohE '"<Segmento>\.[A-Za-z]+"' -- Source/ \
  ':!Source/RefactorTactics/Tests/' ':!Source/RefactorTacticsEditor/Private/Tests/' | sort -u

# 🔴 E il RIBALTAMENTO, che e' la meta' che conta: lo stesso comando col nome che il codice usa
git grep -ohE '"Reaction\.[A-Za-z]+"' -- Source/RefactorTactics/Ability/RTCatalogLibrary.cpp | sort -u
git grep -ownE 'VisionRange|NoiseAtCell|NoiseIdentificationLevel|ERTPredictiveOutcome' -- Source/ | head
```

### Hanno un'entità reale dietro — e la decisione costa meno di come questa pagina diceva

| Segmento | Chiavi | Cosa esiste nel codice |
|---|---:|---|
| **`Module`** | 7 | ✅ **tutti e sette**, sotto un altro nome: `Reaction.AllyIntercept`, `.Anchor`, `.Cleanse`, `.CounterShot`, `.EmergencyDash`, `.HazardEscape`, `.ReactiveShield` — gli `EquipmentId` che `URTCatalogLibrary::MakeReactionModules()` costruisce |
| **`Gadget`** | 8 | ✅ **sette su otto** con lo stesso nome: `Gadget.BreachCharge`, `.Insulator`, `.Medkit`, `.PortableCover`, `.Sensor`, `.SmokeEmitter`, `.Sprinkler`. Manca `Gadget.Anchor`, che il manifest nomina e il codice non ha |
| **`Weapon`** | 6 | ✅ corrispondenza **esatta, prefisso compreso**: `Weapon.Environmental`, `.Impact`, `.Overcharge`, `.Precision`, `.Split`, `.Suppressive`. 🔴 **È l'unico dei sette in cui coincide l'INSIEME INTERO dei nomi**, prefisso compreso. ⚠️ Il prefisso da solo non lo distingue — anche `Gadget` ce l'ha uguale; ciò che `Weapon` ha di unico è che non avanza un nome né da una parte né dall'altra |

🔴 **La divergenza `Gadget` è a UN verso solo**: il manifest ha `Gadget.Anchor`, che il codice non ha.
⚠️ **La prima stesura ne dichiarava due**, contando anche un `Gadget.Mine` «del codice» — che vive solo in
`Source/RefactorTacticsEditor/Private/Tests/RTLauncherScenarioBrowserTests.cpp`, cioè in una fixture.
L'errore veniva dal comando qui sopra prima che fosse corretto: escludeva **un** albero di test su due,
e il secondo è esattamente quello che ospita quella riga. ⚠️ **`Gadget.Anchor` è anche una trappola
di nome**: `Reaction.Anchor` e `Gadget.Anchor` sarebbero due cose diverse con lo stesso nome, e
`10-catalogo-sette-categorie.md` lo segnala già. Chi decide `Gadget` decide anche questo.

⚠️ **Su `Module` esiste una proposta in conflitto con l'istruttoria di #637, e va risolta esplicitamente**:
`10-catalogo-sette-categorie.md` — materiale di ricerca, quindi **non vincolante** (§2) — assegna i moduli a
**chiavi proprie in `Reaction`**, perché risolverli dal `GrantedActionId` farebbe collassare
`ReactiveShield` e `CounterShot`, che condividono `Action.Counter` con effetti opposti. L'istruttoria di
#637 obietta che la categoria `Reaction` dell'enum è il **ciclo di vita** di una reazione — *«armata,
opportunità, consumata, invalidata»* — e che metterci gli oggetti confonde la cosa con la sua macchina a
stati. **Le due osservazioni sono entrambe vere**: la prima dice perché i moduli servono chiavi proprie, la
seconda perché `Reaction` non è il posto. Nessuna delle due dice dove vanno.

### Non esistono come chiave — e due, mappate, appiattirebbero informazione vera

| Segmento | Chiavi | Cosa ha trovato la misura |
|---|---:|---|
| **`Stat`** | 11 | ❌ **zero id**. Sono **letture numeriche**: `Cooldown` e `Range` sono campi di `FRTActionDef`, `Health` e `Shield` di `ARTUnit`. Un numero non ha una chiave — e D-231 colloca costo/cooldown/cariche fra i **satelliti della card** (cerchio, alto-sinistra, max 1). 🔴 **Ma gli undici nomi non sono omogenei**: `Vision` e `Noise` **esistono** come concetti di gioco fuori da quei due header — `VisionRange`, `NoiseAtCell`, `NoiseIdentificationLevel`, `NoiseType` — quindi «non è una chiave» non equivale a «non è una cosa» |
| **`Decision`** | 2 | ❌ `FastAction` e `FastReaction` non esistono. L'enum che porta quel nome è `ERTReactionDecisionOutcome`, e ha **sei** valori — `Chosen`, `CollapsedByCondition`, `Immediate`, `NoDecider`, `Rejected`, `Timeout`. 🔴 Mapparci sopra due chiavi **perderebbe** quattro esiti |
| **`Timing`** | 3 | ❌ **zero tutte e tre**, per il criterio che questa pagina stessa dichiara. 🔴 *La prima stesura diceva «`Predictive` esiste davvero» citando `ERTPredictiveOutcome`/`ERTPredictiveTargeting`/`FRTPredictiveShot`: sono **tipi che contengono quella parola**, non id — non esiste nessun `Predictive.*`, e il criterio di §1 chiede l'id.* ⚠️ E il segmento è **assente dal CSV** (§2-quinquies): la sua popolazione dipende da quale manifest sia di record |
| **`Result`** | 2 | ⚠️ **La domanda era posta sull'asse sbagliato.** 🔴 *La prima stesura ribaltava lo zero sul suffisso `Result` e concludeva «non esiste». Le chiavi dicono però «Mission Success / Mission Failure», e l'esito di partita **esiste ed è canonico** — sotto il nome `Outcome`, non `Result`.* Restano veri: gli `ERT*Result` sono specifici per dominio, e `UI.Icon.Objective` ha **zero** glifi nel manifest generato. 🔴 Ed è l'unica riga marcata **`CORE`** (§2-quinquies) |

⚠️ **Zero riscontri non è di per sé una risposta**, ed è il difetto che #1403 racconta: un test che cercava
`MakeGenericActions` nel file sbagliato leggeva lo zero come *«non è generica»*. Qui la domanda è stata
**ribaltata** su ciascuno dei cinque segmenti a zero — *esiste sotto un altro nome?* — ed è così che
`Module` è risultato **pieno** (`Reaction.*`) mentre gli altri quattro sono risultati vuoti davvero.

---

## 6. Cosa NON si fa

⛔ **Non si forza una categoria in una esistente per far passare il validator.** Il validator confronta il
segmento con la categoria dichiarata e **non giudica se la classificazione ha senso**: una forzatura passa
in verde, e nessun gate la troverà mai. ⚠️ *Questa riga diceva «mente in modo **permanente**, perché i
numeri dell'enum sono serializzati negli asset»: era falso su entrambi i punti — vedi §2-bis.*

⛔ **Non si aggiunge un valore all'enum se non in coda**, e non lo si aggiunge affatto senza una voce di
Decision Log. 🔴 **Ma il costo non è quello che `RTIconCatalogData.h` dichiarava** (§2-bis): è che
`IconCatalog.V01CategoriesPopulated` elenca le categorie **a mano**, in due cicli che sommano 5 + 7 = 12
(`RTIconCatalogTests.cpp:136-137` e `:146-148`). Un tredicesimo valore non compare in nessuno dei due: il
gate resta **verde** e smette silenziosamente di coprirlo. Chi aggiunge un valore **aggiunge la sua riga**
in quel test, nello stesso commit.

⛔ **Non si aggiunge oggi una chiave in una delle sette categorie che #219 prescrive vuote** — ma ⚠️ **il
divieto morde solo dove si crede**. `V01CategoriesPopulated` costruisce il proprio insieme da
`RequiredIconIds()`, **non** dalle voci del catalogo: una `FRTIconDef` aggiunta al data asset in una delle
sette passa quel test, passa `ValidateIconCatalog` e passa `FindMissingRequiredIcons`. Il rosso arriva
solo per una chiave che entra in `RequiredIconIds()`. 🔴 Scriverlo come divieto generale prometteva un
rosso che non sarebbe arrivato — e `10-catalogo-sette-categorie.md` propone già una di quelle chiavi.

⛔ **Non si modifica `ERTIconCategory` per derivare una risposta.** Se una modifica all'enum sembra il modo
di chiudere una riga di §5, la decisione è stata **dedotta invece che presa**.

---

## 7. Puntatori

- [`brief-icone-v01.md`](brief-icone-v01.md) — quali chiavi servono alla v0.1 e di che colore sono
- [`../systems/spec-icon-card-grammar.md`](../systems/spec-icon-card-grammar.md) — come si compone una card
  ([D-231](../../decisions/RT_PDR_00_Decision_Log.md)); dichiara `ERTIconCategory` fuori scope
- `Source/RefactorTactics/UI/RTIconCatalogData.h` — l'enum e la regola di serializzazione
- `Source/RefactorTactics/UI/RTIconLibrary.cpp` — `RequiredIconIds()` e `ValidateIconCatalog()`
- 🔴 `Source/RefactorTacticsEditor/Private/Content/RTBuildIconCatalogCommandlet.cpp` — `CategoryForIcon()`:
  **il punto che scarta davvero** una chiave col segmento fuori dall'enum. Vive in un altro modulo
- `Source/RefactorTactics/Tests/RTIconCatalogTests.cpp` — `V01CategoriesPopulated` e gli altri gate
- `docs/research/design/icon/` — il materiale di design: **versionato, non vincolante** (`AGENTS.md` §2)
- [#266](https://github.com/DegrassiAaron/refactor-tactics-main/issues/266) — CP 25.1, che porta a spec la
  tassonomia **dopo** che #637 l'ha decisa. Le due non si duplicano: #637 **decide**, #266 **specifica**
