# `PIE-HUD-REGISTRO-ATTIVA`: seduta pilotata dopo #4052, 2026-11-11

Questa seduta rimisura a schermo la voce `PIE-HUD-REGISTRO-ATTIVA` dopo [#4052](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4052), che implementa [`D-603`](../../../decisions/RT_PDR_00_Decision_Log.md). Le due domande sono queste:
- **Istante:** la riga d'attivazione compare nello stesso istante del fumetto?
- **Posto:** la riga di un gadget la cui unità non ha una riga d'esito sta in fondo alla sua fase?

La seduta ripete anche gli altri criteri, per vedere se qualcosa è regredito. ⚠️ Il criterio (d) misura una regola che, durante la stesura di questo documento, l'autore ha cambiato: con [`D-604`](../../../decisions/RT_PDR_00_Decision_Log.md) gridano tutte le azioni tranne `Action.Move`, `Action.Sprint`, `Action.Withdraw` e `Action.Interact`, quindi l'attacco base, l'azione generica e il gadget avranno anche il fumetto. Il metodo è quello della [seduta del 2026-11-10](../4033/seduta-pilotata-2026-11-10.md): una finestra `-game` pilotata da fuori, con input sintetico. Misura la meccanica dei criteri, non l'occhio dell'autore.

⚠️ **Chi misura ha scritto il codice di #4052.** Non è un verdetto indipendente. La misura avviene dopo il merge, su un commit dichiarato, e il giudizio della voce resta all'autore.

## Condizioni

- **Commit:** `main` = `a2d4a63ab` (PR [#4055](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4055)). Il clone principale è stato portato a quel commit in fast-forward, e l'Editor è stato compilato alle 07:20:54 UTC. La DLL `UnrealEditor-RefactorTactics.dll` è più recente di ogni file di `Source/`.
- **Motore:** l'altra sessione che lo teneva ha concesso la finestra. Durante i due giri non sono girate build, suite né altri processi Unreal, in nessun clone.
- **Banco:** [`Lab.T5.Registro4033`](../4033/Lab.T5.Registro4033.json), invariato.
- **Riga di comando:** quella del secondo giro del 2026-11-10, con `-DPCVars=rt.Debug.PlaybackControls=1`. In entrambi i log c'è la riga `comandi di playback ACCESI`.
- **Catture:** `PrintWindow` a 1920×1080.
  - Nel giro A, in risoluzione, le catture si susseguono senza pausa, ~0,08 s l'una dall'altra.
  - Nel giro B sono ogni ~0,14 s.
  - I tempi «+N s» sono contati dalla riga `Attiva:` del log del motore, sullo stesso orologio UTC.
- **Due giri:**
  - **A:** vista tattica in T1 e nessuna pausa. Misura l'istante e il posto.
  - **B:** come il secondo giro del 2026-11-10, con la vista strategica in T1 e la pausa `K` più tre `L` in T2.
- **Suite:** non rieseguita qui. La PR #4055 ha la suite intera verde su `889088656`.

## Esiti

| Punto | Esito | Tavola |
|---|---|---|
| (a), istante | **PASS**, a questa grana. Fumetto e riga compaiono nella stessa cattura, due volte su due. Scarica lineare: assenti a +0,03 s, presenti a +0,10 s. Sovraccarico: assenti a −0,04 s, presenti a +0,04 s. Il 2026-11-10 la riga arrivava una o due catture dopo il fumetto | `tav_a_stesso_fotogramma.jpg` |
| (a), posto | **PASS**: ogni riga entra sopra la riga d'esito della sua unità, che non cambia | `tav_a_stesso_fotogramma.jpg`, `tav_d_f_gadget_nella_fase.jpg` |
| `D-603` (2), la riga del gadget | **PASS**. Nel giro A «Branth — Copertura portatile» sta in fondo alle PREP e prima delle BLAST, dal primo fotogramma del turno 2 alla fine. L'ordine finale è quello dell'anteprima scelta in `D-603`; nel giro B è lo stesso | `tav_d_f_gadget_nella_fase.jpg`, `tav_b_pausa.jpg` |
| (a), pausa | **PASS** (giro B). Con `K` l'avanzamento resta a `[4%]` e il Registro a quattro righe per 8,5 s, con tre `L` in mezzo. Alla ripresa, a +0,10 s da «Attiva: … Action.Brace», c'è la riga di Irrigidimento e l'avanzamento è a `[21%]` | `tav_b_pausa.jpg` |
| (a), passo | **NOT RUN**: `L` è il micro-passo delle rotte, e la Prep di T2 non ne ha (seduta del 2026-11-10, «Il passo `L`») | — |
| (b) | **PASS** (giro B). In vista strategica la riga di Scarica lineare è assente a −0,03 s e presente a +0,10 s, senza nessun fumetto. Per Sovraccarico la cattura a +0,02 s non ha ancora la riga e quella a +0,17 s sì | `tav_b_pausa.jpg` |
| (c) | **PASS**. In T1 Muiren avversaria (B2, fuori vista) cura 180 punti vita, e `grep -c "Attiva: BP_Unit_Muiren_C_1"` risponde `0` su entrambi i log | — |
| (d) | ⌫ *Superato da `D-604` (2026-11-11): con quella regola queste azioni hanno anche il fumetto.* Contro la regola di allora: **PASS** per l'attacco base e l'azione generica: dopo «Aevik — Impulso ad arco» e «Muiren — Irrigidimento» sopra le due unità non c'è fumetto. **Per il gadget** la riga c'è, ma l'assenza del fumetto resta non osservabile: Branth (A3) sta al bordo basso dell'inquadratura, senza nome né barra disegnati | `tav_d_f_gadget_nella_fase.jpg` |
| (e) | **PASS** nei due giri. Il salto di T4 cade dopo due attivazioni mostrate. Il feed finale (`rt.Debug.ScreenHud`) ha otto righe, di cui quattro attivazioni, nessuna doppia | — |
| (f) | **PASS**: nella pianificazione di T5 le otto righe di T4 restano sotto «ROUND 04 · PRECEDENTE» | `tav_d_f_gadget_nella_fase.jpg` |

## Che cosa le tavole NON mostrano

- **Lo stesso frame del motore.** Fra due catture passano ~0,08 s, cioè diversi frame. Le tavole dicono che fumetto e riga compaiono nella stessa cattura, non nello stesso frame.
- **Gli altri ambienti:** la PIE dell'Editor, il pacchetto, 1440p e 4K.
- **Il budget pieno**, e il lampeggio delle righe sotto un inserimento (`OnLineChanged`).
- **Il giudizio dell'autore** sul budget e sul posto delle righe d'attivazione di unità che hanno una riga d'esito: `D-603` non li ha decisi.

## Fuori dai criteri

- **Il danno di «Colpita» in T2** è 141, non 170 come il 2026-11-10. Fra i due giri su `main` è entrata, fra l'altro, [#4050](https://github.com/DegrassiAaron/refactor-tactics-main/pull/4050) (DEF, copertura e Guard). La causa del numero non è indagata, e non tocca nessun criterio.
- **Uno Spazio in più nel giro A**, durante la pianificazione di T4. La ripresa dopo la finestra di reazione di T3 ripete la riga «Risoluzione: 3 fasi», e lo script l'ha presa per un secondo inizio. Il turno 4 è partito ~1,4 s dopo, come gli altri turni del banco, e il salto di (e) è quello previsto a metà di T4.
- **Il PID 77316 è stato riusato da Windows**, prima per il giro A e poi per il giro B. Ogni processo è stato chiuso dopo averne letto la riga di comando (`-abslog` nella cartella del suo giro), insieme al crash reporter con `-MONITOR=77316`.

## Estratti di log

```
# giro A
[2026.11.11-07.23.42:948][  0]LogTemp: Display: [RT] rt.Debug.PlaybackControls: comandi di playback ACCESI (Pause/Resume/Step).
[2026.11.11-07.23.49:032][224]LogRT: [RT] Attiva: BP_Unit_Aevik_C_0 -> Hero.Aevik.LinearDischarge
[2026.11.11-07.23.50:536][368]LogRT: [RT] Attiva: BP_Unit_Aevik_C_1 -> Hero.Aevik.Overload
[2026.11.11-07.23.54:045][707]LogRT: [RT] Attiva: BP_Unit_Branth_C_0 -> Gadget.PortableCover
[2026.11.11-07.23.54:534][752]LogRT: [RT] Attiva: BP_Unit_Muiren_C_0 -> Action.Brace
[2026.11.11-07.23.55:351][829]LogRT: [RT] Attiva: BP_Unit_Aevik_C_0 -> Hero.Aevik.ArcPulse
[RT] Feed: TurnLog=15 voci, osservatori={0}, righe dopo filtro=8 (di cui attivazioni=4), budget=12

# giro B
[2026.11.11-07.26.35:619][  0]LogTemp: Display: [RT] rt.Debug.PlaybackControls: comandi di playback ACCESI (Pause/Resume/Step).
[2026.11.11-07.26.37:836][ 22]LogRT: [RT] Vista STRATEGICA (arm=2400, enter=2400, exit=1900)
[2026.11.11-07.26.40:040][251]LogRT: [RT] Attiva: BP_Unit_Aevik_C_0 -> Hero.Aevik.LinearDischarge
[2026.11.11-07.26.41:540][409]LogRT: [RT] Attiva: BP_Unit_Aevik_C_1 -> Hero.Aevik.Overload
[2026.11.11-07.26.45:047][799]LogRT: [RT] Attiva: BP_Unit_Branth_C_0 -> Gadget.PortableCover
[2026.11.11-07.26.54:041][796]LogRT: [RT] Attiva: BP_Unit_Muiren_C_0 -> Action.Brace
[RT] Feed: TurnLog=15 voci, osservatori={0}, righe dopo filtro=8 (di cui attivazioni=4), budget=12
```

Nel giro B i tasti, dal registro delle azioni dello script (UTC): `K` alle 07:26:45.222, poi `L` alle 47.728, 49.741 e 51.744, e `K` di ripresa alle 53.722. Fra pausa e ripresa il log del motore non ha nessuna riga `Attiva:`.
