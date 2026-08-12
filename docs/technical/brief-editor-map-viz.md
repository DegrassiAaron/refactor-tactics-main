# Brief — Layer di visualizzazione della mappa in editor

> `CURRENT` · **Stato**: brief di requisiti, **in corso di attuazione** — vedi §9
> **Nato da**: la seduta U1 del 2026-08-10 ([#451](https://github.com/DegrassiAaron/refactor-tactics-main/issues/451)), costruendo `L_HexArena`
> **Non è** [`E21`](../roadmap/roadmap-v0.1.md): quella è la leggibilità **in partita**, per il giocatore

## 1. Perché esiste

Non è una richiesta estetica. In una sola seduta di costruzione sono emersi **tre strumenti mancanti**, tutti
della stessa famiglia, e ciascuno è costato tempo reale:

| | Difetto | Costo osservato |
|---|---|---|
| [#474](https://github.com/DegrassiAaron/refactor-tactics-main/issues/474) | il pennello non sapeva dipingere `bBlocksLineOfSight` | il passo 3 di U1 non era eseguibile con gli strumenti |
| [#479](https://github.com/DegrassiAaron/refactor-tactics-main/issues/479) | l'overlay non mostrava quel flag | si dipingeva un muro e la cella restava grigia |
| [#497](https://github.com/DegrassiAaron/refactor-tactics-main/issues/497) | le celle di partenza erano invisibili | si sono spostate tre volte senza segnale, e il criterio della copertura è definito **sul segmento fra gli spawn** |

A questi si aggiunge un episodio che dice qual è il vero rischio: per un'ora la mappa mostrava **61 esagoni**
mentre `rt.Arena.Check` diceva «non ha celle». Era `DemoRadius`, cioè una vista che raccontava qualcosa che
il dato non conteneva. **Una vista che mente costa più di una vista che manca.**

## 2. Perimetro

**Destinatario**: l'autore della mappa, in editor. Solo lui.

Questo autorizza scelte che in partita sarebbero inaccettabili — colori sgargianti, forme didascaliche,
densità di informazione alta. Non deve piacere a nessuno: deve far capire.

**Non è E21.** Il CP 21.3 chiede che *«i colori delle superfici siano leggibili in partita, non solo
nell'overlay dell'editor»*: guarda il giocatore. La separazione è già dichiarata nel repository — il commento
di `rt.Debug.DrawCells` dice *«è uno strumento di SVILUPPO, non la presentazione definitiva»*. Questo brief
sta dalla parte dello strumento.

## 3. Decisioni prese

### 3a. Mesh 3D generate dal dato, transient

Non linee, non mesh posate a mano: **geometria istanziata leggendo l'asset**, rigenerata a ogni cambiamento.

Il vincolo che rende sicura questa scelta: **niente stato proprio, niente salvataggio nel livello**. Il
pattern esiste già e ha funzionato — `RebuildInstances` si riaggancia a `OnMapChanged` e `PostEditUndo`,
ed è ciò che finora ha impedito alle celle di divergere dall'asset.

Se le pareti finissero nel `.umap` diventerebbero una seconda verità che nessun test verifica, e un binario
in più da versionare in un repository dove `Content/**` è gitignorato per scelta.

> ⚠️ **Il rischio va tenuto sotto tiro.** Le mesh sono la cosa che l'autore guarda; se la rigenerazione manca
> un caso, la scena mostra un muro che nel dato non c'è più. È già successo oggi, in forma benigna, con le
> celle fantasma. Ogni percorso che muta l'asset deve passare per la rigenerazione — e questo è verificabile.

### 3b. Forma per la regola, colore per la superficie

Due canali **indipendenti**, così una cella può dire due cose insieme senza ambiguità:

| Canale | Dice | Esempio |
|---|---|---|
| **forma** | cosa *fa* la cella | parete piena = non ci si passa · lastra bassa = non ci si vede attraverso · gradino = transizione |
| **colore** | di che *terreno* è | fango, Rough, acqua, ghiaccio, fuoco |

Il vantaggio rispetto al codificare tutto col colore: si legge senza ricordare una legenda. Una cella che
blocca la vista ma si attraversa — la cosa che serve a una rotta coperta ma percorribile, e che oggi è la
più fraintesa — diventa *visibilmente* diversa da un muro pieno, invece di essere un anello di un altro
colore.

## 4. Cosa deve diventare leggibile

Tutte e quattro le famiglie, in ordine di frequenza sulla mappa:

1. **Superficie e costo di movimento** — oggi a mappa monotona è tutto grigio uguale, e quanto costa
   attraversare non si vede.
2. **Blocchi: movimento e vista** — oggi due anelli concentrici, leggibili solo da vicino e a overlay acceso.
3. **Coperture, porte e transizioni** — vedi §5: sono proprietà di **bordo**, e l'overlay a cerchi centrati
   non può dirle.
4. **Layer, quota e transizioni fra piani** — oggi c'è solo il filtro `ActiveOnly`, che **nasconde invece di
   spiegare**: isola un piano e non mostra come i piani si collegano.

## 5. I bordi — la parte che richiede attenzione geometrica

Coperture (`FRTHexCover`) e porte (`FRTHexDoor`) hanno un campo `Edge` di tipo `ERTHexDirection`. Le
direzioni sono **sei, pointy-top**:

```
E (+1, 0) · NE (+1,-1) · NW (0,-1) · W (-1, 0) · SW (-1,+1) · SE (0,+1)
```

**Non esistono «nord» e «sud» puri**: l'esagono ha i vertici in alto e in basso, quindi i lati guardano in
quelle sei direzioni. Ogni ragionamento fatto in termini di «via nord / via sud» usa un vocabolario che la
griglia non ha — è un errore commesso durante la seduta stessa.

**Come posizionare una mesh su un lato.** La libreria offre `AxialDirection`, `DirectionTowards` e
`HexCorners`, ma **nessuna funzione per il centro di un bordo**. Il modo che rispetta la disciplina già
stabilita — `MakeCoverYardArena` avverte che *«la direzione si CHIEDE alla libreria invece di scriverla a
mano: se la convenzione dei sei lati cambiasse, un valore inciso qui diventerebbe silenziosamente il bordo
sbagliato»* — è ricavare il bordo dai **due centri di cella**:

- centro del bordo = punto medio fra `AxialToWorld(cella)` e `AxialToWorld(vicino nella direzione)`
- orientamento = la direzione fra quei due centri

Mai angoli incisi, mai indici di `HexCorners` scelti a occhio: se la convenzione dei sei lati cambia, la
geometria derivata dai centri segue, quella incisa mente in silenzio.

Vale la pena valutare se aggiungere alla libreria una funzione per il centro/orientamento del bordo: oggi
manca, e servirà a chiunque debba disegnare qualcosa su un lato.

## 6. Fuori scope

- **La presentazione in partita**: è E21, e questo layer non deve arrivare al giocatore
- **Mesh d'autore**: per uno strumento di lavoro bastano primitive dell'engine — nessuna dipendenza da arte
- **Rendere gli spawn impostabili**: sono derivati da `PickStartCells`, e mostrarli è un'altra cosa dal deciderli
- **Sostituire l'overlay a linee**: le linee costano nulla e funzionano; il layer 3D si aggiunge, non rimpiazza

## 7. Domande ancora aperte

- **Sempre attivo o a comando?** L'overlay attuale ha un interruttore per tool. Un layer 3D che copre la
  mappa potrebbe dare fastidio mentre si dipinge.
- **Quanto costa?** 61 celle × più elementi per cella, rigenerati a ogni pennellata. La pennellata attuale è
  già transazionale e rigenera una volta a fine stroke — va verificato che regga con più geometria.
- **Come si mostra la quota** senza che i piani sovrapposti si nascondano a vicenda: trasparenza, esplosione
  verticale, o un filtro migliore di `ActiveOnly`?
- **Serve una legenda a schermo?** Con forma + colore + bordi, il vocabolario cresce.

## 8. Passo successivo proposto

Il lavoro è troppo grande per una issue e troppo specifico per un'epic esistente. La forma naturale è un
gruppo di issue ordinate per **frequenza sulla mappa** (§4), che è anche l'ordine in cui restituiscono
valore: superficie → blocchi → bordi → layer.

Il primo pezzo è anche quello che chiude il difetto più votato dall'esperienza: **rendere il costo di
movimento visibile senza aprire un pannello**.

## 9. Cosa è atterrato

Questa sezione esiste perché per due giorni la testa del documento ha detto «nessuna implementazione»
mentre il primo pezzo era già in `main`. È lo stesso difetto che il §1 mette in cima — una vista che
racconta qualcosa che il dato non contiene — applicato al brief invece che alla mappa.

| Pezzo | Issue | Stato | Cosa ha lasciato nel codice |
|---|---|:--:|---|
| 1/4 · superficie e costo | [#551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/551) | ✅ chiusa 2026-08-12 | l'ISM `Relief`, `ReliefHeightForCost`, e il pattern «forma per la regola, colore per la superficie» |
| 2/4 · blocchi | [#552](https://github.com/DegrassiAaron/refactor-tactics-main/issues/552) | 🔄 in corso | l'ISM `Blockers`: colonna dove non si passa, lastra dove non si vede |
| 3/4 · bordi | [#553](https://github.com/DegrassiAaron/refactor-tactics-main/issues/553) | ⏳ | — |
| 4/4 · layer e raggiungibilità | [#554](https://github.com/DegrassiAaron/refactor-tactics-main/issues/554) | ⏳ | — |

⚠️ **Il §5 resta la parte non ancora affrontata**, ed è quella con il rischio geometrico: nessuna delle
due implementazioni finora tocca i **bordi**, perché costo e blocchi sono proprietà di *cella*. La
funzione per il centro/orientamento di un lato che il §5 propone **non esiste ancora**.

⚠️ Delle quattro domande del §7 nessuna è stata chiusa: sono state **aggirate**, non risolte. Il layer è
sempre attivo perché finora costa poco, e il costo di rigenerazione con più geometria (§7, seconda
domanda) **non è mai stato misurato** — con `Blockers` gli elementi per cella diventano fino a tre.
