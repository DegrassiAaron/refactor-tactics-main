# Scudo base e sorgente del danno — design

> **Stato**: ✅ **implementata** il 2026-08-28 e recepita come
> **[D-224](../../decisions/RT_PDR_00_Decision_Log.md)**. Questa spec resta il documento di *design*: per la
> regola vigente vale il Decision Log, non questo file.
>
> **Misura finale**: suite **1260/1260, 0 fallimenti**, `VALIDA` per [D-222].
>
> ⚠️ **Tre punti di questa spec sono stati superati dall'esecuzione**, e sono segnati sul posto:
> `RechargeBaseShield` sta nel **costruttore** e non in `BeginPlay`; `Action.Shield` **non** ha trovato un
> portatore (i dieci tasti di `AbilityHotkeys()`); il numero di decisione è **D-224**, non D-223 —
> quest'ultimo era già rivendicato da un branch in volo.

## Il problema

Ogni unità ha `Shield = 0` di default e lo scudo esiste solo quando un'azione lo concede. Il risultato è che
lo scudo è un evento raro, e le fonti di danno piccole non incontrano mai resistenza.

La regola nuova: **ogni personaggio ha 5 punti di scudo base**, che non crescono e si ricaricano a inizio
round.

Questa regola **supera** una decisione congelata nel PRD di ricerca del damage model — *«Shield regeneration:
nessuna rigenerazione automatica»*
([`../../research/prd/prd-damage-model-armor-shield.md`](../../research/prd/prd-damage-model-armor-shield.md)).
Il superamento è deliberato e va registrato come **D-224**, non lasciato implicito.

### Cosa la regola costa, misurato

| Fonte | Danno | Con 5 di scudo base |
|---|---|---|
| Attacco base (`BasicAttackDamageForRange`) | 28 / 25 / 22 / 20 | −18% … −25% |
| `Phase.PressureJet` | 16 | −31% |
| `Gadget.ReactiveCapacitor` (contrattacco) | 10 | −50% |
| `Burning` nel Cleanup | 8 | −63% |

🔴 **Il costo non è distribuito: cade sul danno piccolo.** È la ragione della decisione 4 più sotto — senza
di essa `Burning` diventerebbe ornamentale e i contrattacchi perderebbero metà del loro peso.

### Cosa è già corretto, e non va toccato

⚠️ **La prima stesura di questa spec rivendicava un difetto che non esiste** e prescriveva di correggerlo.
Falso: `ARTUnit::ApplyCombatState` decrementa già `TemporaryShield` in proporzione allo scudo perso, quindi
l'erosione temporaneo-prima è **implementata e corretta**.

```cpp
const int32 ShieldLost = FMath::Max(0, Shield - FMath::Max(0, NewShield));
TemporaryShield = FMath::Max(0, TemporaryShield - ShieldLost);
```

```
base 5, temporaneo 25   → Shield 30, Temp 25
incassa 10              → Shield 20, Temp 15      ← Temp aggiornato
Cleanup                 → Shield = 20 − 15 = 5    ✓ la base resta
```

∴ La **decisione 2 non richiede lavoro**: descrive il comportamento esistente e lo pinna con un test che
oggi manca. Ciò che manca davvero è la decisione **4** — nessuno oggi distingue la sorgente del danno.

## Decisioni

| # | Decisione | Alternativa scartata |
|---|---|---|
| 1 | Scudo base **5** per ogni unità, non cresce, si ricarica a inizio round | valore per eroe: senza crescita sarebbe un campo che nessuno scrive |
| 2 | Il danno erode **prima il temporaneo**, poi la base — ✅ **già implementato**, serve solo il test che lo pinna | erosione indistinta: lascia la traccia con 0 dove il design dice 5 |
| 3 | **Nessun tetto** sul temporaneo; il cap 5 vale solo per la base | `MaxShield` per eroe: nessuna decisione qui lo usa |
| 4 | La base assorbe **solo `Direct`**; l'ambientale la salta | assorbe tutto: spegne `Burning` e i contrattacchi |
| 5 | ~~`Action.Shield` entra nei kit di **Phase** e **Wraith**~~ — 🔴 **ritirata in esecuzione**: 6 azioni fanno **11** voci di kit contro i **10** tasti di `AbilityHotkeys()`, quindi l'azione sarebbe stata raggiungibile per il gate e impremibile dal giocatore | Gadget: avrebbe la terza fonte di scudo ([D-218]) |
| 6 | Estendere `FRTUnitCombatState` e `FRTAttack` | `FRTDamagePacket` completo: 4 campi su 6 speculativi |

## Modello dati

Nessun campo nuovo su `ARTUnit`. Lo scudo base è ciò che avanza dal totale:

```
invariante   BaseShield = Shield − TemporaryShield,   con 0 ≤ BaseShield ≤ 5
all'inizio   Shield = 5,  TemporaryShield = 0
```

Il 5 è una **costante di regole**, non una stat per eroe: «di base non aumenta», quindi `URTHeroData` non
viene toccato. Vive come `static constexpr int32 URTCombatLibrary::BaseShield = 5`, dove stanno **già** le
altre nove costanti di combattimento — `BurningCleanupDamage`, `GuardFirstHitReduction`,
`LowCoverDamageReduction` e le altre. Non in `Config/`: non è un parametro di formato come `RoundLimit`, è
una regola del combattimento, e la casa le tiene tutte lì.

⚠️ **Dove si applica il 5 iniziale.** Non è il default del campo in `ARTUnit.h`: quel valore verrebbe
sovrascritto da chiunque costruisca un'unità a mano, test compresi. Va assegnato in `ARTUnit::BeginPlay()`
chiamando `RechargeBaseShield()`, così esiste **un solo** punto che stabilisce il valore base e i test lo
esercitano invece di aggirarlo. Ogni test che assume uno scudo iniziale nullo va riletto — vedi
[Rischi](#rischi).

## Il ciclo

Round e turno sono lo stesso contatore (`MatchState.RoundNumber = TurnNumber`, `RTTurnManager.cpp:1542`), e
a inizio turno il temporaneo è già scaduto. La ricarica è quindi una riga e **non richiede un hook nuovo**:
va in coda al `Cleanup`, subito dopo `ExpireTemporaryShield()` (`RTTurnManager.cpp:1502`).

```
Blast → Move → Cleanup [ Burning · ExpireTemporaryShield · RechargeBaseShield ] → Planning
```

Così l'invariante *«a fine turno ogni unità viva ha esattamente 5 di scudo e 0 di temporaneo»* si verifica in
un punto solo.

```cpp
void ARTUnit::RechargeBaseShield()
{
    if (Health <= 0) { return; }          // i morti non si ricaricano
    Shield = RTRules::BaseShield + TemporaryShield;
}
```

La somma con `TemporaryShield` è ridondante nella posizione scelta — lì vale sempre 0 — ma tiene
l'invariante vera se un giorno l'ordine delle due chiamate cambia.

## Il resolver

L'enum è l'unica cosa nuova; `FRTUnitCombatState` guadagna un campo.

```cpp
enum class ERTDamageSource : uint8 { Direct, Environmental };            // nuovo

struct FRTUnitCombatState { int32 Health; int32 Shield; int32 TemporaryShield; };  // + TemporaryShield
```

⚠️ **`FRTAttack` NON cambia.** Il danno ambientale non passa dal resolver: `Burning`, danno da terreno e
propagazione elettrica chiamano `ApplyDamage` **direttamente** dal `TurnManager`. Ogni `FRTAttack` che
attraversa `ResolveAttacks` è un colpo del Blast, cioè `Direct` per costruzione — aggiungere un campo che
avrebbe sempre lo stesso valore sarebbe la stessa speculazione con cui è stato scartato `FRTDamagePacket`.
`ResolveAttacks` passa `ERTDamageSource::Direct` come costante.

⚠️ **La firma di `ApplyDamage` cambia, e ha sei chiamanti di produzione** — tutti da aggiornare nella stessa
fetta, altrimenti non compila. È un bene: un overload lascerebbe i vecchi chiamanti a saltare la regola
senza accorgersene.

| Chiamante | Cosa applica | Sorgente |
|---|---|---|
| `RTCombatResolver.cpp:21` | colpi del Blast | `Direct` |
| `RTTurnManager.cpp:177` | danno da terreno | `Environmental` |
| `RTTurnManager.cpp:1391` | `Burning` nel Cleanup | `Environmental` |
| `RTTurnManager.cpp:2346` | propagazione elettrica | `Environmental` |
| `RTTurnManager.cpp:4749` | colpo al decision boundary | `Direct` |
| `RTTurnManager.cpp:5118` | colpo di Overwatch | `Direct` |

```cpp
FRTDamageResult URTCombatLibrary::ApplyDamage(int32 Damage, ERTDamageSource Source,
                                              int32 Shield, int32 TemporaryShield, int32 Health)
{
    const int32 SafeDamage = FMath::Max(0, Damage);
    const int32 SafeShield = FMath::Max(0, Shield);
    const int32 SafeTemp   = FMath::Clamp(TemporaryShield, 0, SafeShield);
    const int32 SafeBase   = SafeShield - SafeTemp;

    // 1 — il temporaneo assorbe per primo, qualunque sia la sorgente
    const int32 FromTemp = FMath::Min(SafeDamage, SafeTemp);
    int32 Remaining      = SafeDamage - FromTemp;
    const int32 NewTemp  = SafeTemp - FromTemp;

    // 2 — la base assorbe SOLO il danno diretto
    const int32 FromBase = (Source == ERTDamageSource::Direct)
                         ? FMath::Min(Remaining, SafeBase)
                         : 0;
    Remaining           -= FromBase;
    const int32 NewBase  = SafeBase - FromBase;

    // 3 — il resto arriva alla salute
    const int32 NewHealth = FMath::Max(0, Health - Remaining);

    return FRTDamageResult(NewHealth, NewBase + NewTemp, NewTemp);
}
```

Ordine di erosione:

```
Direct         temporaneo → base → salute
Environmental  temporaneo → salute            (la base è saltata)
```

⚠️ **Assunzione dichiarata**: l'ambientale è fermato dallo scudo **temporaneo** ma non dalla base. È quanto
il PRD congelava (*«Environmental: Shield sì»*), e la lettura è che lo scudo che ti costruisci copre da
tutto mentre il cuscinetto passivo copre solo dai colpi. Se si decidesse che `Burning` ignora ogni scudo,
cambia la riga 1: `FromTemp` diventa condizionale come `FromBase`.

`ApplyCombatState` deve accettare e scrivere anche `TemporaryShield`, altrimenti il difetto descritto sopra
sopravvive alla correzione del resolver.

## Catalogo e roster

`Action.Shield` (Prep, priorità 35, cooldown 2, scudo 25) entra nei kit di **Phase** e **Wraith**, uno per
squadra — le formazioni sono fisse (`RTGameMode.h:87,90`: Team 0 = Gadget + Phase, Team 1 = Riktor + Wraith).

Perché non Gadget: [D-218] ha appena corretto Riktor perché il suo modulo duplicava un mestiere già nel kit,
e nella stessa riga registra che **Gadget ha lo stesso difetto non corretto** — `Reaction.ReactiveShield` e
`Hero.Gadget.ReactiveCapacitor` sono entrambi `Action.Counter` ed entrambi danno scudo. Sarebbe la terza
fonte di scudo sullo stesso eroe il giorno dopo aver deciso che i doppioni si tolgono.

Conseguenze:

- La riga di esclusione in `RTCatalogTests.cpp:501` va **tolta**. Come per `Action.Purge` con [D-218], è il
  gate `Catalog.EveryCoreActionIsReachableOrDeclared` a chiederlo diventando rosso da solo.
- `#1403` si chiude su due terzi delle azioni che elenca: resta `Action.Cleanse`.
- Da allineare: [`RT_ActionCatalog_v0.1.md`](../../balance/RT_ActionCatalog_v0.1.md) §4 e
  [`adr-0003`](../../decisions/adr-0003-modello-azioni-v01.md), che la dà «per arrivata».

## Test

| Test | Cosa fissa |
|---|---|
| Erosione a tabella | due sorgenti × quattro stati (solo base · solo temporaneo · entrambi · danno che sfonda) |
| Erosione temporaneo-prima | 5 base + 25 temp − 10 diretti → **5 al Cleanup**. Oggi già vero e **non testato**: il test pinna il comportamento, non lo cambia |
| Ambientale salta la base | 5 base + 0 temp − 8 `Burning` → salute −8, scudo invariato |
| Ambientale contro temporaneo | 5 base + 25 temp − 8 `Burning` → temp 17, base 5, salute intatta |
| Invariante di fine turno | ogni unità viva: `Shield == 5` e `TemporaryShield == 0` |
| Morte | un'unità a `Health == 0` non si ricarica |
| Raggiungibilità | `Action.Shield` esce dalle dichiarate; il gate resta verde senza quella riga |

Il **corpus golden va rigenerato**: `Shield` passa da 0 a 5 in ogni snapshot e l'hash copre `Shield`
(`RTMatchStateHash.h:55`). È la regola a imporlo, non l'implementazione — nessun approccio lo evitava.

## Rischi

| Rischio | Perché | Mitigazione |
|---|---|---|
| I test esistenti assumono `Shield == 0` | il default cambia da 0 a 5 | rileggere `RTUnitTests.cpp` (`AddTemporaryShield` 40/25/15), `RTDefensiveReactionTests.cpp`, i test del resolver **prima** di toccare il codice |
| `TemporaryShield` non è nell'hash | due stati con stesso totale ma quota diversa hashano uguale | fuori scope qui; va **registrato**, non corretto di straforo |
| Il TTK si allunga | fino a ~60 punti assorbiti su `RoundLimit` 10–14 | è l'effetto voluto; va misurato in playtest, non stimato |
| Sorgente dimenticata | ogni nuovo produttore di danno deve dichiararla | `ERTDamageSource` senza default nel costruttore di `FRTAttack`, così l'omissione non compila |

## Cosa resta fuori

Niente `DamageType`, `Shape`, `Piercing`, `Shred`, `Armor`, `DamageResistance` né `MaxShield` per eroe:
nessuna decisione di questa spec li usa, e restano fette di E49. `CLEANSE-1` non si chiude e `Action.Cleanse`
resta senza portatore.

Questa spec **anticipa a mano** una parte di 49.1 (la sola sorgente binaria). Quando E49 arriverà,
`FRTDamagePacket` dovrà assorbire `Source` da `FRTAttack`, non affiancarlo.
