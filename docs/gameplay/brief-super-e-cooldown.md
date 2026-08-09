# Brief — Super Action e cooldown a strati

> `CURRENT` · **Stato**: brief di forma · **Data**: 2026-08-09 · **Release**: 📅 **post-v0.1**, senza epic
> **Feature**: `RT-FEAT-ACTION-SUPERS` (v0.2, `IMPLEMENTING`) · **Dipende da**: `RT-FEAT-ACTION-COOLDOWNS`
> **Origine**: `RT_Characters_Roster_Master_Consolidation_v0.1.md` §14–§16 — input, non autorità; triage in
> [`../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md`](../roadmap/plans/consolidamento-chat-openai-triage-2026-08-09.md)
> **Autorità**: subordinato al canone, ad [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) per le fasi
> e ad [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md) per l'ownership dei kit.
> **Nessuna implementazione, nessun numero, nessuna Super assegnata a nessun eroe.**

---

## 1. Perché esiste, e cosa c'era già

`RT-FEAT-ACTION-SUPERS` è nel registry come `IMPLEMENTING`, e la sua nota lo dice con precisione:

> «Esiste la risorsa (energia, `UltimateReady`) ma non una categoria «super» con regole proprie.»

Verificato: nel codice c'è **una funzione**, `URTCombatLibrary::IsUltimateReady(Energy, Max)` — un predicato
puro su una risorsa, con un test (`Combat.UltimateReadyAtFullEnergy`). Nient'altro. Nessuna scheda
personaggio, né in [`../characters/v0.1/`](../characters/v0.1) né in [`../characters/v0.2/`](../characters/v0.2),
descrive una Super.

Il suo `owner_specs` puntava a [`../balance/RT_HeroCatalog_v0.1.md`](../balance/RT_HeroCatalog_v0.1.md), che di
Super non parla: una riga di registry con un owner che non possiede. Questo brief è l'owner mancante — della
**forma**, non del contenuto.

**Non apre lavoro.** Esiste perché la prima persona che costruirà una Super troverà scritto *come*, invece di
inventarlo; e perché la scorciatoia sbagliata, in questo caso, è più comoda di quella giusta.

---

## 2. La regola che vale più di tutte le altre: nessun secondo motore

> Una Super è una **Ability Definition** con metadata, requisiti, costi, cooldown e commit policy.
> Si risolve col resolver che esiste. **Non si costruisce un sistema chiamato `Ultimate`.**

Il progetto ha già rifiutato la stessa scorciatoia due volte, e vale la pena citarle perché sono il
precedente, non un'analogia:

- le **reazioni componibili** — «leggendo solo questo documento si potrebbe dedurre che E14 aggiunga un
  secondo motore: **non lo fa**» ([`spec-reazioni-componibili-cp55.md`](spec-reazioni-componibili-cp55.md),
  banner in testa);
- l'**equipaggiamento** — «non serve un secondo motore di reazioni, E5 è già il caso»
  ([`../balance/RT_EquipmentCatalog_v0.1.md`](../balance/RT_EquipmentCatalog_v0.1.md)).

La ragione è sempre la stessa: due motori che risolvono azioni divergono alla prima regola nuova, e il
determinismo è una proprietà del **percorso unico**, non di ciascun percorso preso separatamente.

Forma attesa, senza tipi nuovi:

```text
Ability Definition
  + tag/metadata «super»
  + requisiti (i gate di §3)
  + costi e cooldown
  + commit policy
  = Super Action
```

---

## 3. Che cosa distingue una Super da un'abilità forte: i gate

Una Super non è «un'abilità grossa con cooldown lungo» — quella è già esprimibile oggi, e non richiede
niente. È il **massimo impegno della meccanica firma** del personaggio, e deve dichiarare **almeno un gate**
fra:

```text
Resource      una risorsa accumulata
State         uno stato del personaggio o del bersaglio
Environment   una condizione del terreno o della superficie
Geometry      una configurazione spaziale
Prediction    una scommessa dichiarata in Planning
Setup         un'azione precedente, propria o di squadra
```

più una dichiarazione di **rischio o recovery**: cosa resta in mano a chi la usa e sbaglia.

**Il criterio operativo**: una Super senza gate è un'abilità con un numero più alto. Se togliendo il gate la
Super resta giocabile allo stesso modo, il gate era decorativo — e quella non è una Super.

> Vale la stessa logica del campo `Misplay / Failure State` ([D-032](../decisions/RT_PDR_00_Decision_Log.md)):
> non è compilabile con «fa meno danno». Deve nominare la decisione andata storta e il suo costo.

---

## 4. Cooldown a strati

Il modello v0.1 è già corretto e **non va sostituito**: `CooldownTurns` è espresso in **turni completi**
([`../balance/RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md)), quindi avanza nella logica del
turno e non con la durata di un'animazione. Questo brief lo estende, non lo riscrive.

Da supportare quando servirà, in quest'ordine di necessità:

| Strato | Che cosa aggiunge |
|---|---|
| cooldown individuale | esiste già |
| **shared cooldown group** | più azioni che condividono un contatore |
| **charges** | *N* usi prima dell'attesa |
| **recovery** | un costo che si paga *dopo* l'uso, distinto dall'attesa |

Tre vincoli, e sono quelli che si rompono per primi:

1. **Niente global cooldown obbligatorio.** Un contatore che blocca tutto è il modo più rapido di rendere
   ogni personaggio uguale agli altri: cancella proprio la differenza che i cooldown esistono per esprimere.
2. **I cooldown avanzano in turni logici**, nel Cleanup. Mai col tempo reale, mai con la durata di un
   montage — è la stessa regola per cui il sequencing competitivo non passa da `Delay` o `DeltaTime`.
3. **Dopo il commit, un whiff può consumare il cooldown lo stesso** — ma solo se la definition lo
   **dichiara**, e il TurnLog deve spiegare il consumo e il motivo. È l'estensione naturale di
   [D-016](../decisions/RT_PDR_00_Decision_Log.md), dove il whiff di una predictive action «risolve lo
   stesso» come fallback dichiarato.

---

## 5. Una Super «lenta» non è un'animazione lunga

Se una Super deve sembrare pesante, il peso si esprime **nelle fasi**, non nella durata della presentazione:
telegrafo in `Prep`, il campo che può cambiare in `Dash`, il payoff nel `Blast`, l'eventuale limitazione nel
`Move`, il recovery nel `Cleanup`.

L'ordine è quello di [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) e non cambia. Il risultato logico
non dipende dal tempo reale: la presentazione può rallentare, la simulazione no.

---

## 6. Che cosa questo brief **non** decide

- **Nessuna Super è assegnata a nessun eroe**, né in v0.1 né in v0.2. Le schede di
  [`../characters/v0.2/`](../characters/v0.2) restano come sono.
- **Nessun numero**: costi, cooldown, cariche e soglie appartengono ai cataloghi.
- **Nessuna epic.** `RT-FEAT-ACTION-SUPERS` resta v0.2 senza epic assegnata, ed è corretto così: E35 è il
  roster a 8, non le Super.
- **Non promuove `IsUltimateReady` a categoria.** Resta il predicato su una risorsa che è oggi; se la Super
  userà quella risorsa, la userà come requisito dichiarato, non come identità.
- **Non decide se la v0.2 avrà Super.** Decide che *se* le avrà, non avranno un motore proprio.

---

## 7. Che cosa questo brief **non** possiede

| Tema | Owner |
|---|---|
| Fasi e priorità della risoluzione | [ADR-0003](../decisions/adr-0003-modello-azioni-v01.md) |
| Ownership di abilità e sinergie | [ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md) · [`spec-ownership-abilita-interazioni-sinergie.md`](spec-ownership-abilita-interazioni-sinergie.md) |
| Cooldown della v0.1, valori a catalogo | [`../balance/RT_ActionCatalog_v0.1.md`](../balance/RT_ActionCatalog_v0.1.md) |
| Roster a 8 e fazioni v0.2 | **E35** di [`../roadmap/roadmap-post-v0.1.md`](../roadmap/roadmap-post-v0.1.md) |
| Stati e trasformazioni del personaggio | [`brief-stati-personaggio-e-trasformazioni.md`](brief-stati-personaggio-e-trasformazioni.md) (E34) |
| Stato di avanzamento | [`../roadmap/feature-registry.yaml`](../roadmap/feature-registry.yaml) |
