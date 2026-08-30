# La posa della sagoma dell'ultimo contatto — spec panel

> `CURRENT` · **Referto di revisione**, non owner. Valuta la proposta *«snapshot al momento del contatto
> (`FPoseSnapshot`): il fantasma mostra come l'unità stava quando l'hai vista l'ultima volta»*, formulata
> il 2026-08-30 dopo la diagnosi del difetto.
>
> **Data**: 2026-08-30 · **Modo**: critique · **Focus**: requirements + architecture
>
> **Cosa è**: il verdetto su una proposta di **presentazione** che tocca il confine con la simulazione.
> `/sc:spec-panel` è task **documentale** ([`CLAUDE.md`](../../../CLAUDE.md) §6): questo referto è stato
> prodotto **prima** di qualunque riga di codice, e non ne autorizza nessuna da solo.
>
> **Cosa non è**: un'autorità. Se una riga qui diverge dall'owner
> ([`conoscenza-parziale-visibile-spec.md`](../../technical/systems/conoscenza-parziale-visibile-spec.md),
> il [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md)), **ha ragione l'owner**.
>
> **Nessun `D-nnn` è assegnato**: il panel non decide, misura.

---

## 1. Il difetto che ha aperto la discussione

La sagoma dell'ultimo contatto si disegna in **posa di riferimento**. Su Riktor la posa di riferimento
distende le catene in fila, e a schermo diventa una linea che attraversa la scena.

Il codice è privo di ambiguità. `UpdateContactGhost` assegna alla sagoma la mesh dell'eroe:

```cpp
ContactGhost->SetSkeletalMesh(HeroSkeletal->GetSkeletalMeshAsset());
```

…e nessuno le assegna una classe di animazione, **deliberatamente**: `FindHeroSkeletal()` salta
`ContactGhost` per identità (`Skeletal != ContactGhost`), quindi `ARTUnit::ApplyUnitAnimClass()` non lo
raggiunge mai. Un `USkeletalMeshComponent` senza `AnimInstance` mostra la posa di riferimento.

⚠️ **Il repository descriveva già questa immagine**, in `editor-sessions.yaml` § U7: *«senza anim BP la
skeletal resta nella posa di riferimento dello scheletro, dove le ossa di catene e tentacoli stanno distese
in fila»*. Era scritto per la skeletal viva; vale identico per la sagoma, e nessuno aveva fatto il
collegamento.

🔴 **E un commento del codice è già falso oggi**: `UpdateContactGhost` dichiara *«La mesh/**posa** arrivano
dalla skeletal VIVA del Blueprint»*. La posa non arriva. Va corretto **qualunque** opzione si scelga — è
una promessa che ha già ingannato una lettura.

### Perché si vede solo su Riktor

| Osservazione | Spiegazione |
|---|---|
| «non subito» | la sagoma nasce solo quando esiste un ricordo, cioè dopo aver **perso di vista** l'unità |
| «solo quello con le catene» | anche gli altri tre fanno la sagoma in posa di riferimento — ma una silhouette umanoide in T-pose *sembra* una sagoma, le catene di Riktor no |
| «le animazioni iniziali sembrano corrette» | l'unità viva **è** animata: quello deformato è il suo fantasma, fermo sull'ultima cella nota |

⛔ **Non è un difetto di locomozione, e non è la scala**: è indipendente da
[#1719](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1719), la cui correzione è verificata
a schermo su Phase e Wraith.

---

## 2. La misura che governa tutto il resto

```
FRTLastKnownContact  →  FRTTeamKnowledge  →  FRTHexSim   (lo snapshot di simulazione)
```

Il docstring di `FRTTeamKnowledge` lo dichiara: *«Formato **versionato**, come il TurnLog e per la stessa
ragione: è **stato di gioco che entra nello snapshot**, quindi un replay lo rilegge, e un campo aggiunto
senza versione renderebbe illeggibili le tracce già scritte»*.

**Il ricordo — chi, dove, quando — è simulazione. La posa non lo è.** Tutto il §3 discende da qui.

---

## 3. I reperti

### 🔴 CRITICO · Fowler — la posa non può vivere in `FRTLastKnownContact`

Il posto "naturale" per uno snapshot è accanto a `Cell` e `TurnNumber`. È la trappola, e va disarmata per
iscritto prima che qualcuno ci cada.

Metterlo lì significa: presentazione dentro lo stato di gioco, **bump di versione del formato**, **corpus
golden da rigenerare**, e ogni snapshot di turno più pesante di centinaia di transform d'osso per unità
ricordata.

⚠️ **E c'è di peggio**: una posa dipende dal tempo di animazione, cioè dal frame rate. È **non
deterministica**. Un dato non deterministico dentro una struttura che un replay rilegge è la definizione del
difetto che il TurnLog esiste per impedire.

**Raccomandazione**: lo snapshot vive **solo lato presentazione**, su `ARTUnit`, e non attraversa mai
`FRTTeamKnowledge`.

### 🔴 CRITICO · Nygard — il momento della cattura è il difetto in agguato

Quando l'unità smette di essere renderizzata UE può **smettere di aggiornarne la posa**
(`VisibilityBasedAnimTickOption`). Catturare *dopo* aver nascosto restituirebbe una posa stantia o la posa
di riferimento — cioè **esattamente il bug che si sta correggendo**, con un passaggio in più e più codice a
sostenerlo.

**Raccomandazione**: cattura **one-shot, prima** di `SetVisibility(false)`, dentro
`RefreshComponentVisibility` — l'unico posto che sa quali componenti esistono e quando lo stato cambia
(lo dichiara il suo stesso commento: *«qui i flag sono lo STATO e questa funzione è l'unico posto che sa
quali componenti esistono»*).

⛔ **E deve restare one-shot**: una cattura continua mentre l'unità è nascosta trasformerebbe il ricordo in
vista. Sarebbe il **terzo leak** in una issue che ne conta due nel titolo.

### ⚠️ MAGGIORE · Cockburn — chi è il titolare del ricordo?

**D-043**: la conoscenza è **di squadra**, non di unità. Uno snapshot su `ARTUnit` ha **una** casella: due
squadre che avessero perso di vista la stessa unità in momenti diversi condividerebbero un ricordo solo.

In v0.1, con un visore locale, non si vede. È un'incoerenza **latente** col modello, non un difetto oggi.

**Raccomandazione**: dichiararla nel codice e nella issue — *«uno snapshot per attore, non per squadra:
corretto finché il visore è uno, sbagliato dal secondo osservatore»*. Una riga che si può cercare vale più
di una scoperta fra sei mesi.

### ⚠️ MAGGIORE · Adzic — il caso che la proposta non copre

Un contatto **non nasce sempre dalla vista**: `CP 13.4` produce contatto da **rumore**. Un'unità sentita e
mai vista non ha nessuna posa da ricordare.

```
Dato    un contatto registrato per rumore, unità mai vista
Quando  si mostra la sagoma
Allora  ...?
```

**Conseguenza sulla scelta, ed è la riga più importante di questo referto**: la posa congelata **non è
un'alternativa allo snapshot — ne è il ripiego obbligatorio**. Chi implementa lo snapshot deve costruire
comunque la posa congelata, per il caso in cui lo snapshot non esista.

### ⚠️ MAGGIORE · Wiegers — nessun criterio falsificabile

*«Mostra com'era quando l'hai vista»* non è verificabile. E qui c'è un'occasione: E21 è l'unica epic della
v0.1 il cui DoD non si chiude in automation, ma **questo** difetto sì.

**Criterio proposto**, headless e senza schermo:

> Dato un `ARTUnit` con skeletal e posa avanzata di N frame, quando la visibilità passa a `false` e la
> sagoma si mostra, allora **almeno un osso della sagoma differisce dalla posa di riferimento** dello
> skeleton.

È l'oracolo che separa «sagoma animata» da «catene in fila», e gira senza un umano davanti allo schermo.

### 🔧 MINORE · Fowler — l'esclusione per identità va riaperta con cautela

Applicare una posa richiede *qualche* `AnimInstance` sulla sagoma. Ma se a fornirla è
`ApplyUnitAnimClass`, la sagoma prende `URTUnitAnimInstance` e **si anima dal vivo**: leak.

**Raccomandazione**: una classe dedicata — che applica una posa e non avanza — assegnata esplicitamente in
`UpdateContactGhost`. L'esclusione in `FindHeroSkeletal` **resta**.

---

## 4. Verdetto

**La proposta è semanticamente giusta e il panel non la respinge.** Un ricordo che mostra la posa ricordata
è ciò che `S4` descrive, e la sagoma è già *«semitrasparente»* per la stessa ragione: è memoria, non vista.

⚖️ Ma il costo reale è più alto di *«costa di più»*: sono **tre** pezzi, non uno — la cattura al momento
giusto, la classe di animazione dedicata, **e la posa congelata come ripiego**.

**Raccomandazione**: **una sola issue**, due criteri di accettazione in sequenza — prima il ripiego, che da
solo toglie le catene dallo schermo e non introduce leak; poi lo snapshot, che lo sostituisce quando esiste.

⛔ **Non due issue**: il ripiego senza lo snapshot resta legittimo, lo snapshot senza il ripiego è
**incompleto** — non copre i contatti da rumore.

⛔ **Il vincolo non negoziabile**: lo snapshot non entra in `FRTLastKnownContact`. Se qualcuno lo propone, la
risposta è il §2 di questo referto.

---

## 5. Cosa non è stato fatto, e perché

- **Nessuna riga di codice.** Il panel è documentale (CLAUDE.md §6), e la proposta tocca il confine fra
  presentazione e simulazione: il posto dove decidere è un referto, non un diff.
- **Nessun `D-nnn`.** Il panel misura, non decide. Se l'autore accetta il vincolo del §2 — *la posa non entra
  nello stato di gioco* — quello sì è materiale da Decision Log, e va preso al momento della scrittura.
- **Suite non eseguita.** Nessuna riga di codice toccata: non c'è misura da produrre (**D-222**).
