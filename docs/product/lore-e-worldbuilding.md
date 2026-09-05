# Lore e worldbuilding — la sorgente interna

> `PROPOSED` · **Non normativo, e non ancora ratificato** · **Aggiornato**: 2026-08-30 ·
> **Decisione che assegna questo owner**: [D-246](../decisions/RT_PDR_00_Decision_Log.md)
>
> **Ambito**: Harmonic Coupling, The Refactor, ARC, le quattro fazioni — e il livello di conoscenza con
> **spoiler narrativi** che la Wiki non pubblica.

## Che cos'è questa pagina, e che cosa non è

È la **sorgente d'autore del worldbuilding**, portata dentro il repository il 2026-08-30 dalla radice, dove
stava come `RefactorTactics_Wiki_Lore.md`. Da quel giorno ha un owner e un percorso; prima non ne aveva
nessuno dei due, e la Wiki lo diceva apertamente: *«questa pagina non ha ancora un owner doc nel
repository»*.

🟡 **Non è canone.** `Harmonic Coupling`, `Refactor` e `ARC` **non compaiono** nel
[Decision Log](../decisions/RT_PDR_00_Decision_Log.md) né in nessun owner doc di `gameplay/` o
`technical/` — misurato il 2026-08-30 su `fff33020`: `git grep -l "Harmonic Coupling"` rispondeva **un
solo file**, questo stesso, allora in radice. Il mondo qui descritto è una **proposta consolidata**, non una decisione presa. Fino
alla ratifica non si cita come autorità e non decide niente: la gerarchia delle fonti è in
[`../README.md`](../README.md), e questa pagina non ci compare.

⚠️ **E i `FactionId` qui sotto non sono simboli**: `git grep -o 'Faction\.[A-Za-z]*'` su `Source/` e
`Scenarios/` risponde **zero** — misurato il 2026-08-30. `Faction.Conflux`, `Faction.Constrine`,
`Faction.Sentinel` e `Faction.Resonance` esistono in questa pagina e sulla Wiki, e in nessun file che il
gioco esegua. Sono nomi proposti, non identificatori in uso: chi ne scrivesse uno in un `.json` di scenario
non troverebbe niente dall'altra parte.

🔒 **È il livello interno di una coppia.** La parte pubblica è **già pubblicata** sulla Wiki, riscritta
player-first: [`Lore`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/Lore),
[`Harmonic e ARC`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/harmonic-e-arc) e le quattro
pagine di [`Fazioni`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/Fazioni). Quella pagina
dichiara che *«una parte del materiale non è pubblicata qui … quel livello resta nella documentazione
interna»*. **Questa è quella documentazione interna**, e fino a oggi la frase non aveva un referente: la
sezione *Archivio classificato* — l'anomalia temporale, gli Attractor precedenti al Refactor, `The
Revision` — esisteva **solo** nel file di radice.

⚠️ **La Wiki non è una copia di questa pagina, ed è voluto.** La Wiki riscrive, taglia e collega alle
meccaniche; qui il testo resta quello dell'autore. Se le due divergono su un fatto del mondo, **vince
questa**, che è la sorgente. Se divergono su *come si racconta*, vince la Wiki, che è il prodotto
player-facing ([D-076](../decisions/RT_PDR_00_Decision_Log.md): il clone pubblicato è la sua unica fonte).

<!-- rename-exempt: la riga dichiara la mappatura -->
📌 **Due cose sono state cambiate rispetto al file di radice, e nient'altro.** I nomi del roster v0.1 sono
stati portati a quelli canonici — `Flux` → **Gadget**, `Riva` → **Phase**, `Bastion` → **Riktor**,
`Vektor` → **Wraith**, per [D-130](../decisions/RT_PDR_00_Decision_Log.md) con la mappatura di
[D-037](../decisions/RT_PDR_00_Decision_Log.md) — e i quattro `![…](images/factions/overview/…)` sono stati
tolti: puntavano a file che **non esistono né qui né sul clone della Wiki**, che tiene invece
`images/factions/<fazione>.png`. Un'immagine incorporata e assente non la segnala più nessuno dal 2026-08-21
([D-182](../decisions/RT_PDR_00_Decision_Log.md)), quindi si toglie invece di lasciarla marcire.

---

## In breve

Per secoli l'umanità ha trattato materia, energia, acqua, calore, strutture e informazione come sistemi separati, collegati da leggi note e sufficientemente prevedibili.

Poi è stato scoperto l'**Harmonic Coupling**.

## Harmonic Coupling

L'Harmonic Coupling è un fenomeno fisico per cui sistemi apparentemente indipendenti possono entrare in accoppiamento quando raggiungono condizioni precise.

In questi stati, interazioni normalmente deboli possono diventare molto più forti o qualitativamente diverse.

Non è magia.

È:

- misurabile;
- studiabile;
- riproducibile entro condizioni note;
- utilizzabile tecnologicamente;
- ancora non completamente compreso.

Le prime applicazioni trasformarono energia, materiali, infrastrutture e controllo ambientale. Le città iniziarono a essere progettate non più come insiemi di componenti isolati, ma come sistemi dinamici in grado di reagire e riconfigurarsi.

## The Refactor

Il cambiamento decisivo arrivò durante uno dei primi grandi esperimenti sull'Harmonic Coupling.

L'esperimento avrebbe dovuto iniziare a un preciso istante di attivazione.

Non ci arrivò mai davvero.

Undici secondi prima del trigger previsto, i sensori registrarono un accoppiamento non programmato.

Otto secondi prima, sottosistemi isolati iniziarono a scambiarsi energia.

Tre secondi prima, la topologia operativa dell'impianto cessò di corrispondere ai suoi schemi di progettazione.

Poi il sistema cambiò.

Non esplose.

Non collassò.

**Si riorganizzò.**

Condotti, reti energetiche, sistemi idraulici, barriere e controllo ambientale iniziarono a comportarsi come parti di una nuova configurazione.

La cosa più inquietante non fu che la struttura fosse cambiata.

Fu che la nuova configurazione sembrava avere una logica.

Da quel momento il termine **Refactor** indicò una riconfigurazione stabile o temporanea delle proprietà o delle relazioni funzionali di un sistema prodotta dall'Harmonic Coupling.

```text
Harmonic Coupling
        ↓
Harmonic / Resonant State
        ↓
Refactor
        ↓
nuova configurazione del sistema
```

L'evento storico divenne noto come **The Refactor**.

Il mondo iniziò a distinguere due epoche:

**prima del Refactor** e **dopo il Refactor**.

---

## Identità dei personaggi e asset vendor

La lore usa soltanto identità **RefactorTactics**. I nomi degli asset o degli slot vendor non sono identità
narrative e non diventano nomi player-facing, nomi di abilità, `CharacterId` o lore.
[D-037](../decisions/RT_PDR_00_Decision_Log.md) lo stabilisce — *«lo slot non è l'identità»* — e
[D-321](../decisions/RT_PDR_00_Decision_Log.md) lo riporta a invariante dopo averlo trovato violato.

Le otto identità retail del roster sono decise:

| Fazione | Identità RefactorTactics | Release |
|---|---|---|
| Conflux | **Nexis** · **Slake** | v0.1 ⏳ |
| Constrine | **Kern** · **Scryer** | v0.1 ⏳ |
| Sentinel Directorate | **Ward** · **Vigil** | v0.2 |
| Resonance | **Rime** · **Tethra** | v0.2 |

⏳ **Identità decisa, runtime non ancora migrato.** Il codice porta ancora gli ID legacy `Hero.Gadget`,
`Hero.Phase`, `Hero.Riktor` e `Hero.Wraith` — che sono i nomi degli **slot Paragon**, non nomi ispirati ad
essi. La migrazione è differita **post-v0.1** da [D-321](../decisions/RT_PDR_00_Decision_Log.md) e ha come
owner [#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297).

⚠️ **Fino ad allora quei quattro nomi sono identità temporanee, non eccezioni permanenti**, e la lore evita di
cristallizzarli come canonici: dove serve nominare un personaggio della v0.1, si scrive l'identità retail.

Gli slot asset vendor continuano a essere citati **solo** nei documenti tecnici di mapping e provenienza.
Tabella owner: [`../characters/paragon.md`](../characters/paragon.md).

Tracking operativo: [#2291](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2291) — purge e gate ·
[#2297](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2297) — migrazione tecnica.

---

# Le quattro fazioni

Le fazioni di RefactorTactics non sono classi e non obbligano la composizione di squadra.

Descrivono:

- identità narrativa;
- filosofia;
- dottrina tattica;
- cultura;
- linguaggio visivo;
- rapporto con Harmonic Coupling e Refactor.

Una squadra può contenere liberamente membri di fazioni diverse e non esistono bonus automatici per una composizione mono-fazione.

## Conflux

**FactionId:** `Faction.Conflux`  
**Membri iniziali:** **Nexis** · **Slake** — **v0.1** ⏳ · schede: [`v0.1/gadget.md`](../characters/v0.1/gadget.md) · [`v0.1/phase.md`](../characters/v0.1/phase.md)

> **Tutto è collegato.**

Conflux interpreta il mondo come una rete di relazioni.

Non cerca di fissare il sistema in una configurazione perfetta. Preferisce comprenderne i collegamenti, modificarli e sfruttare ciò che emerge.

La sua dottrina può essere riassunta così:

```text
trasforma
    ↓
collega
    ↓
propaga
    ↓
sfrutta la conseguenza
```

### Rapporto con Harmonic

Conflux studia soprattutto:

- reti;
- propagazioni;
- connessioni;
- adattamento;
- comportamenti emergenti.

**Conflux studia la rete.**

### Rapporto con il Refactor

Per Conflux, un Refactor è una proprietà naturale dei sistemi sufficientemente interconnessi.

Il problema non è impedire ogni cambiamento.

Il problema è capire come indirizzarlo.

### ARC / dati riservati

Alcuni archivi Conflux contengono log di connessioni e percorsi emergenti che non risultavano presenti nell'architettura originaria dei sistemi osservati.

---

## Constrine

**FactionId:** `Faction.Constrine`  
**Membri iniziali:** **Kern** · **Scryer** — **v0.1** ⏳ · schede: [`v0.1/riktor.md`](../characters/v0.1/riktor.md) · [`v0.1/wraith.md`](../characters/v0.1/wraith.md)

> **Ciò che è delimitato può essere controllato.**

Constrine nasce dalla convinzione che un sistema diventi comprensibile quando i suoi limiti sono definiti.

Non vuole semplicemente difendere il territorio.

Vuole ridurre lo spazio delle possibilità.

```text
delimita
    ↓
canalizza
    ↓
prevedi
    ↓
punisci
```

### Rapporto con Harmonic

Constrine studia:

- vincoli;
- geometria;
- containment;
- stabilità;
- prevedibilità.

### Rapporto con il Refactor

Per Constrine, un Refactor può essere utile soltanto quando i suoi confini sono conosciuti, controllabili e ripetibili.

### ARC / dati riservati

Constrine cataloga configurazioni geometriche che sembrano aumentare la probabilità di anomalie e Refactor non standard.

In alcuni archivi queste configurazioni sono indicate come **Forbidden Configurations**.

---

## Sentinel Directorate

**FactionId:** `Faction.Sentinel`  
**Membri iniziali:** **Ward** · **Vigil** — **v0.2** · schede: [`v0.2/steel.md`](../characters/v0.2/steel.md) · [`v0.2/murdock.md`](../characters/v0.2/murdock.md)

> **Una posizione controllata diventa una certezza.**

Il Sentinel Directorate è orientato a protezione, sicurezza, sorveglianza e risposta preparata.

La sua domanda fondamentale non è:

> Come vinciamo ogni scontro?

È:

> **Che cosa non possiamo permetterci di perdere?**

Centrale, ponte, corridoio civile, sito armonico, informazione, alleato o obiettivo diventano il centro della battaglia.

### Rapporto con Harmonic

Sentinel usa e controlla tecnologia armonica per:

- stabilizzare zone;
- proteggere infrastrutture;
- contenere hazard;
- mantenere condizioni operative sicure;
- rispondere a incidenti.

### Rapporto con il Refactor

Per Sentinel, un Refactor fuori controllo è prima di tutto un **evento di sicurezza**.

Deve essere contenuto, stabilizzato o evacuato.

### ARC / dati riservati

Il Directorate mantiene dossier su:

- Active Refactor Zones;
- fallimenti di contenimento;
- incidenti armonici persistenti;
- siti che continuano a modificarsi dopo il termine apparente dell'evento iniziale.

---

## Resonance

**FactionId:** `Faction.Resonance`  
**Membri iniziali:** **Rime** · **Tethra** — **v0.2** · schede: [`v0.2/aurora.md`](../characters/v0.2/aurora.md) · [`v0.2/kwang.md`](../characters/v0.2/kwang.md)

> **Potere e posizione hanno valore quando entrano in risonanza.**

Resonance studia non soltanto quali sistemi sono collegati, ma **quando** una configurazione diventa significativa.

La sua dottrina segue:

```text
prepara
    ↓
allinea
    ↓
sincronizza
    ↓
esegui
```

### Rapporto con Harmonic

Resonance studia:

- Harmonic States;
- timing;
- allineamento;
- attractor;
- configurazioni ricorrenti.

**Conflux studia la rete. Resonance studia lo stato della rete.**

### Rapporto con il Refactor

Per Resonance, il Refactor è il risultato visibile di una configurazione armonica più profonda.

### Harmonic Attractors

Alcuni stati finali sembrano ricorrere in sistemi molto diversi.

Queste configurazioni vengono indicate come **Harmonic Attractors**.

Il punto controverso è che alcuni attractor non sono semplicemente simili.

Sembrano identici.

---

# Il mondo dopo il Refactor

La tecnologia armonica entra progressivamente in:

- energia;
- trasporti;
- materiali;
- controllo ambientale;
- infrastrutture;
- sicurezza;
- sistemi militari.

Il valore strategico non dipende più soltanto dalla quantità di territorio controllato.

Dipende da **quali relazioni** possono essere controllate.

Una porta può valere più di un edificio.

Un ponte può diventare accesso, trappola o barriera.

Una rete idrica può trasformarsi in vettore energetico.

Una copertura può cambiare funzione.

Un corridoio sicuro può cessare di esserlo dopo una riconfigurazione.

Per questo molti conflitti dell'era moderna vengono affidati a piccoli team altamente specializzati.

Le loro missioni non richiedono necessariamente di conquistare una città.

Possono richiedere di:

- controllare un nodo;
- stabilizzare un sito;
- recuperare dati;
- sabotare un esperimento;
- proteggere un'infrastruttura;
- modificare una configurazione;
- impedire un Refactor.

Le fazioni possono cooperare in coalizioni operative anche quando le loro filosofie sono incompatibili.

**Fazione** e **squadra di missione** sono due concetti distinti.

---

# Harmonic, Refactor e ARC

La teoria pubblica distingue:

```text
HARMONIC
fenomeni compresi e modellati

REFACTOR
riconfigurazioni prodotte dal Coupling

ARC
fenomeni o configurazioni che non corrispondono
completamente ai modelli accettati
```

`Arcane` è un termine colloquiale utilizzato per gli eventi **ARC-class**.

Arcano non è quindi un elemento fisico equivalente a Fuoco, Acqua o Elettricità.

Un evento ARC può essere misterioso per i personaggi del mondo, ma le sue regole non devono essere casuali o incoerenti.

---

<details>
<summary><strong>Archivio classificato — spoiler narrativi</strong></summary>

## L'anomalia temporale

La versione pubblica afferma che The Refactor fu causato dall'esperimento.

I log classificati raccontano qualcosa di diverso.

Il primo accoppiamento anomalo venne registrato **prima** dell'attivazione completa dell'esperimento.

```text
T - 11.7
anomalia armonica

T - 8.2
prima riconfigurazione locale

T - 3.1
topologia fuori modello

T 0.0
trigger sperimentale previsto

T + 0.4
Full Refactor
```

## Il problema degli Attractors

Analisi successive mostrarono pattern associati agli Harmonic Attractors in:

- dataset geologici;
- vecchi log energetici;
- osservazioni atmosferiche;
- sistemi precedenti alla scoperta ufficiale dell'Harmonic Coupling.

Alcuni dati precedono The Refactor di decenni.

La domanda non è più soltanto:

> Perché il nostro esperimento ha creato il Refactor?

Diventa:

> **Perché esistono firme compatibili con il Refactor prima che l'umanità sapesse come produrlo?**

## The Revision

Esistono riferimenti frammentari a una organizzazione indicata come:

**The Revision**

Non è considerata una quinta fazione giocabile.

La sua natura non è pubblicamente nota.

Le informazioni disponibili suggeriscono che:

- raccoglie dati ARC;
- opera attraverso più fazioni;
- ha accesso a informazioni precedenti all'annuncio pubblico di The Refactor;
- interviene su siti, archivi e programmi di ricerca;
- non sembra voler semplicemente distruggere o provocare Refactor.

Una frase compare in più documenti attribuiti alla Revision:

> **Preserve continuity through controlled revision.**

Il dato più pericoloso contenuto negli archivi non è una teoria sugli alieni, sugli dèi o su una civiltà perduta.

È molto più semplice.

**The Refactor potrebbe non essere stato il primo Refactor.**

Potrebbe essere stato soltanto il primo riconosciuto dal mondo moderno.

La causa ultima resta **OPEN**.

</details>

---

# Relazioni fra le fazioni

| Relazione | Tensione principale |
| --- | --- |
| Conflux ↔ Constrine | adattamento contro controllo |
| Conflux ↔ Sentinel | sperimentazione contro sicurezza |
| Conflux ↔ Resonance | rete dinamica contro configurazione ottimale |
| Constrine ↔ Sentinel | prevedibilità e sicurezza, con rischio di immobilismo |
| Constrine ↔ Resonance | modello controllabile contro stati limite |
| Sentinel ↔ Resonance | contenimento contro esplorazione di configurazioni rischiose |

Nessuna relazione è un'alleanza permanente.

Le quattro fazioni possono cooperare quando l'obiettivo lo richiede.

---

# Collegamenti

**Dentro il repository** — le schede dei personaggi nominati qui:
[`characters/v0.1/`](../characters/v0.1/gadget.md) (Gadget, Phase, Riktor, Wraith) ·
[`characters/v0.2/`](../characters/v0.2/steel.md) (Ward · Vigil · Rime · Tethra) ·
[`characters/paragon.md`](../characters/paragon.md) per la mappatura visuale
([D-037](../decisions/RT_PDR_00_Decision_Log.md)).

**Sulla Wiki** — la riscrittura player-first di questa stessa materia:
[`Lore`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/Lore) ·
[`Fazioni`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/Fazioni) ·
[`Conflux`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/conflux) ·
[`Constrine`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/constrine) ·
[`Sentinel Directorate`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/sentinel-directorate) ·
[`Resonance`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/resonance) ·
[`Harmonic e ARC`](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/harmonic-e-arc).

⚠️ `Harmonic Coupling`, `Refactor`, `ARC / Arcane` e `Harmonic Attractors` **non hanno una voce di
glossario nel repository**: esistono in questa pagina e sulla Wiki, e da nessun'altra parte. Finché il
worldbuilding non è ratificato è corretto così — ma è anche la ragione per cui non si possono citare come
se fossero termini definiti.

---

## Nota editoriale

Questa pagina deve restare sincronizzata con il [Decision Log](../decisions/RT_PDR_00_Decision_Log.md) e
con le quattro pagine di fazione della Wiki.

✅ **La separazione dei due livelli è avvenuta, e questa pagina ne è la metà interna.** La nota originale
dell'autore chiedeva: *«se la Wiki diventa player-facing senza spoiler, spostare l'Archivio classificato in
documentazione interna o in una pagina spoiler separata»*. La Wiki **è** diventata player-facing — è la
ristrutturazione della [`#422`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/422) — e la
sua pagina `Lore` pubblica il livello aperto e dichiara di lasciare fuori quello chiuso. L'Archivio
classificato qui sopra è il livello chiuso, e questo file è la documentazione interna che quella frase
nominava senza poterla puntare.

⛔ **Quindi la regola operativa, d'ora in poi:** ciò che sta dentro `<details>` **non si porta sulla
Wiki**. Se un giorno servirà pubblicarlo, avrà una pagina dichiarata come spoiler e la decisione sarà
registrata; non ci si arriva copiando un paragrafo.

## Stato, e la domanda aperta che lo governa

Il worldbuilding non è ratificato, e la domanda che lo tiene aperto **esiste già** ed è di livello
prodotto: *«Identità originale (nomi, lore) — necessaria per una pubblicazione»*, in
[`OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) §*Aperte — livello prodotto*, owner
[`piano-canonico-mvp.md`](piano-canonico-mvp.md) §9. Non se ne apre una seconda: questa pagina è il
materiale su cui quella domanda si deciderà.

Ciò che serve per chiuderla non è scritto qui perché non è stato deciso. Ciò che si può dire oggi è
**quanto costa lasciarla aperta**: nessuna feature dipende da questa pagina, nessun test la legge, e il
Feature Registry non la copriva nemmeno quando esisteva — *«il worldbuilding non è una meccanica»*, dice la
Wiki. È l'unica area del progetto in cui restare non decisi non blocca niente.
