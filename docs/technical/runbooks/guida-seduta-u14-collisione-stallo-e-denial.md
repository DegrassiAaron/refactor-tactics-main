# Guida di seduta — U14 · `PIE-V01-COLL`: stallo ripetuto e denial nel choke

> `CURRENT` · **Creata**: 2026-09-04 · **Seduta**: `U14` in
> [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml) · **Voce**: `PIE-V01-COLL` ·
> **Epic**: [#2276](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2276)
> **Owner degli esiti**: [`test-manuali-pie.md`](../test-manuali-pie.md) — questa guida dice **come**
> osservare, non **cosa è risultato**.
>
> ⛔ **Copre UNA delle undici voci di U14.** Le altre dieci — `ROUGH` `DASHCOVER` `PUSH` `ELEC`
> `FIREWATER` `LOWCOVER` `INTERCEPT` `FF` `FALLBACK` `DOOR` — hanno il loro allestimento e non sono qui.
> Questa guida esiste perché `PIE-V01-COLL` ha preso due clausole nuove il 2026-09-04 e **vuole un
> allestimento che il resto della seduta non ha**.

## Perché serve un allestimento proprio

L'arena ambientale del blocco 5 **non ha un varco obbligato**. Senza un choke, «lo stallo si ripete» e «il
denial nega il passaggio» non sono osservabili: due unità che si contendono una cella in campo aperto si
aggirano al turno dopo, e non c'è niente da guardare.

## 1. Allestimento — non si costruisce, si sceglie

🔑 **Il choke è già un asset versionato.** `Movement.CollisionChoke`
([`Scenarios/Movement/CollisionChoke.json`](../../../Scenarios/Movement/CollisionChoke.json)) porta la board
esatta e i quattro turni:

| | |
|---|---|
| barriera | colonna `q=0` bloccante il movimento, **tranne** `(0,0,0)` — varco unico verificato per BFS |
| contendenti | `A1` a `(-1,0,0)` e `B1` a `(1,0,0)`, **equidistanti** dal varco a un passo |
| T1 · T2 | entrambi puntano il varco: **la contesa si ripete** |
| T3 | `A1` cede, `B1` entra |
| T4 | `A1` prova, `B1` **sta fermo**: è il denial |

Non c'è niente da dipingere e nessuna coordinata da digitare.

### La via

1. Details del **GameMode** → categoria `RefactorTactics|Test` → **`Scenario To Run`**.
2. Scegli **`Movement.CollisionChoke`** dal **menu a tendina** — si sceglie, non si scrive.
3. Alza **`ScenarioTurnPauseSeconds`** (default `1.5`): quattro turni scorrono in fretta, e le due clausole
   chiedono di leggere il log **mentre** accade. ⚠️ Se lo alzi, **annotalo nell'esito**: un verdetto preso a
   velocità diversa è un verdetto su un'altra cosa.
4. **Play** → osserva i quattro turni **senza fermarti** → **Stop**.

⚠️ La cvar `rt.Test.Scenario` fa lo stesso e **prevale** sulla proprietà. Se la usi, **svuotala dopo**.
⚠️ Le unità saranno **cilindri** (`ARTUnit` nudi): per queste tre clausole va benissimo — anzi, per la
sovrapposizione è la resa più leggibile che ci sia.

## 2. Le tre clausole, e ciò che le falsifica

### 2.1 Non si sovrappongono — *la clausola storica*

Al **T1** e al **T2** i due contendenti puntano la stessa cella. Guarda il varco `(0,0,0)`.

- ✅ **passa** se restano **ciascuno dalla propria parte** e nessuno dei due entra;
- ❌ **cade** se per un fotogramma i due cilindri **occupano la stessa cella**, anche se poi si separano.

### 2.2 🔴 Stallo ripetuto — *nuova*

La stessa contesa avviene **due volte di fila**. Leggi il **combat log** a **entrambi** i turni.

- ✅ **passa** se il reason di cella contesa compare al **T1 e al T2**, con le coordinate;
- ❌ **cade** se compare **solo la prima volta**: alla seconda, «fermo perché conteso» diventa
  indistinguibile da «fermo perché ho scelto di stare fermo», ed è precisamente il difetto che la clausola
  cerca.

⚠️ **Conta le righe, non l'impressione.** Headless sono **4** esiti `BlockedContested` (due unità × due
turni): se a schermo il log ne mostra due, metà dell'informazione non arriva al giocatore.

### 2.3 🔴 Denial — *nuova, ed è quella che pesa*

Al **T4** `B1` occupa il varco **stando fermo** e `A1` prova a entrare.

- ✅ **passa** se il giocatore riceve **un segnale qualsiasi** — al click, al commit o nel log — che dica che
  il passaggio è negato;
- ❌ **cade** se non succede **niente**: nessuna riga, nessun avviso, il piano semplicemente non parte.

🔴 **Headless è già misurato che il TurnLog è muto.** `Movement.CollisionChoke` asserisce
`Move`/`BlockedByUnit` = **0**, perché il piano è rifiutato in **pianificazione** (`FindPathForUnit` →
`NoPath`) e ciò che non entra nel resolver non produce esito. Questa clausola chiede se anche **a schermo**
*«ho provato e me l'hanno negato»* sia indistinguibile da *«non ho provato»*.

⛔ **Se cade, il difetto NON è di questa voce**: è
[#79](https://github.com/DegrassiAaron/refactor-tactics-main/issues/79), la cui DoD chiede già che *«i
fallback applicati siano espliciti (percorso bloccato → fermo)»*. Registralo qui e **rimandalo lì**.

## 3. Cosa NON rifare

Il gioco **non si rompe**, ed è già provato: `Movement.CollisionChoke` gira `PASS (6/6)` a ogni suite, e
`RefactorTactics.HexMove.ContestedCellStopsBoth` pinna i due esiti di contesa. ⛔ Questa seduta **non**
verifica che la contesa funzioni: verifica che il giocatore **se ne accorga**. Se ti trovi a controllare che
le unità si fermino, stai rifacendo un test che è già verde.

## 4. Come registrare l'esito

L'esito va in [`test-manuali-pie.md`](../test-manuali-pie.md), nella colonna **Stato** di `PIE-V01-COLL` —
**non qui**, e non in `editor-sessions.yaml` (regola `R-6`).

Registra **le tre clausole separatamente**: un verdetto aggregato non dice quale metà ha ceduto, ed è
l'errore che `PIE-HEX-LAYER-FOCUS` documenta su sé stessa — *«le clausole sono state guardate insieme e
riferite con un solo ok»*. Per ciascuna: ✅ o ❌, la data, e per un ❌ **cosa hai visto**, non cosa manca.

⚠️ E se hai alzato `ScenarioTurnPauseSeconds`, scrivi a quale valore.
