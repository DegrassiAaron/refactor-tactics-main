# Triage — handoff «Status, Buff/Debuff, Control, Brace & Overwatch»

> `CURRENT` · **Data**: 2026-08-10 · **Owner di una sola domanda**: delle 69 sezioni del sorgente, quali
> aggiungono qualcosa e quali descrivono lavoro già fatto o già in volo.
>
> **Sorgente**: [`2026-08-10-status-control-brace-overwatch.md`](../../archive/src/handoff/2026-08-10-status-control-brace-overwatch.md)
> **Fratelli**: è il **quarto** sorgente del 2026-08-10 sullo stesso perimetro. I primi tre sono in
> [#390](https://github.com/DegrassiAaron/refactor-tactics-main/pull/390),
> [#394](https://github.com/DegrassiAaron/refactor-tactics-main/pull/394),
> [#395](https://github.com/DegrassiAaron/refactor-tactics-main/issues/395) e
> [#397](https://github.com/DegrassiAaron/refactor-tactics-main/pull/397).

## 0. Il risultato in tre righe

**Il sorgente si divide in due metà con destini opposti.**

- **§39–§51 — Brace e Overwatch**: quasi interamente **già deciso o in volo** nella catena di PR aperte
  #390 → #394 → #397. Contiene inoltre **tre nomi che il repository ha già assegnato ad altro**, uno dei
  quali è stato **esplicitamente respinto** ieri.
- **§2–§28, §52–§54 — Status e Control**: è la parte con contenuto reale. Il repository ha **undici status
  implementati** ma **nessun framework** che li governi: niente categorie, niente severity, niente primitive
  riusabili, niente stacking policy dichiarata, niente resistenza/immunità/cleanse.

**Non risolve `MAP-1`.** Non nomina mai clearance, footprint o calpestabilità.

## 1. `MAP-1` resta aperta

Verificato con `grep -icE "clearance|footprint|standab|calpestab|anchor|metri"` sul sorgente: **18
occorrenze, zero pertinenti** — otto sono `Anchor`, il nome del profilo Brace di Bastion, e le altre sono
`metri` dentro «para**metri**».

Il sorgente non tocca la geometria della cella. `MAP-1` — *quale frazione di cella occupa il footprint
standard* — resta dove sta, in [`OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).

## 2. Cosa esiste già: undici status, zero framework

Misurato su `Source/` (`TAG_Status_*`):

```
Braced · Burning · Electrified · Exposed · Guarded · Marked · Obscured · Reveal · Root · Slow · Wet
```

Owner documentale: [`spec-stati-temporanei-cp82.md`](../../gameplay/spec-stati-temporanei-cp82.md) — E8,
CP 8.2, con sei decisioni già prese (`D1`–`D6`) su durate sentinella, consumo di `Marked`, `Electrified`
senza consumatore, `Obscured` fuori dal targeting, erosione dello scudo da `Burning`, durata di `Wet`.

**Confronto con il set v0.1 che il sorgente propone** (§7):

| Sorgente | Repository |
|---|---|
| `Wet` · `Marked` · `Exposed` · `Burning` | ✅ esistono |
| Prepared Reaction States | ✅ `Braced`, `Guarded` — manca `Overwatch.Armed` |
| **`Suppressed`** | ❌ **assente** da codice e canone |
| **`Dazed`** | ❌ **assente** da codice e canone |

Quindi il sorgente aggiunge **due status** e — molto più importante — **il livello che oggi non esiste**:

| §  | Cosa manca al repository |
|---|---|
| §3 | Categoria (`Modifier/Control/Environment/Stance/Reaction/Special`) e **polarità** separata |
| §4 | Le sei primitive del resolver: `MODIFY · DEGRADE · RESTRICT · INTERRUPT · CONVERT · CONSUME` |
| §5–§6 | Severity `C0`–`C3` e la regola anti-stun-lock |
| §14–§15 | Stacking policy ed expiration dichiarate **sul dato**, non nel codice |
| §16–§20 | Pipeline di applicazione, successo parziale, danno e status risolti separatamente |
| §21–§23 | Resistance (degrada), Immunity (nega), Cleanse per categoria |
| §26–§27 | Reapply policy e prevenzione dei loop di conversione |
| §28 | Reason code e eventi TurnLog degli status |
| §53 | Validator/lint: 16 regole |

**È questa la parte da consolidare.** Oggi ogni status è cablato dove serve: funziona, ed è esattamente il
motivo per cui il dodicesimo costerà come i primi undici messi insieme.

## 3. Tre nomi già assegnati — e uno già respinto

> ⚠️ **`Reposition` è stato respinto ieri.** [PR #397](https://github.com/DegrassiAaron/refactor-tactics-main/pull/397)
> (`D-067`) decide che il ripiegamento dopo l'Overwatch si chiama **`Withdraw`**, perché `Action.Reposition`
> **è un'azione viva**: scatto lineare di 2 celle in macro-fase Dash, concesso anche da `Riva.FlowReaction` e
> `Vektor.Feint`. Il sorgente usa `Reposition` in §43, §44, §57 e §60.

| Il sorgente propone | Il repository ha già | Esito |
|---|---|---|
| §41 — Brace profile **`Flow`** per Riva | **`Riva.FlowReaction`**: `Reposition 1` dopo un attacco subìto, fase Preparation, priorità 36 | **Collisione.** Due «Flow» di Riva in due fasi diverse si pagano a ogni lettura del TurnLog |
| §41 — Brace profile **`Deflection`** per Vektor | **`Vektor.Deflection`**: costruita su `Action.Deflect`, cioè **riduzione del danno** | **Collisione semantica invertita.** Il sorgente lo vuole anti-displacement; nel gioco è anti-danno |
| §43–§44 — **`Reposition`** | `Action.Reposition` (Dash, 2 celle) · e `D-067` ha scelto **`Withdraw`** | **Respinto ieri.** Non si riapre |

Anche i nomi degli status divergono: il sorgente scrive `Rooted`, `Slowed`, `Shocked`; il codice ha
**`Root`**, **`Slow`**, **`Electrified`**. Non è una sfumatura: sono `FGameplayTag` con ID stabile, e un
documento che li chiama in un altro modo produce `grep` vuoti a chi cerca.

## 4. La metà già in volo — §39-§51

Non c'è niente da consolidare qui, e il motivo è che tre sorgenti fratelli sono arrivati prima.

| § | Tema | Dove vive già |
|---|---|---|
| §43–§44 | Lifecycle `Watch → ripiegamento` | **#394** (triage, chiude `BAS-5`) e **#397** (`D-067`: costo, nome, budget **2 MP**) |
| §45 | Trigger su transizione di Move | #394 |
| §46 | `HOLD` e timeout | Già canonico: ADR-0004, `HOLD` non consuma la charge |
| §47 | Target simultanei → una sola opportunity | Già canonico (ADR-0004) e già registrato come scenario da scrivere in **#395** |
| §49 | `FIRE` non libera il movimento in anticipo | #394 |
| §51 | Reaction Clash — «non implementare subito» | Già così: `RT-FEAT-REACTION-CLASH`, e la matrice 3×3 esiste come spec eseguibile (`Spec.Clash.*`) |

> 💡 **La §51 dice una cosa giusta con un anno di ritardo**: *«prima validare i Single Responder»*. Il
> repository lo ha già fatto al contrario — il Clash ha quattro scenari-specifica scritti e i profili di
> Brace no. Non è un errore da correggere: è che `Spec.Clash.*` nasce da `D-047`/`D-049`, che sono decisioni
> **successive** a questo sorgente.

## 5. Conflitto con una regola di progetto

§1 chiede che il sistema sia *«compatibile con Gameplay Tags e GAS come layer di abilità/effetti»*.

**`CLAUDE.md` §2: No GAS nella v0.1** — il modello è `URTActionData` / `URTHeroData` / `URTEquipmentData`.

Non è fatale: i `FGameplayTag` **sono già usati** (`TAG_Status_*`) e non implicano GAS. Ma la frase va letta
per quello che è — una compatibilità **futura**, non un requisito della v0.1 — altrimenti autorizza per
sbaglio una dipendenza che il progetto ha escluso.

## 6. Cosa entra

Una feature nuova nel registry e un'epic, **non** l'implementazione: il sorgente stesso (§66) dice di non
mischiare C++ e consolidamento documentale.

- **`RT-FEAT-STATUS-FRAMEWORK`** — categorie, severity, primitive, stacking, durate, resistance/immunity/
  cleanse, reason code, validator. Stato **`DESIGNED`**: la direzione è scritta, la spec owner no.
- I **due status nuovi** (`Suppressed`, `Dazed`) restano *dentro* quella feature: non hanno senso senza le
  primitive che li esprimono — `Suppressed` **è** un `DEGRADE`, `Dazed` **è** un `INTERRUPT` sulla scelta
  manuale.

## 7. Cosa resta aperto

| ID | Domanda |
|---|---|
| `STA-1` | Le sei primitive sono un **enum** o emergono dai dati? Il repository ha già respinto un enum di policy per l'Overwatch (*seconda verità* accanto ad `AllowedResponses`): la stessa obiezione si applica qui, e va decisa **prima** di scrivere la spec |
| `STA-2` | La severity `C0`–`C3` è un **dato dichiarato** o si deriva da quante capability l'effetto tocca? Derivarla evita che qualcuno dichiari `C1` su un effetto che ne toglie tre |
| `STA-3` | `Suppressed` e `Dazed` entrano nella **v0.1** o nel framework e basta? Il sorgente §7 li mette nel set ridotto del vertical slice, ma nessuno dei quattro kit li produce oggi |

## 8. Il difetto di metodo, quarta occorrenza

È il quarto sorgente consecutivo con la stessa forma, e ormai il rapporto è misurabile: **la metà che
riguarda Brace e Overwatch era già decisa, quella sugli status no.**

Il segnale che li distingue non è la lunghezza né il tono — §43 e §4 sono scritte con la stessa sicurezza.
È che **§4 nomina un meccanismo che il repository non sa esprimere** (una primitiva riusabile fra status),
mentre §43 nomina un meccanismo che il repository ha appena finito di nominare da solo, con un nome migliore.

> 🧭 **Corollario operativo**: quando un sorgente propone un **nome**, cercarlo in `Source/` prima di
> tutto il resto. Tre dei quattro nomi proposti qui erano già presi, e il quarto era stato respinto il
> giorno prima con la motivazione scritta.
