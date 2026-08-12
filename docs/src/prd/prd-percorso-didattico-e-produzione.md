# PRD — Percorso didattico UE, roadmap e produzione

> **Non è fonte normativa.** Livello **8** della gerarchia. Lo **stato** del progetto vive in
> [`../../roadmap/roadmap-checkpoint.md`](../../roadmap/roadmap-checkpoint.md) e
> [`../../roadmap/roadmap-v0.1.md`](../../roadmap/roadmap-v0.1.md); i **requisiti di lungo periodo** in
> [`../../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md`](../../roadmap/RT_PDR_10_Roadmap_QA_Rischi_v0.2.md).
> Nessuna data, stima o percentuale di questo file descrive il presente.
>
> **Testo estratto dai PDF originari, non riscritto.**

## ⚠️ La fase didattica è chiusa dal 2026-08-05

Metà di questo documento — il curriculum, le otto lezioni, il ponte mentale da C# a C++, i criteri di
completamento per lezione — è stato scritto quando RefactorTactics era **anche** un percorso di apprendimento
di Unreal partendo da C#. Quella fase è finita: il progetto è un **prodotto**, i corsi `02-Tutorial` e
`03-TutorialToMVP` sono storia e l'MVP quadrato M0-M5 è archiviato
([piano canonico](../../product/piano-canonico-mvp.md) §2, §4).

Il curriculum resta qui per una ragione sola: è la traccia di **come è stato costruito** ciò che oggi esiste.
Non è un piano di lavoro, e le sue stime in ore non vanno confrontate con niente.

## Da dove viene

| Sorgente (rimosso il 2026-08-12) | Pagine | Cosa contribuisce qui |
|---|---|---|
| `prd-stampabile.pdf` | 40 (di cui 14 qui) | Percorso tutorial e configurazione · piano di sviluppo, metriche e governance |
| `prd-e-piano-di-sviluppo.pdf` | 45 (di cui 19 qui) | Curriculum tutorial · roadmap dal tutorial al prodotto · modding, analytics, test e rischi |
| `prd-roadmap-e-percorso-didattico.pdf` | 35 (di cui 19 qui) | Roadmap · otto lezioni · rischi, qualità e learning curve · artefatti stampabili |
| `guida-trovare-asset-free.pdf` | 7 | Fonti di asset gratuiti, tabella comparativa dei pacchetti, asset minimi per il POC, importazione in UE |

## Cosa resta vero, cosa no

**Superato per intero.** Timeline, milestone datate, stime in ore/giorni-persona, «ruoli minimi» di un team
che non esiste, la sequenza delle lezioni, i criteri di uscita dal vertical slice espressi in settimane.
Un'assunzione ricorrente merita di essere detta ad alta voce: questi documenti ipotizzano **3-5 o 4-6 FTE**;
il progetto è **a dev singolo** ([piano canonico](../../product/piano-canonico-mvp.md) §9).

**Recuperabile, e non ancora recepito da nessuno.** È il gruppo più consistente dell'intero corpus PRD,
perché la produzione è l'area che il repository ha coperto meno:

- 🟢 **Il risk register a quindici voci** con probabilità, impatto e mitigazione. PDR-10 v0.2 ha un modello di
  rischio; questo ne ha l'istanza compilata, e include rischi che il repo non nomina — bilanciamento
  combinatorio, learning curve, dipendenza da asset di terze parti.
- 🟢 **I tredici gate di release** e la *definition of done per feature*. Il repo ha DoD per checkpoint, non
  per feature né per release.
- 🟢 **I quindici eventi di analytics** e l'adapter interno che li disaccoppia dal provider (Unreal espone
  un'interfaccia astratta con un provider su file, utile in sviluppo e **non** adatto alla produzione). Non
  esiste telemetria nel progetto.
- **La strategia di testing a quattro assi** — unit · deterministico con *golden hash* · rete · funzionale —
  più il log per-round. Il *golden hash* è già la pratica del `TurnLog`; gli altri tre assi no.
- **Le decisioni da congelare dopo il vertical slice**: un elenco esplicito di ciò che va deciso *dopo* aver
  misurato, che è precisamente il meccanismo di
  [`../../OPEN_DECISIONS.md`](../../OPEN_DECISIONS.md).
- **La guida agli asset gratuiti** (Kenney, Quaternius, OpenGameArt, Sketchfab, Itch, Freesound, Fab) con
  tabella comparativa e set minimo per un POC. Resta operativa: il canone dichiara la **direzione artistica
  inesistente** e usa placeholder. Va riletta sapendo che parla di importazione in **UE 5.8.1** e che gli
  asset di terze parti nel repo vivono in `Content/FabAsset/`, non versionati.

**Da verificare prima di usarlo.** Le note di configurazione passo-passo citano percorsi (`C:\Dev\…`,
`D:\Dev\…`) e versioni di UE diverse da quella bloccata; il progetto vive nella **radice del repository**.

---

## PRD stampabile — percorso tutorial, piano di sviluppo, metriche e governance

### Percorso tutorial e configurazione
Il percorso formativo usa il gioco come progetto didattico. Ogni lezione deve produrre una parte eseguibile, introdurre un numero limitato di concetti e terminare con una verifica concreta.

#### Prerequisiti.
|Requisito|Minimo|
|---|---|
|Sistema operativo consigliato|Windows per il primo percorso|
|Unreal Engine|Ultima release stabile scelta dal progetto|
|IDE|Visual Studio 2022|
|Workload VS|Game development with C++|
|SDK|Windows SDK compatibile con la versione UE|
|Version control|Git e Git LFS|
|Spazio libero|Almeno 100 GB consigliati per motore, cache e progetto|
|Conoscenze|C# base, classi, interfacce, collection, eventi|

Visual Studio 2022 è il toolchain predefinito per le release moderne di Unreal; la configurazione ufficiale richiede il workload C++ per game development e un Windows SDK compatibile. 15

#### Configurazione passo per passo.
|Passo|Operazione|Risultato|
|---|---|---|
|Installazione|Installare Epic Games Launcher|Accesso alle versioni UE|
|Motore|Installare una versione stabile di UE5|Editor disponibile|
|IDE|Installare Visual Studio 2022|Compiler C++ disponibile|
|Workload|Selezionare Game development with C++|Toolchain completa|
|Progetto|Games→Blank→C++→Desktop|Progetto compilabile|
|Nome|`RefactorTactics`|Namespace coerente|
|Repository|Inizializzare Git|Cronologia attiva|
|LFS|Tracciare<br>`.uasset` ,<br>`.umap` ,audio e texture|Binary asset gestiti|
|Primo commit|Compilare e aprire l’editor|Baseline riproducibile|
|Plugin|Attivare Gameplay Tags ed Enhanced Input|Fondazioni gameplay|
|Test|Creare una mappa<br>`L_DevSandbox`|Area di esercizio|

Non conviene iniziare compilando Unreal dal sorgente. La build binaria tramite Launcher è l’approccio più semplice; l’accesso al sorgente diventa utile soltanto quando è necessario ispezionare o modificare il motore. 16

#### Struttura dei contenuti.
```
Content/RefactorTactics/
├── Characters/
├── Core/
├── Data/
├── Input/
├── Maps/
├── Materials/
├── UI/
├── VFX/
└── Tests/
```

Le cartelle non devono essere organizzate per nome del singolo sviluppatore. Ogni asset usa un prefisso coerente, per esempio `BP_` , `WBP_` , `DA_` , `GA_` , `GE_` , `IA_` , `IMC_` , `M_` e `T_` .

#### Roadmap delle lezioni.
|Lezione|Prodotto costruito|Concetti|
|---|---|---|
|Ambiente e primo<br>progetto|Progetto compilabile e<br>repository|Editor, IDE, build, commit|
|Unreal per chi conosce C#|Actor C++ esposto a Blueprint|UCLASS, UPROPERTY, lifecycle|
|Camera tattica|Pan, zoom e rotazione|Pawn, Controller, Enhanced<br>Input|
|Prima cella|Cella selezionabile|Actor, collisione, mouse trace|
|Generatore di griglia|Mappa 2D graybox|Loop, spawn, coordinate|
|Identità delle celle|`FRTCellId` e lookup|USTRUCT, TArray, TMap|
|Grafo di movimento|Vicini e archi|Strutture dati e debug|
|Primo A*|Percorso tra due celle|Heuristic, open/closed set|
|Budget di movimento|Evidenziazione celle<br>raggiungibili|Query e costi|
|Mappa multilivello|Scale e ponte|Layer e transizioni|
|Dati del terreno|Acqua, roccia, fuoco|Data Asset e Gameplay Tags|
|Personaggio tattico|Selezione e movimento|Pawn, stato logico, animazione|
|Ciclo di turno|Planning e resolution|State machine|
|Prima abilità|Attacco su una cella|GAS, targeting, cooldown|

|Lezione|Prodotto costruito|Concetti|
|---|---|---|
|LOS e copertura|Tiro bloccato o protetto|Trace, quota, cover|
|Ambiente reattivo|Acqua più elettricità|Eventi e sistemi|
|UI di pianificazione|Percorso e area fantasma|UMG e view model|
|Intenzioni alleate locali|Due finestre PIE|Separazione player/team|
|Multiplayer autorevole|Commit e risoluzione server|RPC, replica, authority|
|Warning di conflitto|Collisioni tra piani alleati|Validator di intenti|
|Ping e label|Comunicazione contestuale|UI e messaggi di squadra|
|Disegno tattico|Linee temporanee|Spline, rate limit|
|Personaggi modulari|Loadout e talenti|Primary Data Asset|
|Replay del turno|Command log riproducibile|Serializzazione e seed|
|Vertical slice|Partita 2 contro 2|Integrazione e playtest|
|Fondazioni mod|Import JSON validato|Schema e registri|
|Mod toolkit|Terreno o abilità custom|Packaging e validator|

#### Formato di ogni lezione.
#### `Obiettivo`

- `→ Concetti nuovi`

- `→ Passi nell’editor`

- `→ Codice minimo`

- `→ Test manuale`

- `→ Errore comune`

- `→ Piccola sfida`

- `→ Commit suggerito`

#### Esempio di lezione: prima cella.
Obiettivo: creare una cella selezionabile da Blueprint.

File `RTGridCell.h` :

```
#pragma once
#include"CoreMinimal.h"
#include"GameFramework/Actor.h"
#include"RTGridCell.generated.h"
```

```
UCLASS()
```

```
classREFACTORTACTICS_APIARTGridCell:publicAActor
```

`{ GENERATED_BODY() public: ARTGridCell(); UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid") int32 GridX = 0; UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Grid") int32 GridY = 0; UFUNCTION(BlueprintCallable, Category = "Grid") FString GetCellLabel() const; };` File `RTGridCell.cpp` :

```
#include"RTGridCell.h"
ARTGridCell::ARTGridCell()
{
PrimaryActorTick.bCanEverTick=false;
}
FStringARTGridCell::GetCellLabel()const
{
returnFString::Printf(TEXT("Cell %d, %d"),GridX,GridY);
}
```

Passi:

1. Compilare il progetto.

2. Nell’editor creare un Blueprint derivato da `RTGridCell` .

3. Chiamarlo `BP_GridCell` .

4. Aggiungere uno Static Mesh Component con un cubo piatto.

5. Impostare `GridX` e `GridY` .

6. Chiamare `GetCellLabel` da `BeginPlay` .

7. Stampare il risultato.

8. Verificare che il log mostri coordinate corrette.

9. Eseguire il commit `lesson/grid-cell` .

#### Esempio di lezione: costo di attraversamento.
```
int32GetTerrainCost(constFGameplayTagContainer&CellTags)
{
if(CellTags.HasTagExact(
FGameplayTag::RequestGameplayTag(TEXT("Terrain.Water"))))
{
```

```
return2;
}
if(CellTags.HasTagExact(
FGameplayTag::RequestGameplayTag(TEXT("Hazard.Fire"))))
{
return5;
}
return1;
}
```

La versione didattica può usare condizioni dirette. Una lezione successiva sostituisce gli `if` con Data Asset e provider registrati.

#### Esempio di lezione: stato del turno.
```
UENUM(BlueprintType)
enumclassERTTurnPhase:uint8
{
Planning,
Committing,
Resolving,
Cleanup
};
```

```
UCLASS()
classREFACTORTACTICS_APIARTTurnManager
:publicAGameStateBase
{
GENERATED_BODY()
public:
UPROPERTY(ReplicatedUsing=OnRep_TurnPhase)
ERTTurnPhaseTurnPhase=ERTTurnPhase::Planning;
UFUNCTION()
voidOnRep_TurnPhase();
};
```

La prima versione cambia fase con un timer. Le lezioni successive aggiungono ready, commit, snapshot e server authority.

#### Risorse ufficiali da seguire.
|Tema|Risorsa|
|---|---|
|Passaggio da Unity/C#|Guida Unreal per sviluppatori Unity<br>17|

|Tema|Risorsa||
|---|---|---|
|Blueprint|Blueprint Overview<br>18||
|Bilanciamento C++ e Blueprint|Guida Epic dedicata<br>19||
|Gameplay Ability System|Documentazione GAS<br>12||
|Gameplay Tags|Documentazione dei tag<br>20||
|Enhanced Input|Documentazione Input<br>21||
|UMG|UMG UI Designer Quick Start|22|
|Navigazione grafi|API<br>`FGraphAStar`<br>23||
|A* originale|Hart, Nilsson e Raphael<br>24||
|HPA*|Botea, Müller e Schaeffer<br>25||
|D* Lite|Koenig e Likhachev<br>26||
|Recast/Detour|Repository ufficiale<br>27||
|EOS|Plugin Online Subsystem EOS|28|

#### Regole didattiche.
- Una lezione non introduce più di due macro Unreal nuove.

- Il codice da copiare deve compilare prima delle estensioni.

- Ogni snippet specifica file e classe.

- Ogni lezione termina con un test visibile.

- Il multiplayer viene introdotto dopo che il ciclo locale funziona.

- GAS viene introdotto con una sola abilità semplice.

- Le ottimizzazioni arrivano dopo profiling e test.

- Ogni refactor conserva un test o una scena dimostrativa.

### Piano di sviluppo, metriche e governance
Le stime seguenti assumono un piccolo team professionale, un progetto originale senza asset AAA e sviluppo PC-first. Non includono tempi di raccolta fondi, certificazione console o campagne marketing estese.

#### Milestone e deliverable.
|Milestone<br>Du|rata indicativa|Deliverable|
|---|---|---|
|Fondazioni|2–3 mesi|Camera, griglia, celle, input, repo, build automatica|
|Prototipo tattico|3–4 mesi|A*, movimento, turni locali, attacco, LOS|
|Prototipo di rete|2–3 mesi|Server authority, intenti di squadra, ready e snapshot|
|Vertical slice|4–6 mesi|2v2, una mappa, quattro personaggi, UI leggibile|
|MVP|6–9 mesi|3v3, due mappe, sei personaggi, matchmaking base|

|Milestone|Durata indicativa|Deliverable|
|---|---|---|
|Alpha|4–6 mesi|Otto personaggi, tre mappe, progressione e telemetria|
|Beta|4–6 mesi|Bilanciamento, onboarding, stabilità, contenuti|
|Mod release|6–9 mesi|Toolkit, map mod, data mod, browser/import e validator|

Una produzione completa con modding può quindi richiedere circa 31–46 mesi con un team ridotto. La durata diminuisce con più personale esperto, ma cresce rapidamente se il progetto richiede qualità artistica elevata, cross-platform o live service.

<!-- Start of picture text -->
RefactorTactics — timeline indicativa<br>Fondazioni Setup, camera e griglia Pathfinding e mappa multilivello<br>Turni e simulatore<br>Core Networking e intenti condivisi<br>Vertical slice<br>MVP<br>Produzione Alpha<br>Beta<br>Estensione Mod toolkit e release<br>2026 2027 2027 2027 2027 2028 2028 2028 2028 2029 2029 2029 2029 2030<br><!-- End of picture text -->

Le date del diagramma sono indicative e assumono avvio operativo a settembre 2026.

#### Scope del vertical slice.
|Area|Incluso|
|---|---|
|Formato|2 contro 2|
|Mappa|Una mappa graybox rifinita parzialmente|
|Livelli|Piano terreno, ponte e tunnel|
|Personaggi|Quattro|
|Abilità|Quattro per personaggio|
|Moduli|Una variante per due abilità|
|Ambiente|Fuoco, acqua, elettricità|
|UI|Path, AoE, intenti alleati, warning, ready|
|Networking|Dedicated server di test|
|Modalità|Obiettivo singolo dinamico|
|Audio/VFX|Minimo necessario alla leggibilità|
|Progressione|Nessuna|
|Modding|Soltanto architettura dati interna|

#### Criteri di uscita dal vertical slice.
Soglia

Criterio Soglia Giocatori che comprendono il ciclo dopo tutorial Almeno 80%

|Criterio|Soglia|
|---|---|
|Giocatori che notano gli intenti alleati|Almeno 90%|
|Turni con conflitto accidentale tra alleati|Meno del 15% dopo tre partite|
|Azioni con esito percepito come inspiegabile|Meno del 5%|
|Partite completate senza blocchi|Almeno 95%|
|Tempo medio decisione|Tra 15 e 30 secondi|
|Divergenze client/server|Zero|
|Leak di intenti avversari|Zero|
|Path query oltre budget|Meno dell’1%|
|Interesse a una seconda sessione|Almeno 60% dei tester target|

#### Scope MVP.
|Area|Target MVP|
|---|---|
|Formato|3 contro 3|
|Mappe|Due|
|Personaggi|Sei|
|Specializzazioni|Due per personaggio|
|Talenti|Sei o più per personaggio|
|Gadget|Sei condivisi|
|Modalità|Convergenza|
|Multiplayer|Sessioni private e unranked|
|Tutorial|Tutorial interattivo e partita contro bot|
|Replay|Replay logico|
|Progressione|Sblocchi orizzontali limitati|
|Modding pubblico|No|

**Scope alpha.** L’alpha aggiunge terza mappa, otto personaggi, matchmaking più robusto, riconnessione, profili, progressione completa, telemetria, bot migliorati, strumenti di bilanciamento e primi test del pipeline mod interno.

**Scope beta.** La beta blocca il formato principale, riduce i cambiamenti architetturali, espande onboarding, accessibilità, localizzazione, performance, sicurezza, reportistica, moderazione e stabilità dei servizi.

#### **Release mod.** La release mod introduce:

- manifest versionato;

- import di data mod;

- editor o template per mappe;

- validatore;

- packaging;

- gestione dipendenze;

- hash di rete;

- partita custom con mod;

- documentazione API;

- esempi ufficiali;

- policy di contenuto;

- migrazione tra versioni.

#### Ruoli minimi.
|Ruolo|Fondazioni|Produzione|
|---|---|---|
|Technical/game director|1|1|
|Gameplay programmer|1|2–3|
|Network/backend programmer|0,5|1–2|
|Technical designer|1|1–2|
|UI/UX designer|0,5|1|
|Level designer|0,5|1–2|
|Environment artist|0,5|1–2|
|Character artist/animator|Outsource o 0,5|1–2|
|VFX/audio|Outsource|1 combinato o outsource|
|QA|Condiviso|2–4|
|Producer|0,5|1|
|Community/mod tools|0|1 nella fase mod|

Un team iniziale realistico è di quattro-sei persone equivalenti full-time. Durante produzione e beta può crescere a dieci-quindici, includendo QA e outsourcing.

#### Stima di effort per sistema.
|Sistema|Effort relativo|Rischio|
|---|---|---|
|Camera e selezione|Basso|Basso|
|Griglia multilivello|Medio|Medio|
|Pathfinding semantico|Alto|Alto|

|Sistema|Effort relativo|Rischio|
|---|---|---|
|Simulatore dei turni|Alto|Alto|
|Intenti condivisi|Medio|Alto per privacy di rete|
|UI di previsione|Alto|Alto|
|Personaggi modulari|Alto|Alto per bilanciamento|
|GAS integration|Medio|Medio|
|Sistemi ambientali|Alto|Molto alto per combinazioni|
|Dedicated server|Alto|Alto|
|Matchmaking|Medio|Medio|
|Replay deterministico|Medio|Alto|
|Modding|Molto alto|Molto alto|
|Contenuti artistici|Molto alto|Dipende dalla qualità target|

#### Metriche di prodotto.
|Categoria|Metrica|
|---|---|
|Onboarding|Completamento tutorial|
|Comprensione|Percentuale di azioni correttamente previste|
|Coordinazione|Utilizzo di intenti, ping e label|
|Frizione|Numero di conflitti accidentali|
|Profondità|Diversità delle build utilizzate|
|Bilanciamento|Win rate per personaggio, mappa e build|
|Qualità mappe|Distribuzione delle aree utilizzate|
|Durata|Tempo partita e turni medi|
|Retention|Ritorno dopo prima e seconda sessione|
|Stabilità|Crash, disconnessioni, divergenze|
|Modding|Mod validate, installate e partite custom|

**Metriche di bilanciamento.** Il win rate globale non è sufficiente. I dati devono essere segmentati per livello di abilità, mappa, composizione, specializzazione, perk di squadra e ordine di scelta. Una build con win rate normale ma pick rate quasi totale può essere comunque problematica.

#### Rischi principali.

|Rischio|Impatto|Mitigazione|
|---|---|---|
|Troppa complessità<br>della mappa|Il giocatore non comprende<br>l’esito|Introdurre pochi sistemi per mappa e<br>usare preview esplicite|
|Esplosione<br>combinatoria|QA e bilanciamento<br>ingestibili|Tassonomia, limiti agli effetti e test<br>combinatori|
|UI sovraccarica|Intenti illeggibili|Filtri, priorità, trasparenza e modalità<br>focus|
|Pathfinding lento|Preview scattosa|Chunk, cache, query asincrone e<br>profiling|
|Divergenze di<br>simulazione|Risultati differenti|Server authority, fixed-point, replay<br>test|
|Leak di intenti|Vantaggio competitivo grave|Team relay, test pacchetti e audit|
|Blueprint spaghetti|Manutenzione difficile|API C++, Blueprint piccoli e data-driven|
|GAS troppo accoppiato|Simulatore dipendente da<br>Actor runtime|Adapter tra GAS e stato tattico|
|Personaggi senza<br>identità|Build intercambiabili|Kit fondamentale non sostituibile|
|Progressione pay/<br>grind-to-win|Perdita di fiducia|Solo opzioni orizzontali e cosmetiche|
|Mod incompatibili|Crash e frammentazione|Schema, hash, sandbox e migration|
|Scope eccessivo|Progetto non completato|Vertical slice con gate rigoroso|
|Server costoso|Sostenibilità ridotta|Simulazione turn-based efficiente e<br>autoscaling|
|Bassa popolazione|Matchmaking lento|Bot, custom lobby e modalità asincrone<br>future|

#### Strategia di testing.
|Test|Descrizione|
|---|---|
|Unit test pathfinding|Percorsi, costi, blocchi, dislivelli|
|Golden test|Snapshot noto produce TurnLog noto|
|Replay determinism|Stesso log riprodotto più volte|
|Network privacy|Client nemico non riceve intenti|
|Packet loss|Preview e commit con latenza/perdita|
|Map validation|Spawn, obiettivi, connessioni e loop|
|Ability validation|Costi, tag, target e cooldown|
|Combinatorial test|Interazioni tra superfici ed effetti|

|Test|Descrizione|
|---|---|
|Soak test|Match ripetuti per ore|
|Mod schema test|File validi, invalidi e versioni precedenti|
|UX playtest|Comprensione senza spiegazione verbale|
|Accessibility|Contrasto, dimensione, input e motion|

#### Definition of done per feature.
Una feature è completata soltanto quando:

- possiede requisiti e criteri di accettazione;

- funziona su server e client quando applicabile;

- non espone informazioni non autorizzate;

- dispone di log e debug visualization;

- possiede almeno un test automatico per la regola principale;

- è utilizzabile con input rimappato;

- produce messaggi di errore leggibili;

- è documentata per designer;

- non introduce riferimenti hard-coded ai contenuti;

- è stata provata in una build packaged, non soltanto in Play In Editor.

#### Decisioni da congelare dopo il vertical slice.
|Decisione|Momento di lock|
|---|---|
|Dimensione squadra principale|Fine vertical slice|
|Sequenza di risoluzione|Prima dell’MVP|
|Modello delle celle|Prima del vertical slice|
|API pathfinding|Prima del vertical slice|
|Struttura degli intenti|Prima del prototipo di rete|
|Tassonomia Gameplay Tags|Versione iniziale prima dell’MVP|
|Schema dei contenuti|Alpha|
|Formato mod pubblico|Beta|
|Catalogo competitivo|Ogni stagione|

**Criterio finale di successo.** RefactorTactics è riuscito quando i giocatori possono osservare una situazione multilivello, proporre un piano visibile ai compagni, modificare il terreno attraverso personaggi differenti e comprendere perché la risoluzione ha prodotto un determinato risultato. La quantità di personaggi, mappe e mod viene dopo questa qualità fondamentale.

1 10 19 Balancing Blueprint and C++ | Unreal Engine 4.27 ... https://dev.epicgames.com/documentation/en-us/unreal-engine/balancing-blueprint-and-cplusplus? application_version=4.27&utm_source=chatgpt.com

2 12 Gameplay Ability System | Unreal Engine 4.27 ... https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system? application_version=4.27&utm_source=chatgpt.com

##### 3

#### unreal.Actor — Unreal Python 5.7 (Experimental) ...

https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/Actor? application_version=5.7&utm_source=chatgpt.com

4 20

#### Gameplay Tags | Unreal Engine 4.27 Documentation

https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-tags? application_version=4.27&utm_source=chatgpt.com

5 24

#### A Formal Basis for the Heuristic Determination of Minimum ...

https://www.cs.auckland.ac.nz/courses/compsci709s2c/resources/Mike.d/astarNilsson.pdf?utm_source=chatgpt.com

#### FIndexArray | Unreal Engine 5.5 Documentation

6 23 FIndexArray | Unreal Engine 5.5 Documentation https://dev.epicgames.com/documentation/en-us/unreal-engine/API/Runtime/AIModule/FGraphAStar/FIndexArray? application_version=5.5&utm_source=chatgpt.com

#### Unreal Python 5.0 (Experimental) documentation

##### 7

https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/PredictProjectilePathParams? application_version=5.0&utm_source=chatgpt.com

8 21

#### Input | Unreal Engine 4.27 Documentation

https://dev.epicgames.com/documentation/en-us/unreal-engine/input?application_version=4.27&utm_source=chatgpt.com

##### 9

#### Unreal Engine 5.3 Release Notes - Epic Games Developers

https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5.3-release-notes? application_version=5.3&utm_source=chatgpt.com

11 17

#### Unreal Engine 4 For Unity Developers

https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-4-for-unity-developers? application_version=4.27&utm_source=chatgpt.com

##### 13

#### Property Replication in Unreal Engine

https://dev.epicgames.com/documentation/en-us/unreal-engine/property-replication-in-unreal-engine? application_version=5.2&utm_source=chatgpt.com

#### 14 Assets and Packages | Unreal Engine 4.27 Documentation

https://dev.epicgames.com/documentation/en-us/unreal-engine/assets-and-packages? application_version=4.27&utm_source=chatgpt.com

##### 15

#### Unreal Engine 5.2 Release Notes

https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-5.2-release-notes? application_version=5.2&utm_source=chatgpt.com

##### 16

#### Non-Games Custom License Getting Started Guide

https://dev.epicgames.com/documentation/en-us/unreal-engine/non-games-custom-license-unreal-engine-getting-startedguide?application_version=4.27&utm_source=chatgpt.com

##### 18

#### Blueprint Overview | Unreal Engine 4.27 Documentation

https://dev.epicgames.com/documentation/en-us/unreal-engine/blueprint-overview? application_version=4.27&utm_source=chatgpt.com

##### 22

#### UMG UI Designer Quick Start Guide - Epic Games Developers

https://dev.epicgames.com/documentation/en-us/unreal-engine/umg-ui-designer-quick-start-guide? application_version=4.27&utm_source=chatgpt.com

> 25 Near Optimal Hierarchical Path-Finding

https://webdocs.cs.ualberta.ca/~mmueller/ps/2004/hpastar.pdf?utm_source=chatgpt.com

> 26 D* Lite

https://aaai.org/Papers/AAAI/2002/AAAI02-072.pdf?utm_source=chatgpt.com

> 27 Industry-standard navigation-mesh toolset for games · GitHub

https://github.com/recastnavigation/recastnavigation?utm_source=chatgpt.com

#### 28 The EOS Online Subsytem (OSS) Plugin | Unreal Engine ...

https://dev.epicgames.com/documentation/en-us/unreal-engine/the-eos-online-subsytem-oss-plugin? application_version=4.27&utm_source=chatgpt.com

---

## PRD e piano di sviluppo — curriculum, roadmap, modding, analytics, test e rischi

### Curriculum tutorial
Il curriculum produce il prodotto reale. Ogni lezione termina con un deliverable integrato nel repository.

Le durate rappresentano lavoro attivo e non includono eventuali blocchi, approfondimenti o rifacimenti.

#### Sequenza completa
|Lezione|Durata|Obiettivo|Deliverable|
|---|---|---|---|
|Ambiente e repository|Tre ore|Installare tool, creare progetto e<br>Git|Build iniziale<br>versionata|
|Orientamento<br>nell’Editor|Tre ore|Viewport, Content Browser,<br>Level, PIE e log|Stanza di test|
|Blueprint essenziali|Quattro<br>ore|Eventi, funzioni, struct e debug|Pedina cliccabile|
|C++ per chi conosce C#|Cinque<br>ore|Reflection, header, source e<br>UObject|Actor C++ esposto|
|Enhanced Input|Tre ore|Pan, zoom, click e mapping<br>context|Camera completa|
|Coordinate della griglia|Cinque<br>ore|World-grid e grid-world|Griglia logica|

|Lezione|Durata|Obiettivo|Deliverable|
|---|---|---|---|
|Rendering della griglia|Cinque<br>ore|Instanced mesh e highlight|Mappa fluida|
|Selezione delle celle|Quattro<br>ore|Hover, click e feedback|Selettore riutilizzabile|
|Grafo multilivello|Sei ore|Nodi, archi e layer|Due piani collegati|
|A* minimale|Sei ore|Open set, costi e parent|Percorso<br>deterministico|
|Cost provider|Sei ore|Terreno, profilo e pericolo|Percorsi diversi per<br>unità|
|Linea di vista|Sei ore|Altezza, cover e tracing|Preview tiro|
|Turn manager|Sei ore|Planning, lock e aftermath|Round locale|
|Piano di azioni|Sei ore|Movimento, abilità e undo|Timeline del piano|
|Resolver simultaneo|Otto ore|Microstep, conflitti e priorità|Due unità risolte|
|Ability System|Otto ore|Ability, effect, tag e cooldown|Tre abilità|
|Planning UI|Sei ore|UMG, Common UI e tooltip|HUD funzionale|
|Intenti condivisi locali|Cinque<br>ore|Ghost, path, AoE e label|Due piani visibili|
|Networking<br>fondamentale|Otto ore|GameMode, GameState, RPC e<br>replica|Match listen server|
|Privacy degli intenti|Otto ore|Replica filtrata e anti-leak|Alleati sì, nemici no|
|IA tattica|Dieci ore|Candidate generation e utility|Bot con piano valido|
|Personalizzazione|Otto ore|Data Asset, moduli e budget|Due build differenti|
|Test e analytics|Otto ore|Automation, Insights ed eventi|Suite automatica|
|Packaging|Otto ore|Development, Shipping e config|Demo installabile|
|Mod dati|Dieci ore|JSON, schema e validator|Nuovo terreno senza<br>build|
|Roguelike|Sedici ore|Run, deck, ricompense e save|Mini-run cooperativa|

#### Criteri di completamento di ogni lezione
Una lezione è conclusa soltanto quando:

1. il progetto compila da una checkout pulita;

2. il comportamento è riproducibile;

- esiste almeno un test per la nuova regola;

- il deliverable è usabile dalla lezione successiva;

5. non sono presenti errori rossi nel log;

- il codice è committato;

- la documentazione del modulo è aggiornata;

- le decisioni temporanee sono registrate come debito tecnico.

#### Milestone didattiche
|Milestone|Cosa sai fare|
|---|---|
|Sandbox interattiva|Editor, Blueprint, input e Actor|
|Griglia funzionante|Struct, container, conversioni e rendering|
|Pathfinding multilivello|Algoritmi C++, grafi e test|
|Turno locale|Macchina a stati e separazione modello-vista|
|Ability vertical slice|GAS, tag, effetti e dati|
|Planning collaborativo|UI complessa e visualizzazione|
|Multiplayer privato|RPC, autorità e replica selettiva|
|Beta tattica|IA, personalizzazione e bilanciamento|
|Build distribuibile|Packaging, profiling e crash handling|
|Modding|JSON, schema, dipendenze e sicurezza|
|Espansione roguelike|Save, generazione run e deckbuilding|

#### Metodo di studio consigliato
Per ogni lezione:

```
Leggi il concetto
      |
      v
Riproduci l'esempio minimo
      |
      v
Scrivi un test
      |
      v
Integra nel gioco
      |
      v
Rompi volontariamente il sistema
      |
      v
Leggi log e debugger
      |
      v
Rifattorizza
```

Non copiare grandi sistemi da Lyra all’inizio. Leggi Lyra quando hai già costruito una versione semplice dello stesso problema: a quel punto il sample diventa comprensibile invece di sembrare magia nera.

### Roadmap dal tutorial al prodotto
#### Timeline di alto livello
<!-- Start of picture text -->
Roadmap RefactorTactics<br>ApprendimentoFondazioni e griglia<br>Vertical slice locale<br>Core Multiplayer alpha<br>Beta del core<br>Qualità e lancio<br>Prodotto Supporto mod<br>Roguelike deckbuilding<br>2026 2027 2027 2027 2027 2028 2028 2028 2028 2029<br><!-- End of picture text -->

Versione leggibile in stampa:

```
Fondazioni e griglia       | Mesi iniziali      | Sandbox interattiva
Vertical slice locale      | Fase successiva    | Match locale completo
Multiplayer alpha          | Dopo la slice      | Due contro due online
Beta del core              | Fase centrale      | Quattro contro quattro
Qualità e lancio           | Pre-release        | Prodotto PC
Supporto mod               | Dopo il core       | SDK dati
Roguelike deckbuilding     | Espansione         | Co-op da uno a quattro
```

#### Fasi e gate
|Fase|Contenuto|Gate di uscita|
|---|---|---|
|Fondazioni didattiche|Editor, Git, Blueprint, C++, camera|Sandbox avviabile|
|Griglia|Coordinate, rendering, selezione|Mappa interattiva|
|Pathfinding|Grafo, layer, cost provider|Percorsi verificati|
|Vertical slice locale|Turni, abilità, UI e ambiente|Match uno contro uno|
|Multiplayer alpha|Autorità, intenti e riconnessione|Due contro due stabile|
|Core beta|Quattro contro quattro, IA, build|Feature complete|
|Qualità|Accessibilità, performance e replay|Release candidate|
|Lancio|Packaging, servizi e operazioni|Prodotto PC|
|Mod support|Loader, schema, validator e browser|SDK mod|
|Roguelike|PvE, run, deck, boss e save|Espansione co-op|

#### Scope della vertical slice
La vertical slice deve contenere solo:

|Sistema|Quantità|
|---|---|
|Mappa|Una|
|Layer|Due|
|Personaggi|Quattro|
|Abilità per personaggio|Tre|
|Specializzazioni|Nessuna o una semplificata|
|Modalità|Relay Control|
|Giocatori|Uno contro uno, bot opzionale|
|Tipi di terreno|Quattro|
|Collegamenti verticali|Scala e jump pad|
|Effetti ambientali|Fuoco e acqua|
|Interazioni|Porta e terminale|
|UI planning|Completa ma placeholder|
|Networking|Escluso dalla prima slice|

La slice deve dimostrare:

1. movimento multilivello;

2. un percorso diverso per due unità;

- una abilità che modifica la mappa;

- una interazione fra superficie e abilità;

- un conflitto simultaneo;

- una preview leggibile;

- una risoluzione ripetibile.

#### Alpha multiplayer
Aggiunge:

|Area|Requisito|
|---|---|
|Server|Autorevole|
|Match|Due contro due|
|Intention sharing|Solo alleati|
|Reconnect|Snapshot del round|
|Ready state|Replicato|
|Timeout|Gestito server-side|
|Abbandono|Bot o surrender|

|Area|Requisito|
|---|---|
|Logging|Piani, eventi e hash|
|Test|Due processi più packet simulation|

Il sistema online può essere costruito dietro un’interfaccia. Common User è usato in Lyra per login, autenticazione e sessioni, mentre l’integrazione EOS richiede registrazione e configurazione del prodotto. 20

#### Beta del core
La beta è feature-complete quando comprende:

- quattro contro quattro;

- almeno otto personaggi;

- almeno tre mappe;

- una modalità principale;

- personalizzazione orizzontale;

- bot capaci di completare i match;

- tutorial giocabile;

- replay tecnico; • analytics; • riconnessione;

- accessibilità di base;

- validator mappe;

- suite deterministica;

- build installabile.

#### Espansione cooperativa roguelike
La modalità roguelike non sostituisce il prodotto core: usa la stessa griglia, lo stesso resolver, le stesse abilità e lo stesso networking.

Loop:

```
Scegli personaggio e starter deck
              |
              v
Scegli un nodo della run
              |
              v
Combattimento simultaneo
              |
              v
Ricompensa o evento
              |
              v
Modifica deck e build
              |
              v
```

<!-- Start of picture text -->
Miniboss<br>              |<br>              v<br>Nuovo atto<br>              |<br>              v<br>Boss finale<br><!-- End of picture text -->

Struttura proposta:

|Elemento|Baseline|
|---|---|
|Giocatori|Da uno a quattro|
|Atti|Tre|
|Combattimenti|Da sei a dieci|
|Miniboss|Uno per atto|
|Boss finale|Uno|
|Deck iniziale|Otto–dodici carte|
|Mano|Quattro–sei carte|
|Energia|Risorsa rinnovata per round|
|Reliquie|Passive della run|
|Eventi|Scelte narrative e meccaniche|
|Salvataggio|Tra nodi, con versione schema|
|Meta-progression|Sblocchi orizzontali|

Decisione critica: **movimento base, attacco base e difesa minima devono essere sempre disponibili** , indipendentemente dalla mano. Le carte aggiungono opzioni, modificano abilità o creano combo; non devono poter produrre un round in cui il giocatore non può fare nulla.

Categorie di carte:

|Categoria|Esempio|
|---|---|
|Azione|Colpo perforante|
|Movimento|Salto termico|
|Reazione|Scudo di emergenza|
|Modifica|La prossima abilità crea ghiaccio|
|Coordinazione|Un alleato può seguire il tuo percorso|
|Ambiente|Elettrifica le celle bagnate|
|Evocazione|Posiziona un drone|

|Categoria|Esempio|
|---|---|
|Manipolazione deck|Pesca, scarta, conserva|
|Rischio|Più potenza ma genera pericolo|

Generazione delle mappe:

```
Stanze progettate a mano
        +
Parametri semantici
        +
Regole di connessione
        +
Validator automatico
        =
Incontro procedurale controllato
```

Non generare liberamente ogni cella nelle prime versioni. Assemblare stanze curate riduce mappe irrisolvibili e mantiene leggibilità tattica.

### Modding, analytics, test e rischi
#### Strategia delle mod
Il supporto mod è l’ultima grande feature del prodotto core, ma le decisioni necessarie vengono prese sin dall’inizio:

1. ogni contenuto ha un ID namespaced;

2. i dati hanno una versione schema;

3. i riferimenti non dipendono da path fragili;

4. regole e presentazione sono separate;

5. le operazioni sono dichiarative;

6. il loader produce errori leggibili;

7. client e server confrontano il manifest;

8. il resolver non carica codice arbitrario.

#### Livelli di modding
|Livello|Contenuto|Matchmaking pubblico|
|---|---|---|
|Data mod|JSON per terreno, carte, effetti e incontri|Sì, se validata|
|Content mod|Mesh, audio, materiali e mappe cotte|Solo con stesso manifest|
|Game Feature mod|Modalità o pacchetto complesso|Server dedicati o allowlist|
|Native mod|Codice C++|No, salvo ambienti privati|

Il sistema Game Features offre una base utile per pacchetti di contenuto modulari, mentre Primary Data Asset e Asset Manager permettono caricamento e scaricamento controllato dei dati. 21

#### Manifest di una mod
```
{
"schemaVersion":1,
"namespace":"aaron.volcanic_pack",
"modId":"volcanic_pack",
"displayName":"Volcanic Pack",
"version":"1.0.0",
"engineRange":{
"minimum":"5.8.0",
"maximumExclusive":"5.9.0"
},
"gameVersionRange":{
"minimum":"1.0.0",
"maximumExclusive":"2.0.0"
},
"dependencies":[
{
"modId":"core",
"minimumVersion":"1.0.0"
}
],
"content":[
"terrain.lava_crust",
"effect.burning",
"card.thermal_jump"
],
"network":{
"deterministic":true,
"rankedAllowed":false
}
}
```

#### Terreno moddabile
```
{
"schemaVersion":1,
"namespace":"aaron.volcanic_pack",
"id":"terrain.lava_crust",
"type":"terrain",
"displayName":"Crosta lavica",
"tags":[
"Terrain.Hot",
"Terrain.Unstable"
],
"movement":{
"baseCost":3,
```

```
"blockedBy":[
"Unit.Tag.Fragile"
],
"costModifiers":[
{
"whenUnitHas":"Unit.Tag.Fireproof",
"add":-2
}
]
},
"onEnter":[
{
"effect":"effect.burning",
"stacks":1
}
],
"network":{
"deterministic":true
},
"limits":{
"maxInstancesPerMap":300
}
}
```

#### Carta roguelike moddabile
```
{
"schemaVersion":1,
"namespace":"core",
"id":"card.thermal_jump",
"type":"roguelike_card",
"displayName":"Salto termico",
"rarity":"uncommon",
"energyCost":1,
"requiresTags":[
"Character.Mobility"
],
"targeting":{
"kind":"cell",
"range":5
},
"effects":[
```

```
{
"op":"addTraversalEdge",
"edgeType":"Jump",
"durationTurns":1
},
{
"op":"applyStatus",
"status":"status.evasion",
"durationTurns":1
}
]
}
```

#### Operazioni autorizzate
Invece di eseguire script arbitrari, la prima API pubblica deve offrire un catalogo di operazioni:

```
applyDamage
applyHealing
applyStatus
removeStatus
moveUnit
pushUnit
pullUnit
spawnEntity
destroyEntity
modifyCellTags
addTraversalEdge
removeTraversalEdge
createHazard
drawCards
discardCards
gainEnergy
registerTrigger
```

Ogni operazione ha:

- schema noto;

- range numerici;

- numero massimo di target;

- ordine di esecuzione;

- comportamento deterministico;

- regole di rete;

- versione.

#### Handshake multiplayer
```
SERVER
```

```
  crea lista ordinata mod
```

```
  calcola hash del manifest
             |
             v
CLIENT
  calcola lo stesso hash
             |
       +-----+-----+
       |           |
     uguale      diverso
       |           |
     entra       rifiuto con
                 elenco differenze
```

Le partite classificate devono usare esclusivamente:

- contenuti core;

- mod firmate;

- allowlist server;

- schema approvato;

- stesso hash;

- nessun codice nativo esterno.

#### Analytics hooks
Unreal offre un’interfaccia analytics astratta con provider intercambiabili; esiste anche un provider di file utile per sviluppo e test che scrive eventi JSON, ma non è pensato come backend di produzione. 22

Eventi minimi:

|Evento|Dati non personali|
|---|---|
|`match_started`|modalità, mappa, build version|
|`planning_started`|round e durata timer|
|`plan_changed`|revisione e tipo modifica|
|`intent_viewed`|tipo intento alleato|
|`plan_locked`|tempo residuo|
|`ally_conflict_warned`|tipo conflitto|
|`path_preview_completed`|nodi visitati e tempo|
|`resolution_completed`|durata e numero eventi|
|`desync_detected`|fase e hash|
|`ability_used`|ability ID e contesto|
|`build_selected`|moduli, senza identità personale|
|`match_ended`|risultato e durata|

|Evento|Dati non personali|
|---|---|
|`card_drafted`|carta e alternative|
|`run_node_entered`|tipo nodo e atto|
|`mod_validation_failed`|codice errore e schema|

Gli analytics devono passare attraverso un adapter interno:

```
classIRTAnalyticsSink
{
public:
virtual~IRTAnalyticsSink()=default;
virtualvoidRecordEvent(
constFString&EventName,
constTMap<FString,FString>&Attributes)=0;
};
```

Il gameplay non deve conoscere il provider esterno.

#### Test plan
##### Test unitari
|Sistema|Test|
|---|---|
|Coordinate|Conversione world-grid|
|Grafo|Vicini e archi|
|A*|Percorso minimo e nessun percorso|
|Cost provider|Immunità, blocchi e modificatori|
|LOS|Ostacoli, altezza e cover|
|Resolver|Ordine eventi|
|Movimento|Collisioni e swap|
|Status|Stack e durata|
|Mod loader|Schema e dipendenze|
|Save|Versione e migrazione|
|Deck|Pesca, scarto e reshuffle|

##### Test deterministici
Per ogni scenario:

```
Load fixture
Apply seed
Submit plans
Resolve phase
Hash state
Compare golden hash
```

##### Testare:

- stesso risultato in cento esecuzioni;

- ordine diverso di inserimento degli Actor;

- frame rate differente;

- server headless;

- replay da log;

- mod caricate in ordine differente ma normalizzato.

##### Test di rete
|Caso|Verifica|
|---|---|
|Latenza alta|Submit e conferma coerenti|
|Packet loss|Nessun piano perso definitivamente|
|Reconnect|Snapshot corretto|
|Client malevolo|Piano illegale rifiutato|
|Enemy inspection|Nessun intento privato presente|
|Timeout|Server applica fallback|
|Version mismatch|Connessione rifiutata chiaramente|
|Mod mismatch|Differenze elencate|
|Host migration|Fuori scope iniziale o formalmente gestita|

Networking Insights può analizzare il traffico di rete e le proprietà replicate, mentre Unreal Insights 23 raccoglie dati di tracing e profiling ad alta frequenza.

##### Test funzionali
Unreal supporta Functional Test Actor eseguibili nei livelli; l’Unreal Test Adapter di Visual Studio consente inoltre di individuare, eseguire e debuggare test Unreal dall’IDE. 24

Mappe test dedicate:

```
Test_Pathfinding
Test_Multilayer
Test_LineOfSight
Test_Cover
```

```
Test_SimultaneousCollision
Test_TeamIntentPrivacy
Test_EnvironmentInteractions
Test_ModValidation
Test_RoguelikeSave
```

#### Crash e osservabilità
Crash Reporter può essere incluso nelle build pacchettizzate e configurato per inviare report a un endpoint controllato dal progetto. 25

Log categories:

```
DECLARE_LOG_CATEGORY_EXTERN(LogRTGrid,Log,All);
DECLARE_LOG_CATEGORY_EXTERN(LogRTTurn,Log,All);
DECLARE_LOG_CATEGORY_EXTERN(LogRTNetwork,Log,All);
DECLARE_LOG_CATEGORY_EXTERN(LogRTMod,Log,All);
```

Ogni round deve poter generare un log compatto:

```
MatchId
Round
MapRevision
Seed
PlanHashes
ResolvedEventHashes
FinalStateHash
```

#### Gate di release
La release candidate non può procedere se esiste uno dei seguenti problemi:

|Gate|Requisito|
|---|---|
|Crash|Nessun blocker riproducibile|
|Privacy|Nessun leak del piano nemico|
|Determinismo|Nessun desync nella suite golden|
|Networking|Riconnessione validata|
|Accessibilità|Input rimappabile e informazioni non solo cromatiche|
|Tutorial|Completamento end-to-end|
|Performance|Budget rispettati sulle macchine target|
|Modding|Errori schema non causano crash|
|Salvataggi|Migrazioni testate|

|Gate|Requisito|
|---|---|
|Packaging|Build Shipping installabile|
|Analytics|Eventi verificati e disattivabili secondo configurazione|
|Crash reporting|Pipeline testata|

#### Rischi principali
|Rischio|Impatto|Mitigazione|
|---|---|---|
|Scope troppo ampio|Alto|Vertical slice piccola e feature gate|
|Curva C++|Medio|Curriculum incrementale e API Blueprint|
|Desync|Critico|Server authority, interi, seed e hash|
|Leak dei piani|Critico|Replica per team e audit automatici|
|Troppe entità per cella|Alto|Strutture dati chunked e instancing|
|Blueprint ingestibili|Alto|Regole in C++ e grafi piccoli|
|GAS troppo complesso presto|Medio|Abilità manuali prima, migrazione controllata|
|Bilanciamento combinatorio|Alto|Budget build, bot e analytics|
|IA apparentemente sleale|Alto|Snapshot senza piani umani|
|Mod malevole|Alto|API dichiarativa e nessun native code pubblico|
|Aggiornamento Unreal|Medio|Version pinning e branch upgrade|
|Dipendenza plugin C#|Alto|Plugin opzionale, core C++ indipendente|
|Procedurale incoerente|Alto|Stanze curate e validator|
|Roguelike troppo casuale|Medio|Azioni base sempre disponibili|
|Costi asset|Alto|Placeholder fino alla beta core|

#### Decisione architetturale conclusiva
Il cuore di RefactorTactics deve poter funzionare senza grafica:

```
Snapshot iniziale
      +
Piani dei giocatori
      +
Seed
      =
Eventi risolti
      +
Snapshot finale
```

```
      +
Hash
```

Unreal Engine fornisce networking, UI, asset management, Gameplay Ability System, modular gameplay, strumenti IA, profiling e packaging. Tuttavia, **le regole tattiche, il grafo semantico e il resolver devono appartenere al progetto** , non essere nascosti dentro Actor, animazioni o Blueprint di livello.

Questa impostazione permette di usare lo stesso core per:

```
PvP competitivo
Bot e training
Replay
Editor mappe
Test automatici
Server dedicato
Mod
Co-op roguelike
Deckbuilding
```

È il punto che rende RefactorTactics non soltanto un gioco ispirato ad Atlas Reactor, ma una piattaforma tattica coerente, studiabile e realmente espandibile.

> 1 Unreal Engine 5.8 is now available

https://www.unrealengine.com/news/unreal-engine-5-8-is-now-available?utm_source=chatgpt.com

> 2 Blueprint Overview | Unreal Engine 4.27 Documentation https://dev.epicgames.com/documentation/en-us/unreal-engine/blueprint-overview? application_version=4.27&utm_source=chatgpt.com

> 3 Gameplay Ability System | Unreal Engine 4.27 ...

https://dev.epicgames.com/documentation/en-us/unreal-engine/gameplay-ability-system? application_version=4.27&utm_source=chatgpt.com

> 4 https://dev.epicgames.com/documentation/unreal-engine/gameplay-framework-in-unreal-engine https://dev.epicgames.com/documentation/unreal-engine/gameplay-framework-in-unreal-engine

> 5 https://dev.epicgames.com/documentation/unreal-engine/game-features-and-modular-gameplayin-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/game-features-and-modular-gameplay-in-unreal-engine

> 6 Replicating Variables in Blueprints | Unreal Engine 4.27 ... https://dev.epicgames.com/documentation/en-us/unreal-engine/replicating-variables-in-blueprints? application_version=4.27&utm_source=chatgpt.com

> 7 https://dev.epicgames.com/documentation/en-us/unreal-engine/umg-ui-designer-for-unrealengine?application_version=5.2

https://dev.epicgames.com/documentation/en-us/unreal-engine/umg-ui-designer-for-unreal-engine?application_version=5.2

> 8 https://dev.epicgames.com/documentation/unreal-engine/behavior-trees-in-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/behavior-trees-in-unreal-engine

9 https://dev.epicgames.com/documentation/unreal-engine/hardware-and-software-specificationsfor-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/hardware-and-software-specifications-for-unreal-engine

> 10 https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-developmentenvironment-for-cplusplus-projects-in-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/setting-up-visual-studio-development-environment-for-cplusplusprojects-in-unreal-engine

> 11 https://dev.epicgames.com/documentation/unreal-engine/install-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/install-unreal-engine

> 12 https://learn.microsoft.com/en-us/visualstudio/gamedev/unreal/get-started/vs-tools-unreal-install

https://learn.microsoft.com/en-us/visualstudio/gamedev/unreal/get-started/vs-tools-unreal-install

> 13 https://git-lfs.com/

https://git-lfs.com/

> 14 https://www.unrealengine.com/ue-on-github

https://www.unrealengine.com/ue-on-github

> 15 https://dev.epicgames.com/documentation/unreal-engine/building-unreal-engine-from-source https://dev.epicgames.com/documentation/unreal-engine/building-unreal-engine-from-source

> 16 https://github.com/UnrealSharp/UnrealSharp

https://github.com/UnrealSharp/UnrealSharp

> 17 https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-4-for-unity-

##### developers?application_version=4.27

https://dev.epicgames.com/documentation/en-us/unreal-engine/unreal-engine-4-for-unity-developers? application_version=4.27

> 18 https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-cpp-quick-start

https://dev.epicgames.com/documentation/unreal-engine/unreal-engine-cpp-quick-start

> 19 https://dev.epicgames.com/documentation/unreal-engine/lyra-sample-game-in-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/lyra-sample-game-in-unreal-engine

> 20 https://dev.epicgames.com/documentation/unreal-engine/using-lyra-with-epic-online-services-in-

##### unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/using-lyra-with-epic-online-services-in-unreal-engine

##### 21 unreal.PrimaryDataAsset

https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/PrimaryDataAsset? application_version=5.4&utm_source=chatgpt.com

> 22 https://dev.epicgames.com/documentation/unreal-engine/in-game-analytics-for-unreal-engine https://dev.epicgames.com/documentation/unreal-engine/in-game-analytics-for-unreal-engine

> 23 https://dev.epicgames.com/documentation/unreal-engine/unreal-insights-in-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/unreal-insights-in-unreal-engine

##### 24 unreal.FunctionalTest

https://dev.epicgames.com/documentation/en-us/unreal-engine/python-api/class/FunctionalTest? application_version=5.0&utm_source=chatgpt.com

> 25 https://dev.epicgames.com/documentation/unreal-engine/crash-reporting-in-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/crash-reporting-in-unreal-engine

---

## PRD, roadmap e percorso didattico — roadmap, lezioni, rischi e artefatti stampabili

### Roadmap dal tutorial al prodotto
Roadmap Gantt

#### Timeline semplificata
<!-- Start of picture text -->
RefactorTactics — roadmap indicativa per piccolo team<br>Fondazioni Setup e architetturaTutorial base<br>Griglia e turn loop<br>Prototipo Mappa semantica e A*<br>LOS e combattimento<br>Multiplayer Planning condiviso Vertical slice<br>Alpha e strumenti<br>Produzione Beta, QA e online<br>Mod kit e scripting<br>2026 2027 2027 2027 2027 2028 2028<br><!-- End of picture text -->

Le date del diagramma sono illustrative. I range seguenti sono le vere stime operative.

#### Milestone e criteri di successo
|Milestone|Stima|Deliverable|Dipendenze|Criteri di successo|
|---|---|---|---|---|
|Fondazioni|1–2<br>settimane|Repository, engine,<br>IDE, build e<br>convenzioni|Nessuna|Progetto compilabile<br>su un secondo PC|
|Tutorial base|6–10<br>settimane|Camera, input, tile<br>selection, primo Actor<br>C++|Fondazioni|Piccola scena<br>giocabile e versionata|
|Turn loop<br>locale|4–8<br>settimane|Planning, conferma e<br>risoluzione locale|Tutorial base|Match di almeno<br>cinque turni senza<br>soft-lock|
|Griglia<br>multilivello|6–10<br>settimane|Layer, archi verticali e<br>debug tool|Griglia base|Percorsi corretti tra<br>almeno tre piani|
|Pathfinding<br>semantico|5–9<br>settimane|A*, provider, cache e<br>test|Griglia<br>multilivello|Suite di test completa<br>e budget<br>prestazionale<br>rispettato|
|Combat<br>sandbox|10–16<br>settimane|LOS, cover, danni, stati<br>e ambiente|Pathfinding|Scenario tattico<br>completo con quattro<br>unità|
|Personaggi<br>modulari|8–14<br>settimane|Kit, specializzazioni,<br>augment e gadget|Combat<br>sandbox|Almeno quattro eroi e<br>due build significative<br>ciascuno|
|Planning<br>multiplayer|8–14<br>settimane|Server authority, intent<br>alleati, commit|Turn loop|Due team giocano<br>senza leak del<br>planning|
|Vertical slice|12–20<br>settimane|Una mappa, otto eroi,<br>UI, audio, onboarding|Tutti i sistemi<br>P0|Sessione completa di<br>20–30 minuti|
|Alpha|20–36<br>settimane|Contenuti, tool,<br>matchmaking base,<br>telemetria|Vertical slice|Feature complete e<br>bilanciabile|

|Milestone|Stima|Deliverable|Dipendenze|Criteri di successo|
|---|---|---|---|---|
|Beta|16–28<br>settimane|Ottimizzazione,<br>accessibilità, sicurezza<br>e QA|Alpha|Target prestazioni e<br>rete rispettati|
|Mod kit|16–28<br>settimane|Editor, validator, JSON,<br>Lua e packaging|API gameplay<br>stabile|Mod di esempio<br>installabile senza<br>ricompilare il gioco|
|Release<br>candidate|6–12<br>settimane|Stabilizzazione,<br>documentazione e<br>distribuzione|Beta e mod<br>kit|Zero blocker e<br>procedure di rollback<br>pronte|

#### Stime complessive
|Scenario|Impegno|Intervallo plausibile|
|---|---|---|
|Sviluppatore singolo|10–15 ore alla settimana|24–42 mesi|
|Sviluppatore singolo full-time|35–40 ore alla settimana|18–30 mesi|
|Team piccolo|3–5 persone full-time|14–24 mesi|
|Team con produzione contenuti separata|6–10 persone|12–20 mesi|

Le stime non includono certificazione console, localizzazione estesa, infrastruttura live-service, supporto clienti o una grande campagna narrativa.

#### Gate del vertical slice
Il vertical slice deve contenere soltanto:

- una modalità;

- una mappa multilivello;

- otto personaggi;

- due specializzazioni per personaggio;

- un set limitato di terreni;

- planning privato tra alleati;

- un tutorial di onboarding;

- bot di prova;

- replay tecnico o event log;

- nessun editor mod pubblico.

Il progetto non dovrebbe entrare in produzione estesa finché il vertical slice non dimostra:

|Area|Target iniziale|
|---|---|
|Durata partita|20–35 minuti|
|Comprensibilità del<br>planning|Almeno l’80% dei tester interpreta correttamente il piano alleato|
|Stabilità interna|Almeno il 95% delle sessioni termina senza crash|

|Area|Target iniziale|
|---|---|
|Tempo di planning|La maggioranza dei turni si conclude entro il timer|
|Pathfinding|Nessuna destinazione raggiungibile dichiarata irraggiungibile|
|Privacy|Nessun intent avversario in memoria applicativa, actor o log del<br>client|
|Bilanciamento|Nessuna build scelta in oltre il 60% dei casi per pura superiorità|
|Mappa|Almeno tre interazioni ambientali usate spontaneamente per<br>partita|

### Sequenza delle lezioni
#### Ambiente di sviluppo
La documentazione ufficiale raccomanda per Unreal Engine un sistema Windows 10 o 11 a 64 bit, CPU quad-core, 32 GB di RAM e GPU compatibile DirectX 11 o 12 con almeno 8 GB di memoria grafica. Per compilazioni frequenti e mappe complesse è prudente utilizzare 64 GB di RAM, SSD NVMe e una GPU 14 con 12 GB di VRAM, ma questi ultimi sono consigli pratici del progetto, non requisiti ufficiali.

Configurazione Windows:

1. Installare Epic Games Launcher.

2. Installare Unreal Engine 5.8.1.

3. Installare Visual Studio 2022.

4. Selezionare il workload **Game development with C++** .

5. Installare Windows SDK e Visual Studio Tools for Unreal Engine.

6. Installare Git e Git LFS.

7. Creare una cartella senza spazi eccessivi, per esempio `D:\Dev\RefactorTactics` .

8. Creare un progetto `Games` → `Blank` → `C++` .

- Compilare la configurazione `Development Editor | Win64` .

10. Creare il primo commit.

Epic documenta sia l’installazione del motore sia la configurazione di Visual Studio per i progetti C++. 15

Hardware consigliato per il percorso:

|Componente|Minimo didattico|Consigliato|
|---|---|---|
|CPU|4 core moderni|8–16 core|
|RAM|32 GB|64 GB|
|GPU|8 GB VRAM|12 GB VRAM|
|Disco|300 GB liberi|1 TB NVMe libero|
|Sistema|Windows 10/11 64 bit|Windows 11|
|IDE|Visual Studio 2022|Visual Studio 2022 o Rider|

`.gitignore` iniziale:

```
Binaries/
DerivedDataCache/
Intermediate/
Saved/
.vs/
*.sln
.vscode/
```

Cartelle da versionare:

```
Config/
Content/
Plugins/
Source/
RefactorTactics.uproject
```

File binari Unreal come `.uasset` e `.umap` devono essere gestiti con Git LFS:

```
gitlfsinstall
gitlfstrack"*.uasset"
gitlfstrack"*.umap"
gitadd.gitattributes
```

#### Ponte mentale da C# a C++ Unreal
|C# / Unity|Unreal|
|---|---|
|`GameObject`|`AActor`|
|`MonoBehaviour`|`UActorComponent` o<br>`AActor`|
|Prefab|Blueprint Class|
|`ScriptableObject`|`UDataAsset` /<br>`UPrimaryDataAsset`|
|Inspector field|`UPROPERTY(EditAnywhere)`|
|UnityEvent|Delegate Unreal|
|Coroutine|Timer, latent action o task|
|`List<T>`|`TArray<T>`|
|`Dictionary<K,V>`|`TMap<K,V>`|
|`HashSet<T>`|`TSet<T>`|
|`null` managed|`nullptr` e validità<br>`UObject`|

|C# / Unity|Unreal|
|---|---|
|Package assembly|Module Unreal|

Primo esempio copiabile:

```
#pragma once
#include"CoreMinimal.h"
#include"GameFramework/Actor.h"
#include"TutorialTile.generated.h"
UCLASS()
classREFACTORTACTICS_APIATutorialTile:publicAActor
{
GENERATED_BODY()
public:
ATutorialTile();
UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Tile")
int32MovementCost=10;
UPROPERTY(EditAnywhere,BlueprintReadWrite,Category="Tile")
boolbBlocksMovement=false;
UFUNCTION(BlueprintCallable,Category="Tile")
boolCanEnter()const;
};
```

```
#include"TutorialTile.h"
ATutorialTile::ATutorialTile()
{
PrimaryActorTick.bCanEverTick=false;
}
boolATutorialTile::CanEnter()const
{
return!bBlocksMovement;
}
```

Passi:

1. Creare `TutorialTile.h` .

2. Creare `TutorialTile.cpp` .

3. Compilare.

4. Nell’Editor creare `BP_TutorialTile` come child della classe.

5. Aggiungere una Static Mesh.

- Modificare `MovementCost` dal pannello Details.

7. Posizionare più tile nella scena.

Risultato: il codice definisce regole e proprietà, mentre Blueprint definisce aspetto e configurazione.

#### Piano didattico completo
|Lezione|Obiettivo|Output|
|---|---|---|
|Ambiente|Installare engine, IDE, Git LFS e compilare|Build pulita|
|Modello Unreal|Imparare Actor, Component, UObject e<br>reflection|Actor C++ modificabile da<br>Blueprint|
|Camera tattica|Pan, zoom, rotazione e selezione|Camera controllabile|
|Enhanced Input|Separare input da logica|Mapping context funzionante|
|Tile picking|Raycast e coordinate di griglia|Tile evidenziata al passaggio|
|Griglia base|Generare nodi e visualizzarli|Griglia selezionabile|
|Griglia<br>multilivello|Layer e archi verticali|Percorso tra piani|
|Tile semantiche|Tag, costi, hazard e Data Assets|Acqua, fuoco e ghiaccio|
|A* base|Open set, costo ed euristica|Percorso minimo|
|Cost provider|Modificatori per unità e ambiente|Percorsi diversi per<br>personaggio|
|LOS|Visibilità e cover separate|Preview di tiro|
|Turn<br>orchestrator|Planning, commit e resolve|Turno locale completo|
|Personaggi|Kit, specializzazioni e augment|Due build per eroe|
|UI planning|Ghost, spline, AoE e intent|Piano comprensibile|
|Multiplayer|Server, RPC e PlayerState|Match a due client|
|Privacy team|Inoltro intent solo agli alleati|Test leak negativo|
|Test|Automation, PIE, emulazione rete|Suite smoke|
|Data mod|Manifest e JSON registry|Nuovo terreno esterno|
|Editor mod|Tool, validazione e packaging|Mod creata dall’Editor|
|Lua sandbox|Script e capability|Trigger mod sicuro|

#### Lezione sulla camera tattica
Creare un `Pawn` con:

```
Scene Root
├── Spring Arm
└── Camera
```

Input:

|Azione|Binding|
|---|---|
|`IA_CameraMove`|WASD|
|`IA_CameraZoom`|Rotella mouse|
|`IA_CameraRotate`|Q/E|
|`IA_Select`|Tasto sinistro|
|`IA_Cancel`|Tasto destro|

Logica Blueprint iniziale:

```
IA_CameraMove
```

- `→ Get Control Rotation`

- `→ Forward/Right Vector`

- `→ Add Actor World Offset`

In questa fase è corretto usare Blueprint. La conversione in C++ serve soltanto quando la logica diventa condivisa o richiede test automatici.

#### Lezione sulla griglia multilivello
Struttura minima:

```
USTRUCT(BlueprintType)
structFTileEdge
```

```
{
```

```
GENERATED_BODY()
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FTileIdTo;
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32BaseCost=10;
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FGameplayTagContainerRequirements;
```

```
};
```

```
USTRUCT(BlueprintType)
```

```
structFTileNode
```

```
{
GENERATED_BODY()
```

```
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FTileIdId;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32HeightSteps=0;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
int32BaseMoveCost=10;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
FGameplayTagContainerTags;
UPROPERTY(EditAnywhere,BlueprintReadWrite)
TArray<FTileEdge>Edges;
};
```

Passi:

1. Generare una griglia sul layer zero.

2. Aggiungere una seconda griglia sul layer uno.

3. Collegare manualmente due celle con un arco `Traversal.Stairs` .

4. Aggiungere il requisito `Movement.Climb` .

5. Assegnare a un personaggio il tag richiesto.

- Verificare che il personaggio possa salire.

7. Rimuovere il tag.

- Verificare che il percorso non utilizzi più la scala.

9. Aggiungere un ascensore con requisito `State.Powered` .

10. Modificare lo stato durante il turno e invalidare la revisione del grafo.

#### Lezione su A*
Versione iniziale, volutamente semplice:

```
int32Heuristic(constFTileId&A,constFTileId&B)
{
return10*(
FMath::Abs(A.X-B.X)+
FMath::Abs(A.Y-B.Y)+
FMath::Abs(A.Layer-B.Layer)
);
}
```

Costo di un vicino:

```
int32CalculateTraversalCost(
constFTileNode&Current,
```

```
constFTileEdge&Edge,
constFTileNode&Next)
{
returnEdge.BaseCost+Next.BaseMoveCost;
```

```
}
```

Prima estensione:

```
int32CalculateTraversalCost(
constFTileNode&Current,
constFTileEdge&Edge,
constFTileNode&Next,
constFUnitTraversalContext&Unit,
constTArray<ITraversalCostProvider*>&Providers)
{
int32Cost=Edge.BaseCost+Next.BaseMoveCost;
for(constITraversalCostProvider*Provider:Providers)
{
if(!Provider->CanTraverse(Current,Next,Unit))
{
returnMAX_int32;
}
Cost+=Provider->GetAdditionalCost(
Current,
Next,
Unit
);
}
returnFMath::Max(1,Cost);
}
```

Test necessari:

```
Percorso su terreno normale
Percorso attorno a ostacolo
Percorso tra due layer
Scala senza requisito
Scala con requisito
Acqua per umano
Acqua per unità anfibia
Cella resa invalida durante la partita
Start uguale a goal
Goal irraggiungibile
```

#### Lezione sul turn orchestrator
Stati:

```
UENUM(BlueprintType)
enumclassETurnPhase:uint8
{
Planning,
Locked,
ResolvingMovement,
ResolvingReactions,
ResolvingActions,
ResolvingEnvironment,
Cleanup
};
```

Il server conserva:

```
USTRUCT()
structFCommittedTurnIntent
{
GENERATED_BODY()
UPROPERTY()
int32PlayerId=INDEX_NONE;
```

```
UPROPERTY()
int32UnitId=INDEX_NONE;
```

```
UPROPERTY()
TArray<FTileId>Path;
UPROPERTY()
FNameAbilityId;
UPROPERTY()
FTileIdTargetTile;
UPROPERTY()
int32Sequence=0;
};
```

Passi:

1. Avviare `Planning` .

- Accettare preview non definitive.

3. Accettare il commit.

4. Validare il piano.

- Impedire modifiche dopo il lock.

6. Ordinare gli eventi per fase e priorità.

7. Risolvere il movimento.

8. Risolvere azioni e ambiente.

9. Salvare l’event log.

10. Avviare il turno successivo.

#### Lezione sul planning multiplayer
L’architettura client-server e il sistema di replicazione di Unreal sono documentati come base del multiplayer; il server deve essere l’autorità sullo stato condiviso. 16

Struttura dell’intent:

```
USTRUCT(BlueprintType)
structFPlanningIntent
{
GENERATED_BODY()
UPROPERTY(BlueprintReadWrite)
int32Sequence=0;
UPROPERTY(BlueprintReadWrite)
FNameAbilityId;
UPROPERTY(BlueprintReadWrite)
TArray<FTileId>PlannedPath;
UPROPERTY(BlueprintReadWrite)
FTileIdTargetTile;
UPROPERTY(BlueprintReadWrite)
boolbCommitted=false;
};
```

Passi:

1. Aggiungere `TeamId` al `PlayerState` .

2. Impostarlo esclusivamente sul server.

3. Costruire la preview localmente.

- Inviare l’intent al server con RPC non affidabile.

- Verificare ownership e team.

6. Inoltrare soltanto ai `PlayerController` alleati.

7. Disegnare spline e ghost sul client alleato.

8. Eliminare la preview quando cambia turno.

- Utilizzare un RPC affidabile per il commit.

   - Ispezionare il client nemico per verificare che il payload non esista.

Il file nel kit contiene un metodo `IsSameTeam` volutamente provvisorio. Deve essere sostituito da un vero controllo su `TeamId` prima di qualsiasi test online.

#### Lezione sulla UI
Widget principali:

- `WBP_TacticalHUD ├── WBP_TurnTimer ├── WBP_AbilityBar`

- `├── WBP_TeamPlanningStatus`

- `├── WBP_IntentLabel`

- `├── WBP_ConflictWarning`

- `└── WBP_TileInspector`

##### Visualizzazioni nel mondo:

```
BP_PlannedPathSpline
BP_AllyGhost
BP_TargetAreaPreview
BP_FacingArrow
BP_ContextPing
```

Regole di accessibilità:

- non utilizzare solo il colore;

- assegnare anche pattern, icone o stili di linea;

- consentire dimensionamento della UI;

- mostrare testo per i conflitti;

- supportare rimappatura degli input;

- limitare il numero di preview simultanee;

- attenuare i piani non selezionati;

- permettere di isolare il piano di un singolo alleato.

#### Lezione sui test
Unreal offre Automation Test Framework per test automatici e Gauntlet per orchestrare sessioni più complesse, incluse configurazioni con server e più client. PIE consente di testare più giocatori dall’Editor, mentre la network emulation permette di simulare latenza e perdita di pacchetti. 17

Matrice minima:

|Test|Ambiente|
|---|---|
|A* e cost provider|Automation Test|
|Serialize/deserialize mod|Automation Test|
|Turn resolve deterministico|Automation Test|
|Due client e server|PIE|
|Due squadre e planning privato|PIE o Gauntlet|

|Test|Ambiente|
|---|---|
|100–200 ms latenza|Network emulation|
|2–5% packet loss|Network emulation|
|Disconnect durante planning|Gauntlet|
|Manifest mod differente|Packaged build|
|Dedicated server senza rendering|Packaged server|

Esecuzione indicativa:

```
UnrealEditor-Cmd.exe RefactorTactics.uproject
-ExecCmds="Automation RunTests RefactorTactics"
-unattended
```

```
-TestExit="Automation Test Queue Empty"
```

I parametri precisi possono cambiare con la versione del motore; la documentazione ufficiale descrive i comandi e il report dei test. 18

#### Lezione sulle mod
Ordine corretto:

```
Registry interno
→ Data Assets
→ JSON schema
→ Data mod locali
→ Content package
→ Editor mod
→ Lua sandbox
```

```
→ Catalogo pubblico
```

Il primo obiettivo non è permettere qualunque modifica, ma registrare nuovi dati senza modificare il core:

```
USTRUCT(BlueprintType)
structFTerrainDefinition
{
```

```
GENERATED_BODY()
```

```
UPROPERTY(EditAnywhere,BlueprintReadOnly)
FNameId;
```

```
UPROPERTY(EditAnywhere,BlueprintReadOnly)
FTextDisplayName;
```

```
UPROPERTY(EditAnywhere,BlueprintReadOnly)
FGameplayTagContainerTags;
```

```
UPROPERTY(EditAnywhere,BlueprintReadOnly)
int32BaseMoveCost=10;
UPROPERTY(EditAnywhere,BlueprintReadOnly)
boolbBlocksMovement=false;
UPROPERTY(EditAnywhere,BlueprintReadOnly)
boolbBlocksVision=false;
};
```

Validator:

```
ID univoco
Versione schema valida
Tag registrati
Dipendenze presenti
Nessun costo negativo
Nessun riferimento circolare proibito
Asset consentiti
Script entro limiti
Hash calcolato
```

Game Features e Modular Gameplay possono essere studiati per modularizzare contenuti, ma la documentazione li presenta ancora con avvertenze relative allo stato Beta; non dovrebbero diventare automaticamente la base del sistema mod senza una spike. 19

### Rischi, qualità e learning curve
#### Rischi tecnici
|Rischio|Probabilità|Impatto|Mitigazione|
|---|---|---|---|
|Scope eccessivo|Alta|Alto|Un’unica modalità, una mappa e otto eroi nel<br>vertical slice|
|Troppe combinazioni|Alta|Alto|Gameplay Tags, contratti, budget effetti e test<br>automatici|
|Leak del planning|Media|Critico|Payload team-only, packet inspection e test con<br>client nemico|
|Dipendenza da plugin<br>C#|Alta|Medio|Spike di massimo due settimane e core<br>completamente nativo|
|Desync della<br>risoluzione|Media|Alto|Server authority, costi interi, RNG centralizzato<br>ed event log|

|Rischio|Probabilità|Impatto|Mitigazione|
|---|---|---|---|
|Pathfinding lento|Media|Alto|Profiling, cache per revisione e invalidazione<br>regionale|
|LOS incoerente|Media|Alto|Query tattica condivisa da preview e server|
|Mod malevole|Media|Critico|Sandbox, capability, timeout e divieto di native<br>code pubblico|
|API mod instabile|Alta|Alto|Mod support dopo beta tecnica e<br>versionamento semantico|
|Tooling insufficiente|Media|Alto|Debug overlay e validator sviluppati insieme ai<br>sistemi|
|Bilanciamento<br>ingestibile|Alta|Alto|Slot, incompatibilità e telemetria sulle build|
|Rete fragile|Media|Alto|PIE, emulazione, Gauntlet e dedicated server<br>test|

#### Learning curve per uno sviluppatore C
Con un impegno di 10–15 ore settimanali:

|Area|Tempo indicativo|
|---|---|
|Editor e Blueprint|2–4 settimane|
|Actor, Component e UObject|2–4 settimane|
|C++ Unreal quotidiano|4–8 settimane|
|Memory model e reflection|4–8 settimane|
|UMG e input|3–6 settimane|
|Pathfinding e test|6–12 settimane|
|Networking e lifecycle|8–14 settimane|
|Tool per l’Editor|10–16 settimane|
|Packaging e mod|12–20 settimane|

Queste fasi si sovrappongono. Non è necessario “imparare tutto il C++” prima di iniziare: è più efficace imparare un sottoinsieme Unreal mirato:

```
UCLASS, USTRUCT, UENUM
UPROPERTY, UFUNCTION
AActor, UObject, UActorComponent
TArray, TMap, TSet
TObjectPtr e riferimenti deboli
Delegate
ModuleRules
```

```
Replication e RPC
Automation Test
```

#### Regole di qualità
Il core tattico deve essere:

- deterministico sul server;

- indipendente dagli effetti visivi;

- interrogabile attraverso API pure dove possibile;

- serializzabile in event log;

- testabile senza avviare una mappa completa;

- data-driven;

- compatibile con revisioni del grafo;

- protetto da input non validi;

- misurabile tramite profiling.

#### Criteri prestazionali iniziali
I valori seguenti sono target interni da misurare e adattare:

|Sistema|Target prototipo|
|---|---|
|A* singolo|p95 inferiore a 2 ms sulla mappa target|
|Batch preview|Inferiore a 8 ms per frame, con throttling|
|Aggiornamento intent|5–10 Hz durante modifica|
|Payload intent|Idealmente sotto 2–4 KB|
|Turn resolve|Inferiore a 100 ms lato server|
|LOS batch|Cache per turno e invalidazione mirata|
|Script mod|Budget per callback e interruzione automatica|
|Frame rate PC|60 FPS su hardware target|
|Server dedicato|Nessuna dipendenza da rendering|

Questi non sono standard esterni: sono budget iniziali del progetto. Devono essere validati con una mappa rappresentativa, non con una scena vuota.

#### Definition of done per il supporto mod
Il supporto mod può essere considerato pronto quando:

- una mod aggiunge un terreno senza ricompilare;

2. una mod aggiunge una nuova abilità usando soltanto dati approvati;

- una mod scriptata non può leggere filesystem o rete liberamente;

- uno script in loop viene terminato;

- una mod con API incompatibile viene rifiutata con un errore leggibile;

6. server e client confrontano gli hash;

7. una mod può essere disabilitata senza corrompere i salvataggi;

8. il validator produce un report;

- il packaging genera un artefatto separato;

10. la documentazione include almeno una mod completa di esempio.

### Artefatti stampabili e starter pack
Il kit prodotto include:

|File|Scopo|
|---|---|
|`RefactorTactics_PRD_Roadmap_Tutorial_A4.pdf`|Documento principale pronto per la<br>stampa|
|`RefactorTactics_PRD_Roadmap_Tutorial_A4.html`|Versione modificabile e stampabile da<br>browser|
|`architettura_refactortactics.png`|Diagramma a 300 DPI|
|`architettura_refactortactics.svg`|Diagramma vettoriale|
|`roadmap_gantt.png`|Roadmap a 300 DPI|
|`roadmap_gantt.svg`|Roadmap vettoriale|
|`core_loop_turno.png`|Core loop a 300 DPI|
|`core_loop_turno.svg`|Core loop vettoriale|
|`TileTypes.h`|Tipi copiabili per tile e archi|
|`SemanticPathfinder.h/.cpp`|A* didattico|
|`PlanningPlayerController.h/.cpp`|Esempio RPC per planning alleato|
|`RefactorTactics.Build.cs`|Dipendenze iniziali del modulo|
|`mod.json`|Manifest mod|
|`terrain_electrified_water.json`|Esempio data mod|
|`electrified_water.lua`|Esempio di script sandboxato|

Download separati dei diagrammi:

- <u>Architettura PNG</u>

- <u>Architettura SVG</u>

- <u>Roadmap PNG</u>

- <u>Roadmap SVG</u>

- <u>Core loop PNG</u>

- <u>Core loop SVG</u>

La versione PDF contiene anche la bibliografia con i collegamenti ufficiali a installazione, requisiti hardware, Visual Studio, framework di gameplay, networking, relevancy, Gameplay Tags, Data Assets, Asset Manager, plugin, packaging, test, UnrealCLR, UnLua, AngelScript e letteratura sul pathfinding.

> 1 https://forums.unrealengine.com/t/unreal-engine-5-8-released/2729274 https://forums.unrealengine.com/t/unreal-engine-5-8-released/2729274

> 2 https://ui.adsabs.harvard.edu/abs/1968IJSSC...4..100H/abstract https://ui.adsabs.harvard.edu/abs/1968IJSSC...4..100H/abstract

> 3 https://www.cs.cmu.edu/~ggordon/likhachev-etal.anytime-dstar.pdf

https://www.cs.cmu.edu/~ggordon/likhachev-etal.anytime-dstar.pdf

> 4 Actor Relevancy in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/actor-relevancy-in-unreal-engine?utm_source=chatgpt.com

> 5 https://dev.epicgames.com/documentation/unreal-engine/experimental-features

https://dev.epicgames.com/documentation/unreal-engine/experimental-features

##### 6 Using Gameplay Tags in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/using-gameplay-tags-in-unreal-engine?utm_source=chatgpt.com

> 7 https://dev.epicgames.com/documentation/unreal-engine/gameplay-framework-in-unreal-engine https://dev.epicgames.com/documentation/unreal-engine/gameplay-framework-in-unreal-engine

> 8 https://github.com/nxrighthere/UnrealCLR

https://github.com/nxrighthere/UnrealCLR

> 9 MonoUE | Mono for Unreal Engine is a plugin for Unreal ...

https://mono-ue.github.io/?utm_source=chatgpt.com

> 10 https://github.com/tencent/unlua

https://github.com/tencent/unlua

##### Unreal Engine Angelscript - Hazelight

https://angelscript.hazelight.se/?utm_source=chatgpt.com

> 12 https://github.com/ufna/VaRest

https://github.com/ufna/VaRest

> 13 https://dev.epicgames.com/documentation/unreal-engine/plugins-in-unreal-engine https://dev.epicgames.com/documentation/unreal-engine/plugins-in-unreal-engine

14 https://dev.epicgames.com/documentation/unreal-engine/hardware-and-software-specifications-

##### for-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/hardware-and-software-specifications-for-unreal-engine

> 15 https://dev.epicgames.com/documentation/unreal-engine/install-unreal-engine https://dev.epicgames.com/documentation/unreal-engine/install-unreal-engine

##### 16 Networking and Multiplayer in Unreal Engine

https://dev.epicgames.com/documentation/unreal-engine/networking-and-multiplayer-in-unreal-engine? utm_source=chatgpt.com

> 17 https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unrealengine

https://dev.epicgames.com/documentation/unreal-engine/automation-test-framework-in-unreal-engine

> 18 https://dev.epicgames.com/documentation/unreal-engine/running-gauntlet-tests-in-unreal-engine https://dev.epicgames.com/documentation/unreal-engine/running-gauntlet-tests-in-unreal-engine

> 19 https://dev.epicgames.com/documentation/unreal-engine/game-features-and-modular-gameplayin-unreal-engine

https://dev.epicgames.com/documentation/unreal-engine/game-features-and-modular-gameplay-in-unreal-engine

---

## Guida operativa — reperire asset gratuiti

### Sommario esecutivo
Per una POC “zero-cost” in stile fantasy cartoonesco (alla _World of Warcraft_ ) abbiamo esplorato fonti ufficiali e community per asset 3D gratuiti. Le risorse prioritarie includono l’ **Unreal Engine Marketplace (Fab)** , lo **Unity Asset Store** (contenuti free), **Kenney.nl** (CC0), **OpenGameArt** , **Itch.io** e **Sketchfab** (modelli CC0/CC-BY), nonché repository GitHub con asset free. Per ciascuna fonte forniamo esempi di pacchetti rilevanti (con link diretti), licenze e permessi d’uso (ad es. utilizzo commerciale/POC), formati dei file, polycount/LOD, texture, rigging/animazioni e difficoltà di import in UE5.8. Flagghiamo eventuali restrizioni (es. solo uso non commerciale o richieste di attribuzione). È inclusa una tabella comparativa di 12+ pacchetti candidati (sorgente, nome, licenza, formati, animazioni, texture, difficoltà d’import, grado di “fit” al look WoW, note). Citiamo thread Reddit/Discord (es. r/freegameassets, community Unreal), progetti GitHub open con asset permisivi (es. lista _awesome-cc0_ 【63†L223-L231】 【87†L570-L578】) e suggeriamo una lista minima di asset POC (terreno, vegetazione, personaggi, nemici, props, UI, VFX, suoni) con 1–2 pack raccomandati per categoria + link diretti. Infine, riportiamo linee guida passo-passo per importare gli asset in Unreal Engine 5.8.1 (conversione modelli, creazione .uasset, setup materiali, insidie comuni). Ipotizziamo uso “commerciale” del prototipo (licenze _royalty-free_ o CC0), e segnaliamo se è richiesta attribuzione o limitazioni d’uso.

**Parole chiave** : risorse asset free, fantasy cartoonesco, WoW-like, Unreal Engine 5, Kenney, OpenGameArt, Sketchfab CC0, Unity Asset Store, Fab Unreal, Itch.io, importazione asset.

#### Fonti principali di asset gratuiti
Abbiamo catalogato diverse fonti “ufficiali” e comunitarie di asset stilizzati. Ecco le più rilevanti, con esempi di pacchetti free e loro dettagli essenziali:

- **Unreal Marketplace (Epic Fab)** – Offre molti pacchetti free in alta qualità. Ad esempio, **“Fantasy FREE – Low Poly 3D Models Pack”** (IThappy【5†L14-L18】) contiene alberi, case, rocce stilizzati (formati: FBX, GLB, OBJ). La licenza è quella standard Epic (uso royalty-free in UE), consentendo l’uso anche in progetti commerciali【5†L14-L18】. Altro esempio: **“Creative Characters FREE”** (IThappy) con 30 personaggi modulari stile cartone, riggati e con 14 animazioni【7†L18-L23】 (formati FBX/GLB/OBJ). Anche **“Animals FREE”** (IThappy) offre modelli animaletti low-poly【9†】. In generale, i pacchetti Fab hanno licenza Epic (royalty-free per uso UE, spesso con restrizioni di ri-distribuzione) e formati comuni (FBX, GLTF)【5†L14-L18】【7†L18-L23】.

- **Unity Asset Store (free)** – Il catalogo gratuito Unity include vari asset low-poly fantasy. Ad es.: **“Stylized Fantasy Armory – Low Poly 3D Art”** (Daniel Mistage) con armi mediev-fantasy in stile cartoon; licenza Unity EULA (uso commerciale gratuito). **“Low Poly Environment – Nature Free”** (PS Polytope) contiene alberi, erba e rocce medievali【23†L99-L107】. Formati: pacchetti Unity con Prefab e texture (da convertire in FBX/PNG per UE). _Attenzione_ : gli asset Unity richiedono estrazione/Esportazione (Asset Hunter, Unity Editor) prima di importarli in UE. Sempre licenza Unity Standard EULA【27†L98-L104】, uso consentito anche a scopo commerciale. L’importazione è mediamente laboriosa (servono conversione da Unity a FBX, reapplicazione materiali).

   - **Kenney.nl (CC0)** – Kenney offre migliaia di asset CC0 (pubblico dominio, uso libero senza attribuzione【31†L23-L26】). Pacchetti notevoli in stile cartone: **Cube Pets** (animali animati cubici, CC0, 24 file animati)【31†L23-L26】, **Fantasy Town Kit** (case medievali modulari, 160 file, CC0)【42†L22-L28】, **Mini Dungeon** (ostili, armi, personaggi con animazioni, CC0)【44†L21L28】, **Mini Forest** (ambientazione boschiva con arciere animato, CC0)【46†L21-L28】, **Modular Dungeon Kit** (tileset dungeon, animazioni, CC0)【48†L22-L28】, **Pirate Kit** (barche, isole, pirati animati, CC0)【50†L21-L25】 e **Graveyard Kit** (ambientazione cimitero/horror, CC0)【52†L22L26】. Tutti sono CC0 (uso commerciale e modifiche permessi senza attribuzione)【31†L23-L26】 【42†L22-L28】. Forniscono formati FLAT (FBX/GLTF) e spesso file .blend, con texture e rigging inclusi. Importarli in UE5 è facile (FBX + texture), difficoltà **bassa** . Stile: tutti i modelli Kenney sono stilizzati e “cartooneschi” nei colori (fit 3–5 su 5 per WoW-like).

   - **OpenGameArt (OGA)** – Biblioteca collaborativa di asset CC0/CC-BY. Esempio: **“Medieval House Pack”** 【59†L154-L161】 di Daniel Andersson (case medievali low-poly, CC0) con formati .blend (possono esportarsi in FBX). Permette uso commerciale【59†L152-L160】. Altri asset OGA: mobili, rocce, oggetti vari stilizzati. Licenze: spesso CC0 o CC-BY (controllare ogni asset). Usati per prototipi e giochi (alcuni incluso in collezioni di fantasy su OGA).

   - **Itch.io** – Market indie spesso include asset free. Seguire tag “3D, Fantasy, Stylized, Free”. Esistono collezioni CC0 e CC-BY (verificare licenza in ogni progetto). Esempi tipici: pacchetti modulari low-poly (case, armi, UI) su progetti personali. Spesso pacchetti FBX/GLTF, licenza scelta da autore (alcuni CC0, altri CC-BY con attribuzione necessaria).

   - **Sketchfab (downloadable free)** – Motore di ricerca 3D con filtri CC0 e downloadabili. Numerosi modelli cartooneschi (personaggi, alberi, props) sotto CC0 o CC-BY. Ad es. “Stylized Cartoon Girl” riggata【61†L0-L3】, “Cartoon Boy”【61†L3-L6】, icone Fantasy【61†L9-L12】. File: solitamente FBX, OBJ, GLTF con texture. Verificare licenza (colonna “license”). Sketchfab stesso non è specializzato in asset pack, ma si trovano set CC0 utili (e.g. pacchetti di rocce, alberi, mobili, armi).

   - **Altri CC0/Free pack** – **Kenney Itch.io** (tutto il catalogo Kenney su itch è CC0【63†L237-L239】), **BlenderKit** (plugin/portal con asset 3D, alcuni CC0), **PolyPizza** (collezioni CC0 curated). Esistono anche collezioni generiche di CC0 (p.es. [awesome-cc0]【63†L223-L231】 evidenzia risorse come Kenney, Quaternius, Smithsonian e CC0 su Sketchfab).

- 【92†embed_image】 _Figura – Esempio illustrativo di ambiente “cartoonesco” fantasy (CC0)_

#### Tabella comparativa dei pacchetti (esempi)
|Sorgente|Pacchetto|Link<br>scaricamento|Licenza|Formati|Animazioni|Texture|Difficoltà<br>import|
|---|---|---|---|---|---|---|---|
|Unreal<br>Marketplace|Fantasy<br>FREE<br>(IThappy)<br>【5†L14-<br>L18】|_Fab: Fantasy_<br>_Free_|Epic Free<br>content<br>(royalty-<br>free)|FBX, GLB,<br>OBJ|No|medio<br>(1024–<br>2048 px)|Media<br>(Unreal<br>asset)|

|Sorgente|Pacchetto|Link<br>scaricamento|Licenza|Formati|Animazioni|Texture|Difficoltà<br>import|
|---|---|---|---|---|---|---|---|
|Unreal<br>Marketplace|Creative<br>Characters<br>FREE<br>【7†L18-<br>L23】|_Fab: Creative_<br>_Char._|Epic Free<br>content|FBX, GLB,<br>OBJ|Sì (14 clip)|?<br>(simpatici<br>outfit)|Media|
|Unreal<br>Marketplace|Animals<br>FREE<br>(IThappy)|_Fab: Animals_<br>_Free_|Epic Free<br>content|FBX, GLB,<br>OBJ|Prob. no|modeste|Bassa|
|Unreal<br>Marketplace|Stylized Boy<br>Character<br>(ShooTeR)|_Fab: Stylized_<br>_Boy_|Epic Free<br>content|FBX|No|texture<br>atlases ?|Bassa|
|Unreal<br>Marketplace|LapaModels<br>– Stylized<br>Props<br>【16†L5-<br>L8】|_Fab: Stylized_<br>_Assets_|Gratuity:<br>com&perso<br>(no RED)|GLB|No|varie<br>(hand-<br>painted)|Media<br>(compatibile<br>UE)|
|Unity Asset<br>Store|Stylized<br>Fantasy<br>Armory<br>(Unity)<br>【23†L99-<br>L107】|_Asset Store_|Unity EULA<br>(free<br>comm.)|UnityPackage<br>(.prefab/FBX)|No|1024–<br>2048<br>(leak?)|Medio<br>(Unity→UE)|
|Unity Asset<br>Store|Low Poly<br>Nature Free<br>(Polytope)<br>【23†L99-<br>L107】|_Asset Store_|Unity EULA|UnityPackage|No|1024–<br>2048|Medio|
|Unity Asset<br>Store|Low-Poly<br>Simple<br>Nature<br>(JustCreate)|_Asset Store_|Unity EULA|UnityPackage|No|medio|Bassa (FBX)|
|Kenney CC0|Cube Pets<br>【31†L23-<br>L26】|_kenney.nl_|CC0 (Public<br>Domain)|FBX, OBJ,<br>BLEND|Sì (24<br>anim)|512-1024<br>px|Bassa|
|Kenney CC0|Fantasy<br>Town Kit<br>【42†L22-<br>L28】|_kenney.nl_|CC0|FBX, BLEND|No|1024 px<br>(varianti<br>color)|Bassa|

|Sorgente|Pacchetto|Link<br>scaricamento|Licenza|Formati|Animazioni|Texture|Difficoltà<br>import|
|---|---|---|---|---|---|---|---|
||Mini|||||||
|Kenney CC0|Dungeon<br>【44†L21-<br>L28】|_kenney.nl_|CC0|FBX, BLEND|Sì (anim)|1024 px|Bassa|
|Kenney CC0|Modular<br>Dungeon<br>Kit<br>【48†L22-<br>L28】|_kenney.nl_|CC0|FBX, BLEND|Sì (anim)|512 px<br>(text)|Bassa|
|Kenney CC0|Pirate Kit<br>【50†L21-<br>L25】|_kenney.nl_|CC0|FBX, BLEND|Sì (anim)|1024 px|Bassa|
||Graveyard|||||||
|Kenney CC0|Kit<br>【52†L22-<br>L26】|_kenney.nl_|CC0|FBX, BLEND|Sì (anim)|1024 px|Bassa|
|Kenney CC0|Blocky<br>Characters<br>【54†L21-<br>L25】|_kenney.nl_|CC0|FBX, BLEND|Sì (anim)|512 px<br>(very<br>lowpoly)|Bassa|

_N.B._ Le voci sopra sono esempi: molte fonti (Itch, Sketchfab) offrono ulteriori pacchetti simili. Le licenze indicate (Epic, Unity EULA, CC0) permettono uso POC/commerciale; l’unica restrizione possibile è attribuzione in caso di licenze Creative Commons (CC-BY). Tutti i pacchetti Kenney sono **CC0** 【31†L23L26】, quindi utilizzabili senza obbligo di credit. Gli asset UE gratuiti seguono la licenza Epic (uso gratuito nei progetti UE)【5†L14-L18】.

#### Risorse comunitarie e progetti open-source
Oltre agli store ufficiali, esistono forum e community ricchi di link a asset free. Su **Reddit** (es. r/ freegameassets, r/gamedev, ecc.) gli utenti condividono regolarmente bundle low-poly cartoon e consigli di fonti【83†L166-L173】. Su **Discord** (es. Unreal Slackers, Kenney Discord) si discute di asset gratuiti e si segnalano giveaway. Alcuni GitHub offrono liste curate: p.es. _gamedev-free-resources_ 【87†L570-L578】 elenca Unity Asset Store, Unreal, Kenney, OpenGameArt come piattaforme chiave; il repository _awesome-cc0_ 【63†L223-L231】 segnala Kenney, Sketchfab CC0 e altre fonti CC0. Comunità Italiane (forum Blender/UE italiani) occasionalmente raccolgono link a asset gratuiti.

**Thread e Discord utili:** r/freegameassets (share di asset CC0 e free), _Awesome CC0_ (GitHub)【63†L223L231】, _GameDev Free Assets_ (GitHub)【87†L570-L578】, canali Discord come “Kenney Community” o “Unreal Engine Italia”.

#### Asset minimali consigliati per POC
Per un prototipo base “zero-cost” consigliamo almeno questi asset (1–2 pack per categoria):

- **Terreno e ambientazione** :

- **Fantasy Town Kit (Kenney)** 【42†L22-L28】 – moduli di case medievali, muri, recinzioni. Licenza CC0. Formati FBX/BLEND, texture 1024px. Import facile (low poly). _Link:_ <u>Scarica ZIP【42†L22-</u> L28】.

- **Low Poly Nature (Unity)** 【23†L99-L107】 – alberi, rocce, erba (cartoon natural). UnityPackage (estrarre FBX). Licenza Unity free.

##### **Foliazione** :

- **Mini Forest (Kenney)** 【46†L21-L28】 – bosco con arciere e tenda. CC0. Animazioni basiche (20 file). _Link:_ <u>Scarica ZIP【46†L21-L28】.</u>

- **Low-Poly Simple Nature (Unity)** – singolo albero/foglie. Molto leggero (1.9MB).

- **Personaggi** :

- **Creative Characters (IThappy)** 【7†L18-L23】 – personaggi modulari low-poly (umani). Rig e 14 animazioni incluse. _Link:_ (scaricabile dal Fab di Unreal).

- **Blocky Characters (Kenney)** 【54†L21-L25】 – 20 personaggi cubici animati. CC0.

- **Nemici/Creature** :

- **Cube Pets (Kenney)** 【31†L23-L26】 – animali (cane, gatto, tigre...). CC0, 24 animazioni. _Link:_ <u>Scarica ZIP【31†L23-L26】.</u>

- **Mini Dungeon (Kenney)** 【44†L21-L28】 – goblin, eroi, spade, scudi. CC0, animazioni di personaggi.

##### **Armi/Props** :

- **Stylized Fantasy Armory (Unity)** – asce, spade, scudi cartoon. Unity EULA, esportabili in FBX.

- **Fantasy Weapon Pack (Kenney)** – (es. Medieval Weapons di Kenney, non citato sopra) CC0, armi low-poly. Ad esempio Kenney _Low Poly Medieval Weapon Pack_ .

- **Elementi scenici (props)** :

- **Fantasy House (Daniel Andersson)** 【59†L152-L160】 – case medievali CC0 (blend). _Link:_ <u>House1 ZIP【59†L167-L173】.</u>

- **Graveyard Kit (Kenney)** 【52†L22-L26】 – bare, tombe, alberi spettrali. CC0, animazioni zombie.

- **Icone/UI** :

- **Game Icons (Game-icons.net)** – libreria free di icone in stile flat. CC BY o CC0.

- **Kenney UI Pack** (Kenney offre anche 2D GUI gratuite).

- **VFX & Suoni** :

- **Unreal Engine Starter VFX** (fuoco, fulmini) – many included in Engine Content o Marketplace gratuito.

- **Freesound.org, Sonniss** – effetti sonori CC0/CC-BY. Utili VFX audio fantasy.

- **Free Music Archive** – brani liberi (es. per menu) sotto CC0 o CC-BY.

Questi asset coprono vegetazione, strutture, personaggi, nemici, props, UI e suoni. Con i link forniti è possibile scaricarli immediatamente (vedere tabella e riferimenti).

#### Importazione in Unreal Engine 5.8.1
**1. Preparare i modelli:** (se provengono da Unity o Blender) – Esportare/modificare i file in formato FBX. In Unity Asset Store: importare il pacchetto in Unity, selezionare i mesh, poi usare l’export _FBX Exporter_ . Controllare che le mesh siano “scaled for UE” (1 unit=1cm). Verificare che i pivot siano corretti (es. suono inferiore).

**2. Importare in UE5.8:** Aprire l’Editor UE5.8.1, progetto scelto. Nella _Content Browser_ , clic destro **Import to /Game/** . Selezionare i file FBX esportati. Nella finestra di importazione: spuntare “Import Rigid Meshes”, “Combine Meshes” se necessarie, generare collisioni semplici (per prototipo). Attivare “Import Textures” se incluse. Gli oggetti in formati .uasset (convertiti) appariranno nel Content Browser.

**3. Creare materiali:** Selezionare ogni mesh importata, aprire Material Editor. Creare materiali PBR semplici assegnando le texture diffuse (Albedo) e normal map (se disponibili). Ad esempio, per stile cartoon usare shading PBR ma con colori saturi. Se il modello ha colori nel diffuse (es. handpainted), spesso basta collegare la texture al BaseColor. Consiglio di mantenere texture 1024-2048px per qualità equilibrata.

**4. LOD e Performance:** Verificare polycount: molti asset lowpoly (<5k poly) vanno bene. Alcuni pacchetti includono LOD automatici. In UE, si possono generare LOD nel menu di dettaglio Static Mesh (Generate Mesh LODs). Per prototipo non sempre necessario, ma utile per foreste/oggetti numerosi.

##### 5. Tip comuni:
- _Scala:_ controllare che la scala del modello importato (es. un personaggio 180 cm) sia corretta (UE: 1 unit = 1 cm). Eventualmente scalare in Blender/FBX esportato.

- _Gerarchia ossea:_ per personaggi animati, selezionare “Import Animations” e assegnare skeleton appropriato (es. già incluso). Verificare che la root bone sia in posizione corretta (talvolta UE5 ricalcola l’anim di camminata se root non è posizionato su terra).

- _Collisioni:_ UE crea collisioni primitive (box/sphere) se non fornite. Per forme concave, aggiungere collision custom in Blender o usare “Add Box/Sphere Simplified Collision”.

- _.uasset:_ Una volta importati, UE crea file .uasset (binari UE). Assicurarsi di mantenere la gerarchia Content chiara (es. /Game/Assets/Props, /Game/Characters, ecc.).

##### 6. Importazione specifica per Blueprint/UI/VFX:
- _Personaggi:_ dopo import FBX e anim, si può creare un Blueprint Character che usa il mesh e l’Anim Blueprint (mixamo/altro).

- _VFX:_ se ci sono particelle (es. Kenney non fornisce VFX), si può usare i Niagara standard per fuoco, magie.

- _UI:_ Le icone importarle come _Brush assets_ nelle UI o in Widget Blueprint.

**7. Verifica finale:** Posizionare gli asset in una scena di test. Controllare luci (il look cartoon beneficia di illuminazione soffusa e meno realistica). Regolare _Post Process_ (filtro cel shading, saturazione colore) se occorre. Testare collisioni e animazioni.

<!-- Start of picture text -->
Identificazione fonti<br>Raccolta asset pack<br>Ricerca Analisi licenze e permessi<br>Comparazione pack (tabella)<br>Preparazione report<br>-07-25 -07-25 -07-26 -07-26 -07-27 -07-27 -07-28 -07-28 -07-29 -07-29 -07-30 -07-30 -07-31 -07-31 -08-01<br><!-- End of picture text -->

##### `flowchart LR`

```
    A[Scarica asset 3D (FBX)] --> B{Origine file?}
```

```
    B -->|Blend/OBJ| C[Esporta in FBX (Blender)]
```

```
    B -->|UnityAsset| D[Usa Unity FBX Exporter]
```

```
    C --> E[Importa in UE5.8 (via Content Browser)]
```

```
    D --> E
    E --> F[Configura materiali (PBR)]
    F --> G[Regola collisioni/LOD]
    G --> H[Test in scena (luce e scala)]
```

**Fonti** : Documentazione fab.unreal.com per asset gratuiti【5†L14-L18】【7†L18-L23】; licenze CC0/CCBY (cfr. Kenney【31†L23-L26】, OpenGameArt【59†L152-L160】); lista “gamedev free resources”【87†L570-L578】 e _awesome-cc0_ 【63†L223-L231】; thread community Reddit【83†L166L173】. Tutti i dati e i link diretti sono tratti dalle pagine ufficiali degli asset citati (vedi riferimenti).
