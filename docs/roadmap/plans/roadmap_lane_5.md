# Lane 5 — Replay / Audit

> `SNAPSHOT` · **Data**: 2026-08-12 · **HEAD**: `59fa6f8a` (riallineato al merge)
> **Cosa è**: la sequenza di lavoro della lane *Replay / Audit*, letta sul backlog **già aperto**.
> **Cosa non è**: una fonte di stato, né l'owner del replay — quello sono
> [ADR-0009](../../decisions/adr-0009-replay-logico-canonico.md), `D-077`, `D-078`, `D-083`.
> In caso di divergenza **vince la fonte**.
> **Fonte comune delle lane**: [`roadmap-lane-index.md`](roadmap-lane-index.md).

---

## Perimetro

La traccia e ciò che se ne ricava: TurnLog come artefatto, archivio replay, hash, corpus golden,
audit, QA di release. **Non produce esiti**: `D-078` separa il **Player** (autorità = la traccia,
non calcola nulla) dal **Verifier** (autorità = il resolver, ri-simula e produce un verdetto).

⚠️ **Non è terra vergine**: `Source/RefactorTactics/Replay/` ha manifest, recorder e seek, con **16
test** verdi, e `ERTReplayManifestVersion::Initial = 1` è congelato.

---

## La catena di release è pronta in testa

```text
#83 ──> #84 ──┬──> #85
CP 12.3 CP 12.4│    CP 12.5
        ↑      │
     [lane 1]  │
       #41     │
               │
[lane 4] #82 ──┘
```

| Issue | Checkpoint | Stato | Dipende da | Prio |
|---|---|---|---|:--:|
| `#83` | CP 12.3 — Suite automatica completa | 🟢 **pronta** | `#81` ✅ | **P0** |
| `#84` | CP 12.4 — KPI misurati e registrati | ⏳ bloccata | `#83` · *(i numeri da `#41`, lane 1)* | **P0** |
| `#85` | CP 12.5 — Release interna v0.1 | ⏳ bloccata | `#82` (lane 4) · `#84` | **P0** |

⚠️ **`#84` non richiede di centrare i target, richiede di avere i numeri** — è scritto nel gate
`G11`. Un valore fuori budget **registrato** chiude il checkpoint; un valore mancante no.

---

## La catena showcase è bloccata a tre livelli

```text
[lane 2] #501 ──> #163 ──> #512 ──┐
        decisione  CP 14.3  CP 15.3│
                                   ├──> #170 ──> #171
[lane 2] #74 ──> #75 ──────────────┘    CP 15.4   CP 15.5
        CP 10.1   CP 10.2
```

| Issue | Checkpoint | Stato | Dipende da | Prio |
|---|---|---|---|:--:|
| `#170` | CP 15.4 — Golden replay degli 8 turni | ⏳ **bloccata a tre livelli** | CP 15.3 (`#512`) · `#66` ✅ · `#75` | P1 |
| `#171` | CP 15.5 — Presentazione e playtest di leggibilità | ⏳ bloccata | `#170` | P1 |

**DoD di `#170`, le due voci che vincolano il resto della lane:**

- i file golden vivono con quelli del **CP 12.6** (`Source/RefactorTactics/Tests/Golden/`) — stesso
  meccanismo, **non un secondo sistema**;
- **rigenerazione solo con flag esplicito, mai in automatico**, e la PR che rigenera dichiara
  *perché* l'esito è cambiato.

---

## L'unico lavoro replay sbloccato oggi

### `#625` · Il danno da hazard è fuori dalla traccia canonica 🟢 **pronta** · **P2**

**Trovato il 2026-08-12** applicando il gate `replay_representable`; cercato su GitHub, non esisteva,
**aperto lo stesso giorno**.

Nella Cleanup, un'unità con `Status.Burning` subisce `BurningCleanupDamage`, perde HP e **può
morire**. Il fatto viene registrato con `ARTTurnManager::AddLogEvent` — `UE_LOG` più il buffer
circolare `RecentEvents`, troncato a `MaxLogLines` — e **non** passa da `AppendLogEntry`. Quindi
**non esiste nessuna voce canonica**: chi riproduce vede gli HP scendere, o un'unità sparire, senza
un evento che lo spieghi, e `CompareSerializedTraces` non può nominare quel punto come divergenza.

Il codice lo ammette: *«l'eliminazione da hazard non ha un beat di playback … la nasconde il
catch-all di `ConcludeTurn`»*.

**Perché sta in questa lane e perché adesso**: alimenta `#170`, che chiede un golden replay
«riproducibile **e spiegabile**». Una morte per fuoco senza evento non è spiegabile.

⚠️ **Correzione del 2026-08-12.** La prima stesura di questa sezione diceva che la correzione
«cambia gli hash, quindi il corpus golden va rigenerato». **Misurato, è falso**: il corpus pinnato
sono **due** file `.rttl` — `Movement.Basic` e `Movement.Collision` — e **nessuno dei due tocca il
fuoco*; `RefactorTactics.ShowcaseRelay.*` calcola gli hash **dentro la stessa esecuzione** e
confronta run contro run; nessun test pinna un conteggio di voci. **Oggi non diventerebbe rosso
niente.**

E questo **inverte l'argomento**: non è un rischio da gestire, è una **finestra aperta che si
chiude**. È lo stesso ragionamento di `D-084` — *«la finestra per farlo era adesso, prima che un
archivio con checksum finisse su disco»*. Dopo `#170`, che pinna otto turni, la stessa correzione
costerà una rigenerazione motivata invece che nulla.

⚠️ **Stessa famiglia di `#570`** (lane 1): anche lì una correzione ambientale può aggiungere una
voce. Se un giorno servirà rigenerare, prese insieme si rigenera **una volta sola**.

**Stato**: 🟢 **aperta come `#625`** il 2026-08-12 · `v0.1` · P2

---

## Il backlog di fondo: 32 gate `replay_representable: todo`

Dal 2026-08-12 il Feature Registry ha il gate `replay_representable`
([`../feature-registry.md`](../feature-registry.md) §4, regola 7). Distribuzione attuale:
**14 `done` · 32 `todo` · 45 `na`**.

I 32 `todo` **non sono 32 issue**: sono le feature per cui nessuno ha ancora dimostrato che l'evento
sopravviva alla traccia. Due hanno già un indirizzo preciso — `RT-FEAT-ENV-STATUS` e
`RT-FEAT-ENV-FIRE`, cioè esattamente la proposta qui sopra.

```bash
# le feature ancora da dimostrare
python -c "import yaml;d=yaml.safe_load(open('docs/roadmap/feature-registry.yaml',encoding='utf-8'));\
print('\n'.join(f['feature_id'] for f in d['features'] if f['gates']['replay_representable']=='todo'))"
```

---

## Ordine consigliato

1. **`#83`** — P0, pronta, e apre l'intera catena di release.
2. **`#625`** — l'unico lavoro replay vero sbloccato, e la finestra per farlo a costo zero è **ora**.
3. **`#84`** appena `#41` (lane 1) ha prodotto i numeri.
4. `#85` quando `#82` (lane 4) e `#84` sono chiuse.
5. `#170` non prima che la lane 2 abbia sciolto `#501`.

## Dipendenze fuori lane

| Da | Verso | Natura |
|---|---|---|
| `#41` (lane 1) | `#84` | i KPI da registrare |
| `#82` (lane 4) | `#85` | la matrice manuale è metà della release |
| `#512`, `#75` (lane 2) | `#170` | il golden replay li consuma |
| `#570` (lane 1) | corpus golden | stessa rigenerazione della proposta hazard |
