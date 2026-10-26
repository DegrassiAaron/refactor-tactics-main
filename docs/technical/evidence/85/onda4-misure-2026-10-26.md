# Onda 4 della sequenza di chiusura — le sedute, 2026-10-26

> **Che cosa è**: l'evidenza dei gate dell'Onda 4 ([`sequenza-chiusura-v01-spec-panel-2026-10-09.md`](../../../roadmap/plans/sequenza-chiusura-v01-spec-panel-2026-10-09.md) §4) sul candidate v2
> ([`D-541`](../../../decisions/RT_PDR_00_Decision_Log.md)). L'Onda 3 sta in [`onda3-misure-2026-10-26.md`](onda3-misure-2026-10-26.md). Non è un owner: dove diverge dalla
> [`v0.1-definition-of-done.md`](../../../roadmap/v0.1-definition-of-done.md), vale la DoD.
>
> **Convenzione sui numeri** ([`AGENTS.md`](../../../../AGENTS.md) §14): i numeri sono **esiti di questa tornata**, ciascuno col comando che lo produce.

## 0. Su che cosa si è misurato

| | |
|---|---|
| **Candidate v2** | `7450ead647cfce43ae419218302bdf8bd183cc73` (`D-541`) |
| **`HEAD`** | `db3c10f4dc051fb4c6814670ff8db7f167c6c05f`; `git status --porcelain` → 0 righe prima e dopo ogni run; `git diff --quiet 7450ead64 HEAD -- Source Content ':(exclude)Source/RefactorTactics/Tests' ':(exclude)Source/RefactorTacticsEditor/Private/Tests'` → `OK` ([`D-542`](../../../decisions/RT_PDR_00_Decision_Log.md)) |
| **Binario** | DLL dell'Editor `UnrealEditor-RefactorTactics.dll` `sha256` `43A0842A6FBD13D9…`, quella della suite di `G2` (Onda 3); invariata fra l'inizio e la fine di ciascuna run |
| **Macchina** | Unreal e Live Coding spenti prima di ogni lancio (`Get-CimInstance Win32_Process`). Nessuna misura di tempo in questa sezione: l'esito di una partita non dipende dal carico di fondo |

## 1. `G10` — partita 2v2 autobattle

`UnrealEditor-Cmd.exe <uproject> /Game/RT/Maps/Dev/L_HexArena/L_HexArena -game -RTAutobattle -useFixedTimeStep -fps=60 -unattended -nopause -nosplash -nullrhi -NoLiveCoding -abslog="…"`, due volte di seguito. In entrambe il log dichiara la fine dopo 31 s e 41 s; il processo non esce da solo (`Fine partita al round …` e poi il game loop), quindi l'ho terminato io dopo aver verificato sulla `CommandLine` che portasse il mio `-abslog`. Nessun processo Unreal residuo.

**Esito** (riga testuale): *«Partita finita: Vince il team 0 (blu) - per eliminazione (round 9/12, obiettivo 0-2, formato Format.Skirmish2v2)»* e *«Fine partita al round 9: ERTMatchOutcome::Team0Wins»*. Apertura: *«Board 2v2 esagonale avviata su 64 celle con 4 eroi»*.

**Non degenere**, clausola per clausola:

- *danni inflitti*: 26 righe con ` danni`; due eliminazioni (*«Eliminata: BP_Unit_Ivrin_C_0 (team 1)»*, *«Eliminata: BP_Unit_Branth_C_0 (team 1)»*);
- *reaction risolta*: `Hero.Aevik.ReactiveCapacitor` infligge 5 danni (p20) due volte (`(q=-1,r=1,L=0) -> (q=-3,r=0,L=0)` e `(q=-1,r=-2,L=0) -> (q=-1,r=-3,L=0)`, la seconda con *eliminata*);
- *progresso obiettivo mosso*: `obiettivo 0-2`.

**Determinismo**: il flusso `LogRT` senza timestamp è identico fra le due run (407 righe, stesso `sha256` `db535f47768c3d79…` del testo). **Differenza dal 2026-10-02** (`Team1Wins`, `RoundLimit`, round 12): attesa, perché il codice è cambiato (`git rev-list --count 95eddfd37..7450ead64 -- Source` → 639 commit).

**Riserva che resta**: nessuna riga del flusso porta `L=1` (`grep -aoE '[( ,]L=?[0-9]+[),]'` → 208 `L=0` e 31 `L0`, nessun'altra classe): la mappa è multilivello, la partita no. Le due righe `Caduta:` sono cadute di dislivello nella classe `L0`. **La riserva sulle reaction cade**: una è stata risolta con effetto.

Log: [`../g10/partita-2v2-autobattle-candidate-v2-2026-10-26.log`](../g10/partita-2v2-autobattle-candidate-v2-2026-10-26.log) (il solo flusso `LogRT`, senza prefissi di timestamp, come i due precedenti).

## 2. `G13` e `G16`

**NOT RUN.** Sono sedute con una persona (`G13` presidiata sul pacchetto Development; `G16` nell'Editor, con passi percettivi). Questa sezione si compila quando sono eseguite.

## 3. `G9`

**NOT RUN.** Il subset `RELEASE-V01` delle verifiche manuali (PIE) richiede una persona.
