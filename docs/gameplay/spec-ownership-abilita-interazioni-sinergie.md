# Spec — Ownership di abilità, interazioni sistemiche e sinergie

> **Stato:** normativa · **Data:** 2026-08-08  
> **Decisione:** ADR-0006 / D-028  
> **Scope:** contenuti, Wiki, scenari, fazioni, cataloghi; nessun nuovo runtime richiesto da questa sola spec.

## 1. Decisione

RefactorTactics separa tre concetti che non devono diventare una sola fonte dati:

1. **Ability/Action Definition** — appartiene a un singolo contenuto/owner (`Hero`, equipaggiamento, oggetto, regola generica).
2. **System Interaction** — appartiene al sistema che la governa (`Wet`, conduttività, Push, cover, LOS, noise, ecc.).
3. **Synergy Example** — è una vista/fixture che mostra come contenuti indipendenti cooperano attraverso le regole sopra.

Una sinergia **non è** una Ability Definition e non introduce per default un nuovo Stable ID competitivo.

## 2. Invarianti

- Nessuna abilità è posseduta da una coppia di personaggi solo perché la coppia la usa bene.
- Nessuna fazione possiede implicitamente un kit condiviso.
- Nessun `FactionSetBonus` o `PairBonus` esiste senza decisione e definizione data-driven esplicite.
- Il core non contiene branch del tipo `if HeroA && HeroB` per produrre il normale payoff di una sinergia.
- Il producer applica stato/superficie/evento; il consumer legge quello stato/superficie/evento senza dipendere dall'identità del producer salvo requisito esplicito dell'abilità.
- Uno scenario dimostra la sinergia usando la pipeline reale; non la implementa.

## 3. Esempio normativo — Wet → Electric

Corretto:

```text
Riva.PressureJet -> Status.Wet
Flux.LinearDischarge -> se Target.HasStatus(Wet), +8
```

Il secondo passaggio dipende da `Wet`, non da `Hero.Riva`. Una futura sorgente di Wet può abilitare lo stesso payoff se le regole lo consentono.

Errato:

```text
if SourceHero == Riva && AttackerHero == Flux:
    BonusDamage += 8
```

## 4. Fazioni

Le fazioni sono metadata di identità/affinità e possono aggregare:

- filosofia;
- linguaggio visivo;
- roster;
- esempi di sinergia;
- scenari dimostrativi.

Non aggregano automaticamente Ability Definition o bonus di composizione.

## 5. Wiki

Le pagine personaggio possiedono la sezione **Abilità**. Le pagine `Sinergie e combinazioni` e `Fazioni` possiedono esempi e navigazione, non numeri competitivi duplicati.

Se una pagina di sinergia deve spiegare un'abilità, la **linka** alla pagina owner; non ne mantiene una seconda copia normativa.

## 6. Scenari

Uno ScenarioId come `Team.Conflux.FluxRiva.ConductiveFlood` può essere specifico della coppia perché la fixture deve mostrare quella cooperazione. Questo non crea una dipendenza nei dati dell'abilità.

```text
Scenario -> Intents -> existing Ability/Action definitions -> systemic rules -> TurnLog
```

## 7. Data model

Preferire riferimenti a Stable ID indipendenti:

```text
HeroDefinition.AbilityIds[]
AbilityDefinition.Effects[]
Effect -> Status/Surface/Event/Graph change
Scenario -> HeroIds + intents
Synergy/Wiki -> references only
```

Non aggiungere `PartnerHeroId`, `RequiredFactionMate`, `ComboAbilityId` o simili al modello base senza un caso di gameplay approvato che non sia esprimibile con le regole sistemiche.

## 8. Test/validator

Questa decisione non richiede un nuovo sistema runtime. Quando si implementano nuove sinergie, i test devono dimostrare che:

- il payoff funziona con la condizione sistemica valida;
- fallisce senza la condizione;
- non dipende accidentalmente dall'identità di un partner;
- lo scenario usa le stesse Ability/Action Definition del gioco.

Per Wet/Electric, mantenere i test esistenti su Wet e propagazione come protezione primaria.

## 9. Naming editoriale

`Combo` resta ammesso come termine descrittivo (es. `Combo Fighter`, `Combo State Machine`, nome variante) quando descrive una sequenza interna o un payoff. Non implica automaticamente una combinazione fra due personaggi.

Per esempi cross-character preferire **Sinergia**, **Interazione sistemica**, **Setup → Payoff**, **Scenario dimostrativo**.
