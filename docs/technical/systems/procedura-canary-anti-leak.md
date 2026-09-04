# Canary anti-leak — la procedura, non la definizione

> `CURRENT` · Scritto il **2026-09-04** ([#589](https://github.com/DegrassiAaron/refactor-tactics-main/issues/589)) ·
> Owner del **checkpoint**: `M10.3` in [`roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md#m10--rete-e-privacy).
>
> ⛔ **Questo file non decide che cosa il canary debba dimostrare.** Quello è la DoD di `M10.3`, e resta lì.
> Qui c'è **come si esegue**: cosa si allestisce, dove si guarda, cosa si asserisce, e — la parte che manca
> di solito — come si distingue un «pulito» vero da un «non ho guardato nel posto giusto».
>
> Fonte: `docs/archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md` § **PDR-04 §9**.
> ⚠️ Il PDR presuppone **GAS** e il `FRTCellId` **quadrato**: la procedura non ne dipende, il testo attorno
> sì. Leggere la tabella «cosa vale oggi» in testa a quel documento prima di copiarne qualunque riga.

---

## 1. PDR-04 §9 ha **sei** voci, non cinque — e la sesta è già chiusa

Chi conta gli elenchi trova sei righe dove [#589](https://github.com/DegrassiAaron/refactor-tactics-main/issues/589)
ne nomina cinque. Non manca niente: la sesta è di natura diversa, e per questo è stata eseguita per prima.

| PDR-04 §9 | Cosa chiede | Dove vive |
|---|---|---|
| 1 | dedicated/listen server a due squadre, traffico catturato per client | §4 qui sotto — **aspetta M10.1** |
| 2 | canary ID riconoscibili negli intenti del Team A | §5 — **progettabile oggi** |
| 3 | assert che non compaiano in RPC, proprietà o log del Team B | §6 — **progettabile oggi** |
| 4 | relevancy, reconnect, late join, spectator, replay | §7 — **aspetta M10.1** |
| 5 | eseguire **packaged** | §3 — **è un vincolo, non un passo** |
| **6** | *«un test che fallisce se un tipo server-only acquisisce proprietà replicated»* | ✅ **chiusa il 2026-09-04** — vedi §2 |

## 2. La sesta voce: la guardia strutturale, già in main

Non richiede rete, né M10, né un secondo client: è una verifica sulla **reflection**, e gira nella suite
offline di oggi.

| Cosa | Dove |
|---|---|
| Il checker | `Source/RefactorTactics/Core/RTServerOnlyGuard.h/.cpp` |
| La dichiarazione | `USTRUCT(meta = (RTServerOnly))` — oggi su `FRTPlannedIntent` e `FRTReactionOpportunity` |
| I test | `RefactorTactics.Privacy.ServerOnlyTypesAreNotReplicated` · `…GuardDetectsAPlantedLeak` · `…GuardSeesThroughContainersAndNesting` |

Copre tre rotte: il tipo che acquista una proprietà `CPF_Net`; una `UPROPERTY(Replicated)` che lo
**contiene** a qualunque profondità, anche attraverso `TArray`/`TMap`; un parametro di `UFUNCTION` con
`FUNC_Net`.

⚠️ **La seconda rotta è quella che conta**, e una guardia ingenua la manca: `FRTPlannedIntent` è una
`USTRUCT` e non avrà mai un membro replicato — la replica si dichiara sulla **classe** che lo trasporta.
Nessuno replica un intento nudo: lo replica dentro qualcos'altro.

⛔ **Cosa la guardia non è.** Non è il canary, e non lo anticipa. Dice che *non esiste una via dichiarata*
perché un intento parta; il canary dice che *nessun byte è effettivamente partito*. Il primo si dimostra
sui tipi, il secondo solo su una partita vera.

## 3. Perché **packaged** è una modalità di fallimento, non una preferenza

PDR-04 §9.5 dice «eseguire test packaged; PIE da solo non è sufficiente». È la riga più facile da leggere
come zelo, e non lo è:

> **Se il canary passa in PIE e fallisce packaged, il difetto è nella replica — non nella logica.**

In PIE server e client condividono il processo e larga parte dello stato. Un dato «arrivato» al client può
non aver mai attraversato un `NetDriver`: la stessa memoria è visibile a entrambi. ∴ un canary PIE-only è
verde **anche quando il difetto c'è**, e lo è proprio sulla classe di difetto per cui esiste — dà una
garanzia falsa esattamente dove serviva una vera.

Vale come `NOT RUN`, non come verde: un canary eseguito solo in PIE **non chiude M10.3**.

## 4. Precondizioni — e cosa oggi non esiste

| Precondizione | Stato al 2026-09-04 |
|---|---|
| Listen o dedicated server a due squadre | ⛔ `M10.1`, non esiste |
| Piani in DTO filtrati per squadra | ⛔ `M10.2`, non esiste |
| Build packaged Development | ✅ esiste — `PIE-V01-PACKAGED` la esercita già |
| Guardia strutturale | ✅ §2 |

🔴 **Misurato il 2026-09-04: `Source/` non contiene una sola `UPROPERTY(Replicated)`.** L'unica occorrenza
della parola in tutto il modulo è dentro un commento di `Player/RTPlayerState.h`. Non c'è rete da
osservare, e nessuna parte di §5–§7 è eseguibile oggi.

## 5. Passo 2 — disegnare i canary ID

Un canary è un valore **legale** — altrimenti la validazione lo scarta e non viaggia — e **riconoscibile**
in un flusso di byte che nessuno ha strutturato per essere letto.

**Dove metterlo.** Nel campo che il codice stesso dichiara più privato: `FRTPlannedIntent::ReactionName`,
commentato come *«il campo che non deve MAI raggiungere un avversario»*. Secondo canale: `ActionName`.
Entrambi sono `FText`, quindi portano testo arbitrario senza che nessun validatore li rifiuti.

**Che forma dargli.** Un token ASCII improbabile e unico per squadra e per turno, per esempio
`RTCANARY-T0-<turno>-<esadecimale>`. Un valore casuale non basta: deve essere **ricalcolabile**, o chi
legge un fallimento non può ricostruire quale intento sia partito.

⛔ **Non usare una coordinata come canary.** `FRTCellId` passa per la normalizzazione di PDR-04 §7 («*normalizzare
a `FRTCellId` valido e grafo corrente*»): una cella-sentinella o viene rifiutata, o viene riscritta — e in
entrambi i casi l'assenza a valle non dimostra niente.

⚠️ **Il token va cercato in due codifiche.** `FText`/`FString` in Unreal sono UTF-16 in memoria e possono
serializzare come ANSI quando il contenuto è puro ASCII. Un `grep` su una sola delle due trova zero
occorrenze **anche quando il byte è partito**, e quello zero si legge come «pulito».

## 6. Passo 3 — cosa si asserisce, e il controllo che rende l'assenza credibile

Tre superfici, come le nomina PDR-04 §9.3:

| Superficie | Cosa si guarda |
|---|---|
| **RPC** | i payload ricevuti dalle connessioni del Team B |
| **Proprietà** | lo stato replicato visibile al client B, incluse le struct annidate |
| **Log** | l'output del client B — un canary che compare in un `LogRT` è partito comunque |

🔑 **Il controllo positivo, ed è la parte che una procedura scritta a intenzione dimentica.** «Il canary non
compare nel Team B» è compatibile con due mondi: la privacy funziona, **oppure** si sta guardando nel posto
sbagliato. Le due producono lo stesso output.

∴ ogni esecuzione asserisce **due** cose, e la prima non è opzionale:

1. il canary del Team A **si trova** nel client del Team A — la ricerca funziona, la codifica è giusta, il
   buffer è quello vero;
2. il canary del Team A **non si trova** in nessuna delle tre superfici del Team B.

Senza (1), un canary che non trova mai niente è indistinguibile da una privacy perfetta. È la stessa
ragione per cui la guardia strutturale di §2 pianta un leak apposta e pretende di trovarlo.

## 7. Passo 4 — le cinque condizioni al contorno

`relevancy` · `reconnect` · `late join` · `spectator` · `replay`. Ognuna può consegnare stato **fuori** dal
flusso normale di planning, ed è il motivo per cui il PDR le elenca separatamente: la privacy si può
rompere all'ingresso di una connessione, non durante.

⚠️ **`replay` ha già un owner separato**, e non va confuso con questo passo: il prodotto pubblico del replay
è filtrato per osservatore ([D-316], [D-317]) e presidiato da `Replay.Privacy.ObserverTraceOmitsUnknownEntries`
e da `PIE-REPLAY-OSSERVATORE`. Qui il replay entra come **canale di rete**, non come archivio su disco.

## 8. Cosa **non** è ancora un comando, e perché non lo si inventa

I passi 1 e 4 nominano punti di cattura di un sistema che non esiste: nessun `NetDriver` è mai stato
allestito in questo progetto. Scrivere qui un'invocazione precisa produrrebbe un runbook che fallisce al
primo uso — e che nessuno può correggere, perché non saprebbe se l'errore è nel comando o nel gioco.

**Quello che si può dichiarare oggi**, perché è stato verificato sull'engine e non dedotto:

| Meccanismo | Stato verificato (UE 5.8, sorgente su disco) |
|---|---|
| Network profiler | `Engine/Source/Runtime/Engine/Private/NetworkProfiler.cpp` **esiste**, ma è dietro `USE_NETWORK_PROFILER` — ⚠️ **se sia compilato nella build packaged Development va verificato, non assunto** |
| `net.PackageMap.DebugAll` | CVar **esistente**, letta da `NetDriver.cpp` e `PackageMapClient.cpp` |
| Asserzione in-process | Un controllo lato client sul proprio stato ricevuto **non dipende da nessuno strumento esterno** ed è la via più robusta: è anche l'unica che funziona identica in PIE e packaged, quindi rende confrontabili le due esecuzioni |

La scelta fra cattura del traffico e asserzione in-process **è una decisione da prendere alla prima
esecuzione di M10.3**, e va registrata qui quando è presa.

## 9. Voci PIE

Le verifiche interattive vivono in [`../test-manuali-pie.md`](../test-manuali-pie.md), famiglia `PIE-NET-*`.
Sono ⏳ e **post-v0.1**: quel file è un catalogo, non un backlog, e una voce che aspetta M10 non è lavoro in
ritardo.
