# Griglia esagonale e geometria del mondo

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-STANDABILITY -->

> ⚠️ **Progettata, non implementata.** Questa pagina descrive una meccanica **decisa e documentata** che il gioco **non esegue ancora**: oggi non è giocabile. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-STANDABILITY` · Release: `v0.2` · Roadmap: `—`  
> Stato: **DESIGNED** · Gate: `0/8`  
> Scenario: `Spec.Map.WallCrossesCellStillStandable (pianificato)`  
> La pagina Wiki spiega il **principio** (la griglia non vincola i muri); il dato cotto e la pipeline di cottura non esistono ancora.  
> Verificato il `—` su `—`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-STANDABILITY -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-TRANSITION-CLEARANCE -->

> ⚠️ **Progettata, non implementata.** Questa pagina descrive una meccanica **decisa e documentata** che il gioco **non esegue ancora**: oggi non è giocabile. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-TRANSITION-CLEARANCE` · Release: `v0.2` · Roadmap: `—`  
> Stato: **DESIGNED** · Gate: `0/8`  
> Scenario: `Spec.Map.ValidCellsBlockedTransition (pianificato)`  
> Il caso «due celle valide, passaggio chiuso» è già spiegabile; il dato che lo esprime senza passare da una copertura non esiste ancora.  
> Verificato il `—` su `—`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-TRANSITION-CLEARANCE -->

## In breve

> **La griglia esagonale dice dove una unità può stare e come può spostarsi. Non dice che forma devono avere muri, stanze, edifici e porte.**

È la domanda che quasi tutti fanno guardando una mappa per la prima volta: *se il pavimento è fatto di esagoni, i muri devono seguire i lati degli esagoni?*

**No.** Un edificio ha angoli a 90°, stanze rettangolari e porte messe dove serve. La griglia viene **sovrapposta** a quella geometria, non il contrario. Un muro può tagliare un esagono a metà, di sbieco, o sfiorarlo appena.

```text
   ⬡ ⬡ ⬡ ⬡ ⬡          ⬡ ⬡ ⬡ ⬡ ⬡
   ⬡ ⬡ ⬡ ⬡ ⬡    NON    ⬡ ⬡ ⬡ ⬡ ⬡
   ⬡ ⬡ ⬡ ⬡ ⬡           ⬡╱⬡╲⬡╱⬡╲⬡
   ┌─────────┐          ⬡ ⬡ ⬡ ⬡ ⬡
   │  stanza │          muri costretti a
   └─────────┘          zigzagare sui lati
   muro dritto,
   angoli a 90°
```

Il secondo disegno è quello che il gioco **non** fa: produrrebbe mappe che sembrano fatte di alveari.

## Perché allora ci sono gli esagoni?

Perché servono a rispondere a due domande diverse, e solo a quelle:

| Domanda | Risposta |
|---|---|
| **Dove posso stare?** | Su una cella *calpestabile* |
| **Posso andare da qui a lì?** | Se la *transizione* fra le due celle è aperta |

Tutto il resto — che aspetto ha il muro, quanto è spesso, dov'è la maniglia della porta — è geometria del mondo, e non deve incastrarsi in un esagono.

## Quando una cella è calpestabile

Non conta *quanta parte* dell'esagono resta libera. Non esistono regole tipo «l'hex è occupato al 30%».

Conta una cosa sola: **l'unità ci sta?**

```text
Metti l'ingombro dell'unità sul centro della cella
   └── tocca il muro?
         SÌ  → cella non calpestabile
         NO  → cella calpestabile
```

### Esempio 1 — il muro attraversa la cella, la cella resta buona

Il muro taglia un angolo dell'esagono, ma passa lontano dal centro. L'unità ci sta comodamente.

**Cella valida.** Il fatto che l'esagono sia visivamente «tagliato» non conta.

### Esempio 2 — il muro passa dal centro, la cella salta

Qui il muro attraversa proprio il punto dove l'unità dovrebbe stare, o le passa così vicino da toccarla.

**Cella non valida.** Nessuna mezza cella, nessun «ci sto di sguincio».

## Due celle buone, passaggio chiuso

Questo è il caso che sorprende, ed è il motivo per cui *stare* e *passare* sono due domande separate.

### Esempio 3 — muro fra due celle valide

```text
   [ A ]  ║  [ B ]
    ok    ║   ok
          ║
      il muro sta in mezzo
```

`A` è calpestabile. `B` è calpestabile. **Ma da `A` non si arriva a `B`**: il muro sta in mezzo, e per passare bisognerebbe attraversarlo.

Non basta controllare partenza e arrivo: si controlla **il corridoio** che l'unità percorre.

### Esempio 4 — la porta apre il passaggio

```text
   [ A ]  ║ ▯ ║  [ B ]        porta APERTA   → si passa
   [ A ]  ║ ▮ ║  [ B ]        porta CHIUSA   → non si passa
```

Stessa coppia di celle, esito diverso a seconda dello stato della porta. E la porta **non deve stare su un lato dell'esagono**: sta dove l'architettura la mette, e il gioco ne ricava quali passaggi apre.

Quando una porta cambia stato, i percorsi già calcolati vengono ricalcolati — vedi [[Topologia dinamica|Meccanica-topologia-dinamica]].

## Il facing non segue i muri

Un personaggio guarda in una delle **sei direzioni tattiche**, sempre. Anche se è appoggiato a un muro storto, anche se l'angolo dell'edificio è a 90°.

Il facing descrive **dove sta guardando il personaggio**, non come è orientata la parete che ha accanto. I due non devono coincidere.

Dettagli: [[Facing e direzionalità|Meccanica-facing-e-direzionalita]].

## Copertura, vista e passaggio sono cose diverse

Un errore facile è pensare che siano la stessa cosa. Non lo sono:

| | Domanda |
|---|---|
| **Passaggio** | posso attraversare? |
| **Vista (LOS)** | posso vedere? |
| **Copertura** | quanto mi protegge? |
| **Traiettoria** | il colpo arriva? |

Si può passare da un punto e non vederci attraverso. Si può vedere un bersaglio e non avere traiettoria libera. Una copertura bassa lascia passare la vista e riduce il danno; una alta chiude il bordo del tutto.

Vedi [[Coperture|Meccanica-coperture]] e [[Collisioni|Meccanica-collisioni]].

## Lo stato di questa pagina

Il **principio** descritto qui è deciso e non cambia: la griglia non vincola la geometria del mondo.

Quello che **non esiste ancora** è la parte che lo mette in pratica sulle mappe — la calpestabilità ricavata automaticamente dalla forma dei muri e i passaggi chiusi che non sono coperture. È lavoro previsto per la **v0.2**, e i riquadri in cima alla pagina ne dicono lo stato aggiornato.

Oggi le mappe si costruiscono dichiarando a mano quali celle sono bloccate e quali bordi hanno una copertura: il risultato in partita è lo stesso, il lavoro per l'autore della mappa è di più.

## Pagine collegate

- [[Coperture|Meccanica-coperture]]
- [[Porte|Meccanica-porte]]
- [[Collisioni|Meccanica-collisioni]]
- [[Topologia dinamica|Meccanica-topologia-dinamica]]
- [[Facing e direzionalità|Meccanica-facing-e-direzionalita]]
- [[Ponti|Meccanica-ponti]]
