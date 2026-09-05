# <Nome personaggio>

> **Release:** `<v0.x>` · **Design status:** `<status>` · **Hero_Key:** `<stable key>`

## Panoramica

<descrizione sintetica del personaggio, senza introdurre regole non presenti nei dati>

## Identità tattica

<ruoli, specializzazione, range, tipo danno, player question>

## Meccanica firma

### Descrizione della meccanica

<spiegare in prosa: stato/risorsa, come si attiva, cosa premia, cosa deve preparare il giocatore>

### Lettura tattica

**Obiettivo del giocatore.** <cosa cerca di ottenere>

**Misplay / Failure State.** <cosa resta in mano al giocatore quando usa la meccanica **correttamente secondo le
regole** ma legge male il turno>

**Counterplay / rischio.** <come l'avversario può rispondere usando solo regole definite>

> **`Misplay` non è `Counterplay`.** Il counterplay è ciò che fa **l'avversario** per neutralizzare la
> Signature; il misplay è la conseguenza di una decisione sbagliata di **chi la usa**, senza che nessuno lo
> abbia contrastato. Servono a due domande diverse: *«come lo fermo?»* e *«come posso sprecarlo?»*.
> Compilare il campo con «fa meno danno» significa non averlo compilato: deve nominare la **decisione
> specifica** che è andata storta e il suo costo. Vedi [D-032](../decisions/RT_PDR_00_Decision_Log.md).

### Dati della meccanica

<Mechanic_ID, framework, dipendenze, trigger, payoff, **misplay/failure state**, counterplay, telegraphing, status>

## Statistiche base

<valori + Data_Status; non inventare campi mancanti>

## Visione e stealth

<valori + stato>

## Mobilità

<valori + stato>

<!-- 26e0 La sezione "Risorsa firma" e' stata RIMOSSA da questo template il 2026-09-05 (D-265, D-324, #2357).
     Non esiste una risorsa firma universale: l'economia comune del turno e' slot, cooldown e drawback, e
     Energy — l'unica implementazione viva di quel modello — e' uscita dal gameplay con #610.
     Finche' il template la portava, ogni scheda nuova nasceva descrivendo un sistema che non esiste.

     Un kit PUO' dichiarare una risorsa propria: D-265 lascia la via aperta. Ma allora la sezione si aggiunge
     a quella scheda con il suo contratto di dati e la sua validazione — non si eredita da qui. -->

## Ownership del kit

> Le abilità sotto appartengono a questo personaggio. Le sinergie con altri eroi sono esempi derivati e vanno collegate alla [pagina sinergie](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/sinergie-e-combinazioni), non modellate come abilità condivise.

## Profilo di attacco base

> **Sezione obbligatoria.** `BasicAttack` è una categoria universale con **payload dell'eroe**
> ([ADR-0007](../decisions/adr-0007-attacco-base-per-eroe.md), che chiude per l'attacco base i profili
> concreti lasciati aperti da [D-033](../decisions/RT_PDR_00_Decision_Log.md)). Il profilo va **dichiarato**,
> non dedotto dai numeri: due eroi con lo stesso danno possono avere ruoli opposti, e un eroe il cui attacco
> base non ha un ruolo dichiarato finisce per averne uno per caso.

| Campo | Valore |
| --- | --- |
| Ability ID | `<Hero.Nome>` |
| Famiglia | `Primary Weapon` · `Engine` · `Setup` · `Utility / Emergency` |
| Danno / portata | <dal catalogo eroi, non da qui> |
| Payload oltre il danno | <status, spinta, risorsa — oppure «nessuno», che è una risposta> |
| Dipendenza dal base | ★☆☆☆☆ … ★★★★★ — **quanto spesso entra nel gioco normale**, NON quanto è forte |

### Il test della falsa scelta

> Un attacco base a danno basso non deve essere un pulsante finto. Se a una di queste domande non esiste una
> risposta concreta, il profilo è una falsa scelta e va riprogettato — non documentato meglio.

| Domanda | Risposta |
| --- | --- |
| Quando è la scelta corretta? | |
| Quando è inferiore a un'abilità firma? | |
| Che cosa risparmia (risorsa, cooldown, occasione)? | |
| Che counterplay esiste? | |
| Che cosa impara il giocatore usandolo? | |

### Prove

> Il profilo non è dimostrato da questa pagina: è dimostrato da qualcosa che **può diventare rosso**.
> Se una riga è vuota, dirlo — una casella vuota è un'informazione, una casella inventata no.

| Che cosa | Dove |
| --- | --- |
| Il payload è nel dato | <test di catalogo> |
| L'effetto si vede in partita | <scenario> |
| La famiglia è leggibile a schermo | <voce PIE, se applicabile> |

## Abilità

### <Nome skill>

#### Descrizione

<descrizione gameplay della skill basata su effetto, range, forma, fase/fallback e interazioni definite>

<tabella dei valori strutturati>

#### Uso tattico e limiti

<quando serve, cosa prepara/sfrutta, trade-off, PARTIAL/DEFERRED se applicabile>

<Ripetere per ogni skill. Nessuna skill ufficiale deve restare senza descrizione.>

## Fast Reactions / Reaction

### Descrizione delle reazioni

<una voce per ogni reaction, con trigger, risposta e stato di implementazione/review>

<tabella reaction>

## Equipaggiamento

<catalogo applicabile>

## Varianti

<trade-off orizzontali; nessun upgrade puro>

## Talenti

<NOT DEFINED se assenti>

## Stato produzione

<stato per area>

## Stato della pagina

<distinguere canone v0.1 da DATA_SPEC/DESIGN_SPEC v0.2>

## Governance

- Dataset corrente: `docs/src/data/characters-wiki-data-v0.4.xlsx`.
- La descrizione editoriale non crea nuovi valori competitivi.
- I campi mancanti restano mancanti.
- La Wiki non è una fonte runtime.
