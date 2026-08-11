> 🗄️ **ARCHIVIATO il 2026-08-11 — consumato.** Questo è un **sorgente**, non un owner: si legge per la
> provenienza, mai per la regola. Il contenuto recepito vive ora nelle fonti canoniche.
>
> **Cosa è entrato** — le quattro decisioni `Locked` sono
> [D-085](../../decisions/RT_PDR_00_Decision_Log.md) (le spinte additive si sommano; corretto il difetto
> che ne è emerso), [D-086](../../decisions/RT_PDR_00_Decision_Log.md) (affinità naturali ammesse),
> [D-087](../../decisions/RT_PDR_00_Decision_Log.md) (delta di danno per fascia, **principio** deciso e
> numeri no) e [D-088](../../decisions/RT_PDR_00_Decision_Log.md) (quattro varianti giocabili in v0.1).
> Le `Provisional` e le `Open` sono `WV-1`…`WV-5` in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).
> Il catalogo owner è [`RT_EquipmentCatalog_v0.1.md`](../../balance/RT_EquipmentCatalog_v0.1.md).
>
> **Cosa NON è entrato, e perché** — le §18–§29 prescrivono un piano (PDR da riscrivere, dieci issue, pagine
> Wiki, epic nuova) costruito su una fotografia del repository **più arretrata del repository stesso**:
> quando il documento è stato scritto, CP 7.1 era già in `main` con le sei varianti, il validator già
> imponeva lo svantaggio, e `Equipment.SplitHasNoConsumerYet` già pinnava il limite di `Split`. Un
> pacchetto di consolidamento si **filtra**, non si applica: le prescrizioni già soddisfatte non si
> rieseguono, e quelle che contraddicono un ADR non sono conflitti — un sorgente è l'ultima fonte della
> gerarchia. In particolare **non** è stata creata l'epic «Weapon Variants & Horizontal Loadouts»: E7
> esiste già ed è il suo posto.
>
> ⚠️ Il documento chiama `WeaponVariant.Overload` ciò che il catalogo chiama **`Weapon.Overcharge`**, e
> `MultiTarget` ciò che è **`Weapon.Split`**. Valgono i nomi del catalogo.

# RefactorTactics — Weapon Variants / Affinità / Fasce di Danno
## File di consolidamento per Claude

**Scopo:** integrare e consolidare nelle fonti di progetto quanto deciso nel brainstorming sulle varianti d'arma dell'attacco base.

**Contesto progetto:** RefactorTactics, tattico a turni simultanei su griglia esagonale multilivello, simulazione deterministica integer-only, v0.1 offline 2v2 contro bot.

---

# 1. Istruzione principale per Claude

Usa questo documento come **delta di design** da integrare nella documentazione esistente.

Aggiornare e consolidare, dove applicabile:

- `Docs/`
- PDR tecnici e di gameplay pertinenti
- Wiki
- Roadmap
- Feature Map
- Scenario Map
- Editor Map
- backlog / milestone
- epic e issue GitHub
- test plan
- balance documentation
- eventuali Data Asset schema / validator spec

Non duplicare concetti già presenti: **consolidare**, collegare e correggere eventuali parti in conflitto.

Le decisioni esplicite qui sotto prevalgono sulle proposte precedenti non ancora approvate.

---

# 2. Contesto del sistema

Ogni unità ha per turno:

- 1 slot movimento;
- 1 slot azione principale.

Le varianti d'arma modificano **solo l'attacco base** del personaggio.

Una unità equipaggia **una sola variante d'arma**.

Le varianti devono essere scelte orizzontali:

> cambiano il modo di giocare, non rappresentano upgrade verticali.

Vincoli invarianti:

1. niente rarità, livelli o upgrade puri;
2. ogni variante deve pagare un costo misurabile;
3. nessun branch per eroe nel codice;
4. le differenze devono essere data-driven;
5. numeri interi soltanto;
6. niente moltiplicatori o percentuali;
7. il resolver rimane deterministico;
8. stesso snapshot + rules/version + dati + seed => stesso risultato.

---

# 3. Roster coinvolto

Numeri dell'attacco base discussi:

| Eroe | Attacco base | Danno | Portata | Identità rilevante |
|---|---|---:|---:|---|
| Flux | ArcPulse | 22 | 4 | bonus condizionale contro bersaglio Wet |
| Riva | PressureJet | 16 | 5 | applica Wet + Push 1 |
| Vektor | PulseShot | 21 | 4 | attacco base neutro |
| Bastion | ImpactShot | 8 | 3 | Utility / Emergency, attacco deliberatamente debole |

Nota importante:

- Bastion ha deliberatamente un danno base molto inferiore agli altri.
- Il suo attacco non deve essere “normalizzato” verso un ruolo DPS.
- La progettazione delle varianti deve supportare anche attacchi base a forte componente utility.

---

# 4. Varianti originarie

Catalogo concettuale:

| Variante | Vantaggio | Svantaggio |
|---|---|---|
| Precisione | +1 portata | danno ridotto |
| Impatto | +1 Push | −1 portata |
| Sovraccarico | danno aumentato | +1 turno di ricarica |
| Multiplo | +1 bersaglio | danno ridotto |
| Soppressione | applica Slow | danno ridotto |
| Ambientale | migliora hazard prodotti | costo ancora da definire precisamente |

---

# 5. DECISIONI APPROVATE IN QUESTA CHAT

## D1 — Affinità naturali tra eroi e varianti

**APPROVATO.**

Non si cerca parità universale tra tutte le varianti e tutti gli eroi.

È accettabile e desiderabile che:

- certe varianti appartengano naturalmente a certi eroi;
- una stessa variante abbia valore diverso su attacchi differenti;
- emergano sinergie e anti-sinergie specifiche del kit.

Vincolo:

> una combinazione legale può essere subottimale o situazionale, ma non deve diventare una scelta completamente morta o priva di senso.

Quindi non usare come obiettivo di bilanciamento:

> “ogni variante deve valere la stessa percentuale del danno base”.

Usare invece:

> “ogni variante deve creare un trade-off reale e leggibile per l'eroe che la equipaggia”.

---

## D2 — Fasce di danno al posto di delta assoluti universali

**APPROVATO IL PRINCIPIO.**

I costi/bonus in danno delle varianti non devono usare necessariamente lo stesso delta assoluto per tutti gli attacchi.

La variante deve poter scegliere il proprio delta in base a una **fascia del danno base dell'attacco**.

Motivazione:

- `+6` su Bastion 8 vale troppo rispetto a `+6` su Flux 22;
- `−6` su Bastion 8 distrugge completamente l'attacco;
- vogliamo preservare affinità senza generare combinazioni morte.

### Regola fondamentale

La fascia viene determinata dal **danno base della definizione dell'attacco prima dei modificatori della variante**.

Non cambiare fascia dinamicamente per:

- Wet;
- buff;
- debuff;
- status temporanei;
- effetti ambientali;
- bonus di turno.

Questo evita variazioni circolari o poco leggibili del costo della variante durante il match.

### Baseline NUMERICA PROVVISORIA, NON ANCORA BLOCCATA

È stata proposta come prima baseline:

| Fascia | Danno base |
|---|---:|
| Low | 1–10 |
| Medium | 11–18 |
| High | 19+ |

Proposta iniziale dei delta:

| Variante | Low | Medium | High |
|---|---:|---:|---:|
| Precisione | −2 | −3 | −4 |
| Sovraccarico | +3 | +5 | +6 |
| Soppressione | −2 | −4 | −5 |

**Non considerare questi numeri definitivamente approvati.**
Il principio “usa fasce” è approvato; soglie e valori devono essere playtestati e possono cambiare.

---

## D3 — v0.1 usa solo quattro varianti realmente funzionanti

**APPROVATO.**

Nel ruleset giocabile della v0.1:

- Precisione: ENABLED
- Impatto: ENABLED
- Sovraccarico: ENABLED
- Soppressione: ENABLED
- Multiplo: DISABLED / EXPERIMENTAL
- Ambientale: DISABLED / EXPERIMENTAL

Non mostrare nella UI della v0.1 varianti che oggi producono solo il loro svantaggio.

Il catalogo può mantenere ID stabili e definizioni future.

Esempio concettuale:

```text
WeaponVariant.Precision      Enabled
WeaponVariant.Impact         Enabled
WeaponVariant.Overload       Enabled
WeaponVariant.Suppression    Enabled

WeaponVariant.MultiTarget    Disabled.Experimental
WeaponVariant.Environmental  Disabled.Experimental
```

---

## D4 — I Push si sommano

**APPROVATO.**

Gli effetti di Push additivi si sommano.

Regola concettuale:

```text
FinalPush =
    BasePush
  + VariantPush
  + altri modificatori additivi validi
```

Esempio confermato:

```text
Riva PressureJet
BasePush = 1

WeaponVariant.Impact
VariantPush = +1

FinalPush = 2
```

Quindi:

> **Riva + Impatto => Push 2.**

Non introdurre eccezioni specifiche per Riva.

Se questa combinazione risulta troppo forte, bilanciare:

- costo della variante;
- range;
- cooldown se previsto;
- regole generali del displacement;

NON aggiungere:

```cpp
if (Character == Riva)
{
    ...
}
```

---

# 6. Conseguenze di design

## 6.1 Bastion non deve essere il riferimento per normalizzare il roster

Bastion è deliberatamente Utility/Emergency.

Il fatto che alcune varianti abbiano valore diverso su di lui è accettabile.

Il problema da evitare è:

- una variante impossibile da scegliere;
- un attacco ridotto a danno puramente simbolico senza payoff equivalente;
- una variante che diventa gratuitamente forte perché Bastion non valorizza il danno perso.

Quindi misurare il trade-off sul valore tattico complessivo, non sulla sola percentuale di danno.

---

## 6.2 Riva + Impatto diventa uno stress test prioritario

PressureJet ha già:

- danno;
- Wet;
- Push 1.

Con Impatto:

- Push 2;
- range ridotto di 1 secondo la baseline attuale.

Questa combinazione può diventare molto forte su:

- hazard;
- acqua/elettricità;
- bordi;
- porte;
- choke point;
- cover;
- objective zones;
- killzone alleate;
- Overwatch / reaction zones.

Va testata esplicitamente.

---

## 6.3 Evitare eccezioni per eroe

La regola di progetto resta:

> C++ definisce cosa è possibile; dati e Blueprint scelgono quale variante è usata.

Le differenze per eroe devono derivare da:

- attacco base;
- Effects già presenti;
- Variant Definition;
- band selezionata;
- tag;
- rule tables;
- dati.

Mai da codice condizionale sul CharacterId.

---

# 7. Multiplo — stato e requisito futuro

## Stato v0.1

DISABLED / EXPERIMENTAL.

Motivo:

il motore corrente non dispone di un concetto generale di cardinalità dei target.

Una Action Definition dichiara oggi:

- shape;
- range;
- targeting policy;

non:

- NumberOfTargets.

Quindi Multiplo oggi darebbe soltanto il proprio costo in danno.

Non è accettabile come scelta giocabile.

---

## Implementazione futura corretta

Non limitarsi ad aggiungere banalmente:

```cpp
int32 TargetCount;
```

Prima definire:

- chi seleziona i target;
- ordine di selezione;
- targeting automatico vs manuale;
- distanza dei target;
- duplicate prevention;
- ordinamento stabile;
- serialization;
- intent;
- UI;
- ghost preview;
- bot;
- TurnLog;
- moving-target policy;
- validazione;
- replay;
- test deterministici.

Riattivare la variante solo quando questa capacità serve realmente anche al sistema abilità.

---

# 8. Ambientale — stato e requisito futuro

## Stato v0.1

DISABLED / EXPERIMENTAL.

Problema attuale:

“migliora gli hazard” non è sufficientemente definito.

Prima di implementarlo bisogna scegliere cosa viene modificato:

- HazardIntensity;
- HazardDuration;
- area;
- propagation;
- numero celle;
- stato prodotto;
- persistenza;
- altro parametro esplicito.

La variante dovrà avere anche un costo misurabile conforme alle regole del progetto.

Esempi futuri possibili:

```text
HazardIntensity +1
DamageBandDelta negativo
```

oppure:

```text
HazardDuration +1 turn
Range -1
```

Non usare una proprietà generica tipo:

```text
BetterHazards = true
```

---

# 9. Default delle varianti per personaggio

È stata discussa questa baseline:

| Eroe | Default proposto |
|---|---|
| Flux | Soppressione |
| Riva | Precisione |
| Vektor | Sovraccarico |
| Bastion | Impatto |

Razionale:

- Flux → controllo oltre al danno;
- Riva → estende il setup Wet/Push pagando danno;
- Vektor → enfatizza il burst e il ritmo di tiro;
- Bastion → trasforma l'attacco in strumento di displacement/emergenza.

**STATO: proposta forte, non ancora dichiarata esplicitamente definitiva dall'utente.**

Non marcarla come decisione locked senza ulteriore conferma.

Principio invece approvabile come linea di design:

> il default dovrebbe principalmente rinforzare l'identità dell'eroe; la compensazione delle debolezze è più interessante come scelta alternativa del giocatore.

---

# 10. Decisione ancora aperta: Sovraccarico

La semantica precisa di:

```text
+1 turno di ricarica
```

sull'attacco base non è ancora stata approvata definitivamente.

Proposta discussa:

```text
Turno N:
Attack Overload

Turno N+1:
attacco base indisponibile

Turno N+2:
attacco base nuovamente disponibile
```

quindi:

```text
Fire -> Cooldown -> Fire
```

Questa decisione è importante perché determina il vero costo di Sovraccarico.

### Claude deve

- mantenere la questione come OPEN DECISION;
- non trasformarla in requisito consolidato senza conferma;
- creare, se opportuno, una issue di decisione/balance.

---

# 11. Motore attuale — capability wall

Rispettare i limiti attuali.

Oggi sono esprimibili:

- Damage;
- Heal;
- Status;
- Push;
- Pull;
- Damage Reduction;
- delta additivi;
- range delta;
- cooldown in turni, se il sistema lo supporta secondo la definizione effettiva.

Non sono oggi esprimibili genericamente:

- numero arbitrario di bersagli;
- “migliora hazard” senza uno schema esplicito;
- self displacement tramite Push/Pull del target.

Se Claude propone di espandere il motore, deve:

1. dichiarare che è nuova capability;
2. stimarne il costo;
3. indicare quali altre feature la riutilizzano;
4. evitare di aggiungerla solo per salvare una singola variante.

---

# 12. Schema dati suggerito

Allineare con Primary Data Assets, ID stabili, versioning e validator già previsti dal progetto.

Esempio concettuale:

```cpp
enum class ERTAttackDamageBand : uint8
{
    Low,
    Medium,
    High
};
```

Una Weapon Variant Definition può contenere concettualmente:

```text
VariantId
Version
EnabledRulesets
Tags

RangeDelta
CooldownDelta

PushDelta
PullDelta

DamageDeltaByBand:
    Low
    Medium
    High

AppliedStatuses[]

ExperimentalFlags
```

La selezione del band deve avvenire durante la materializzazione/validazione della definizione logica dell'attacco.

Il resolver deve lavorare sui valori già risolti in modo deterministico.

Non usare valori calcolati da Blueprint durante la resolution competitiva.

---

# 13. Validator da aggiungere

Aggiungere controlli data-driven.

## Error

- VariantId vuoto o duplicato.
- Damage band mancante.
- variante abilitata ma vantaggio non supportato dal motore.
- variante abilitata con solo svantaggi e nessun payoff funzionante.
- configurazione di band sovrapposte o con gap.
- valori non interi.
- riferimento a Status/Effect ignoto.
- variant non compatibile con ruleset ma esposta come selectable.

## Warning

- FinalDamage troppo basso per una combinazione legale.
- Push totale sopra una soglia di review.
- variante molto più efficace su un solo Character/Attack profile.
- una variante presenta zero costo effettivo su un attacco.
- una variante è selezionata come default ma è Experimental.
- combinazione `BasePush + VariantPush >= 2` richiede scenario di displacement regression.

Il validator non deve correggere automaticamente i valori.

Deve segnalare.

---

# 14. Balance methodology

Non usare come metrica primaria:

```text
percentuale del danno base
```

Non creare immediatamente un singolo “Power Score” con pesi arbitrari.

Usare scenario deterministici e metriche separate.

Metriche minime:

- Damage dealt;
- attack opportunities;
- wasted attacks;
- KO conversion;
- cells displaced;
- Slow turns applied;
- enemy movement lost;
- forced reroutes;
- objective possession;
- turns affected by cooldown;
- overkill;
- turns to win;
- objective score.

Poi usare confronto Pareto.

Una variante A è sospetta se:

- non è peggiore nelle metriche rilevanti;
- è migliore in almeno una;
- il risultato persiste su più scenari/matchup/mappe/policy bot.

---

# 15. Matrice di test iniziale

Con 4 eroi e 4 varianti attive:

```text
4 x 4 = 16 loadout
```

Creare fixture/scenari deterministici almeno per:

## Flux

- Precisione
- Impatto
- Sovraccarico
- Soppressione

Verificare in particolare:

- Wet synergy;
- range;
- burst;
- control.

## Riva

Tutte e 4.

PRIORITÀ:

```text
Riva + Impact
=> PressureJet Push 2
```

Testare:

- spinta libera;
- spinta contro unità;
- spinta verso hazard;
- spinta verso acqua;
- spinta verso acqua elettrificata;
- spinta contro cover/muro;
- spinta su objective;
- spinta su bordo non percorribile;
- push blocked;
- collisione simultanea.

## Vektor

Verificare soprattutto:

- Precisione e accesso ai target;
- Sovraccarico e downtime;
- trade-off danno/control.

## Bastion

Verificare:

- nessuna variante legale morta;
- Impact come utility;
- Suppression con danno basso ma effetto utile;
- Precision come utility a distanza;
- Overload non trasformi automaticamente Bastion in damage dealer.

---

# 16. Bot testing

Aggiungere in roadmap/scenario map test con almeno tre policy.

## Aggressive / Kill

Ottimizza:

- damage;
- KO;
- focus fire.

## Objective

Ottimizza:

- posizione;
- controllo;
- displacement;
- contest.

## Positional / Conservative

Ottimizza:

- sicurezza;
- cover;
- denial;
- distanza.

Scopo:

una variante non deve apparire dominante soltanto perché il bot è scritto per valorizzarne la metrica.

Se una variante domina sotto più policy, più matchup e più mappe, aprire balance issue.

---

# 17. TurnLog / explainability

Il TurnLog deve mostrare chiaramente il contributo della variante.

Esempi concettuali:

```text
PressureJet
BasePush: 1
WeaponVariant.Impact: +1
FinalPush: 2
```

```text
ArcPulse
BaseDamage: 22
DamageBand: High
WeaponVariant.Suppression: -5
FinalDamageBeforeCombatModifiers: 17
AppliedStatus: Slow
```

```text
PulseShot
BaseDamage: 21
DamageBand: High
WeaponVariant.Overload: +6
FinalBaseAttackDamage: 27
CooldownDelta: +1
```

La UI non deve ricalcolare in autonomia questi valori.

Il resolver/log deve fornire reason/modifier data sufficienti.

---

# 18. Wiki da aggiornare

Creare o consolidare pagine tipo:

## Gameplay / Weapon Variants

Contenere:

- filosofia orizzontale;
- affinità;
- catalogo;
- varianti v0.1;
- experimental;
- band;
- stacking;
- esempi.

## Gameplay / Damage Bands

Spiegare:

- perché esistono;
- che usano il BaseDamage;
- che non cambiano durante il match;
- valori correnti;
- stato provisional/locked.

## Gameplay / Displacement

Aggiungere regola:

> Push additivi si sommano.

Documentare:

- stacking;
- blocking;
- collision;
- bordi;
- hazard;
- simultaneità;
- reason codes.

## Characters / Riva

Aggiornare il caso:

```text
PressureJet base Push 1
Impact variant => Push 2
```

## Characters / Bastion

Specificare che:

- attacco base basso è intenzionale;
- Utility/Emergency;
- le varianti non devono necessariamente preservare lo stesso valore relativo degli eroi DPS.

---

# 19. PDR da consolidare

## PDR-07 — Personaggi, abilità e GAS

Integrare:

- varianti orizzontali;
- affinità;
- band;
- stacking displacement;
- 4 varianti attive;
- 2 experimental;
- default proposti come non locked.

## PDR-09 — Data / Validation

Integrare:

- Stable Variant ID;
- versioning;
- band definitions;
- EnabledRulesets;
- Experimental flag;
- validator;
- hash nel content manifest.

## PDR-05 — Deterministic Simulator

Integrare:

- risoluzione dei modifier additivi;
- ordine stabile;
- final resolved attack definition;
- TurnLog modifier breakdown;
- Push stacking deterministico.

## PDR-10 — Roadmap / QA

Aggiungere:

- Weapon Variant MVP;
- 16-loadout matrix;
- Riva Push 2 stress test;
- balance telemetry;
- bot policy comparison;
- experimental variants future.

## PDR-08 — UI/UX

Aggiungere:

- loadout display;
- breakdown leggibile dei trade-off;
- variant icon/tag;
- experimental non visibili nel ruleset v0.1;
- preview del Push finale;
- preview del danno finale pre-resolution solo se determinabile da informazioni lecite.

---

# 20. Roadmap proposta

## Milestone / Epic: Weapon Variants MVP

### Issue 1 — Variant Data Model

Implementare definizione data-driven con:

- VariantId;
- version;
- range delta;
- cooldown delta;
- Push delta;
- DamageDeltaByBand;
- Status effects;
- ruleset enablement.

### Issue 2 — Damage Band Resolver

Implementare:

```text
BaseAttackDamage
-> Resolve Band
-> Resolve Variant Damage Delta
-> Materialize Attack
```

### Issue 3 — Push Stacking

Regola generale additiva.

Acceptance:

```text
PressureJet Push1 + Impact Push1 = Push2
```

### Issue 4 — Four Playable Variants

Abilitare:

- Precision;
- Impact;
- Overload;
- Suppression.

### Issue 5 — Experimental Variant Gating

MultiTarget e Environmental:

- esistono nel catalogo;
- non selezionabili;
- non compaiono nella v0.1;
- validator impedisce l'abilitazione accidentale.

### Issue 6 — Variant TurnLog Breakdown

Reason/modifier chain visibile.

### Issue 7 — Balance Test Matrix

16 combinazioni.

### Issue 8 — Riva Impact Stress Scenario

Push 2 + environment/objective.

### Issue 9 — Bot Variant Evaluation

Tre policy minime.

### Issue 10 — Decide Overload Cooldown Semantics

Design decision aperta.

---

# 21. Feature Map

Aggiungere/consolidare feature:

```text
Combat
└── Base Attack
    └── Weapon Variants
        ├── Damage Bands
        ├── Precision
        ├── Impact
        ├── Overload
        ├── Suppression
        ├── MultiTarget [Future]
        └── Environmental [Future]
```

Dipendenze:

```text
Weapon Variants
  -> Ability/Attack Definition
  -> Status System
  -> Displacement
  -> Cooldown
  -> Turn Resolver
  -> TurnLog
  -> Data Validation
  -> Bot Scoring
  -> UI Loadout
```

---

# 22. Scenario Map

Aggiungere scenari:

```text
SCN_WeaponVariant_PrecisionRange
SCN_WeaponVariant_ImpactPush
SCN_WeaponVariant_OverloadCooldown
SCN_WeaponVariant_SuppressionSlow

SCN_Riva_Impact_Push2
SCN_Riva_Impact_Hazard
SCN_Riva_Impact_Objective
SCN_Riva_Impact_BlockedPush

SCN_Bastion_VariantViability

SCN_Variant_BotAggressive
SCN_Variant_BotObjective
SCN_Variant_BotPositional
```

Usare snapshot deterministici e golden TurnLog.

---

# 23. Editor Map

Aggiungere task Editor solo dove realmente necessario.

Possibili task:

- creare Data Asset per le 4 variant attive;
- creare asset experimental per MultiTarget/Environmental;
- associare icone placeholder;
- visualizzare Damage Band e delta nel Details Panel;
- debug widget per resolved attack;
- scenario selector per i test variant;
- visualizzazione Push 1 / Push 2 nel debug overlay.

Non spostare nel Blueprint logica competitiva che deve stare nel resolver C++.

---

# 24. Acceptance criteria Weapon Variants MVP

La feature è Done quando:

1. le quattro varianti attive sono data-driven;
2. nessun CharacterId branch è richiesto;
3. la fascia deriva dal BaseDamage;
4. i Push si sommano;
5. Riva + Impact produce Push 2;
6. MultiTarget e Environmental non sono selezionabili in v0.1;
7. TurnLog spiega i modifier;
8. test automatici coprono le 16 combinazioni base;
9. almeno uno scenario golden copre Push 2;
10. bot test può confrontare le varianti;
11. validator rileva configurazioni invalide;
12. stessa fixture produce stesso log/hash;
13. tutto funziona in packaged build secondo la Definition of Done del progetto.

---

# 25. Errori da NON introdurre

- percentuali;
- moltiplicatori;
- eccezioni per CharacterId;
- hard-code Riva/Bastion;
- varianti experimental visibili nel loadout;
- band che cambia durante il match;
- client che decide il valore finale;
- Blueprint come autorità del modifier;
- MultiTarget implementato con target order non deterministico;
- Push stacking dipendente da ordine di TMap/TSet;
- “Ambiental = better hazard” senza parametro concreto;
- auto-fix silenziosi del validator.

---

# 26. Git / issue organization

Commit suggeriti:

```text
docs(combat): consolidate weapon variant affinity and damage-band rules

feat(data): add weapon variant definitions and ruleset gating

feat(combat): resolve damage-band variant modifiers

feat(displacement): support additive push stacking

test(combat): add weapon variant matrix and Riva push2 golden scenarios

feat(ui): expose resolved weapon variant trade-offs

docs(balance): add weapon variant telemetry and bot evaluation matrix
```

Creare epic dedicata se non esiste:

```text
EPIC — Weapon Variants & Horizontal Loadouts
```

Collegarla alle epic esistenti di:

- Combat;
- Character/Abilities;
- Data/Validation;
- UI;
- Bot/AI;
- QA/Determinism.

---

# 27. Decision log

## Locked

- Weapon variants are horizontal choices.
- Natural hero/variant affinities are allowed.
- Universal relative parity is NOT required.
- Damage modifiers use attack damage bands.
- v0.1 playable variants = Precision / Impact / Overload / Suppression.
- MultiTarget / Environmental remain catalogued but disabled/experimental.
- Push effects stack additively.
- Riva + Impact = Push 2.
- No hero-specific branches.

## Provisional

- Damage bands:
  - Low 1–10
  - Medium 11–18
  - High 19+
- Numeric damage deltas per band.
- Default variant assignment:
  - Flux / Suppression
  - Riva / Precision
  - Vektor / Overload
  - Bastion / Impact

## Open

- Exact Overload cooldown semantics.
- Final numeric thresholds of Damage Bands.
- Final per-band damage deltas.
- Final Environmental semantic.
- MultiTarget selection/targeting model.
- Whether all 4 active variants remain viable for every future hero.

---

# 28. Output richiesto a Claude

Dopo il consolidamento Claude deve restituire:

1. file modificati;
2. pagine Wiki create/modificate;
3. PDR modificati;
4. roadmap entries;
5. feature map entries;
6. scenario map entries;
7. editor map entries;
8. epic/issue create o aggiornate;
9. decision log aggiornato;
10. eventuali conflitti trovati nella documentazione;
11. domande che richiedono decisione umana;
12. test aggiunti/proposti;
13. commit suggeriti;
14. eventuali TODO tecnici.

Non reinterpretare decisioni Locked.

Per elementi Provisional/Open mantenere esplicitamente lo stato e non promuoverli a decisioni consolidate senza approvazione.

---

# 29. Priorità immediata

Ordine suggerito:

1. consolidare decision log;
2. aggiornare PDR-07/PDR-09/PDR-10;
3. aggiornare Wiki Weapon Variants + Displacement;
4. aggiornare Roadmap / Feature Map;
5. creare epic + issue;
6. creare scenari Riva Push 2;
7. aggiungere validator;
8. implementare data model;
9. implementare band resolver;
10. implementare TurnLog modifier breakdown;
11. costruire matrice 16 loadout;
12. chiudere decisione Overload.
