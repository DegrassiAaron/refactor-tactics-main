# Facing e direzionalità

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-FACING -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-FACING` · Release: `v0.1` · Roadmap: `E16.1, E16.2`  
> Stato: **IMPLEMENTING** · Gate: `5/9`  
> Scenario: `Spec.Facing.DerivesFromMove`  
> I pezzi che mancano li porta: `RT-FEAT-UI-PLANNING`  
> Decisione accettata (ADR-0005) ma **non implementata**: quello che leggi qui descrive come funzionera', non come funziona oggi.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-FACING -->

## Che cos'è il facing

Il facing è la direzione logica verso cui una unità guarda su una griglia esagonale: una delle **sei direzioni** possibili.

La direzione di design è farlo contare per tre sistemi con **la stessa geometria frontale**:

- difesa;
- percezione;
- reazioni direzionali / Overwatch.

## Come verrà scelto

### Se attacchi o usi un'abilità con un bersaglio

Ti giri verso il bersaglio **prima** che l'azione risolva. Non spari guardando altrove, e quel nuovo
orientamento **resta**: vale per tutto ciò che viene dopo nello stesso round.

Quanto puoi girarti alla fine di un movimento **dipende dal personaggio**, ed è misurato in *step* di 60°:

| Step | Quanto ti giri |
|---:|---|
| 0 | niente: resti come sei arrivato |
| 1 | fino a 60° (la direzione dell'arrivo e le due adiacenti) |
| 2 | fino a 120° |
| 3 | fino a 180°: qualunque direzione |

E ogni personaggio ha **due** valori, perché muoversi e scattare non sono la stessa cosa:

| Personaggio | Alla fine di un Move | Alla fine di un Dash |
|---|---:|---:|
| Flux | 2 | 2 |
| Riva | 2 | 3 |
| Bastion | 1 | 0 |
| Vektor | 3 | 3 |

Bastion è pesante: chiude una carica guardando dove è andato, e basta. Vektor si gira come vuole. Riva è
fluida soprattutto quando scatta.

> ⚠️ Sono **valori di partenza**, non bilanciamento definitivo: cambieranno col playtest.

### Unità ferma

Se non si muove, **chiunque** può scegliere liberamente fra tutte le sei direzioni, senza consumare uno slot
d'azione. Questo non dipende dal personaggio.

### Mentre ti stai muovendo

Durante il movimento guardi sempre **nella direzione dell'ultimo passo che hai fatto**. Conta: se qualcuno ti
tiene sotto Overwatch e scatta a metà tragitto, il lato che gli offri è quello di *quel* momento, non quello
in cui finirai. Girarti alla fine non cambia ciò che è già successo.

## Quando cambia, dentro il round

Il facing non è un valore per turno: cambia **più volte**, e ogni sistema legge il valore più recente al
momento in cui gli serve.

```text
inizio round        eredita il facing finale del round precedente
   ↓ Prep           un'azione con bersaglio ti orienta
   ↓ Dash           una mobilità speciale ti orienta nella sua direzione
   ↓ Blast          il bersaglio del colpo ti orienta, prima che il colpo risolva
   ↓ Overwatch      la zona sorvegliata usa il facing con cui hai armato la guardia
   ↓ Move           l'ultimo passo fissa il facing finale
fine round          quel facing resta, ed è quello con cui inizi il round dopo
```

Il `Move` è l'ultima fase volontaria: il facing con cui **chiudi** il round è quindi una scommessa su dove
arriverà la minaccia nel round successivo. È la stessa forma di impegno anticipato dell'Overwatch armato — se
prevedi male, resti scoperto.

## Stessa cella, facing diverso

Due percorsi che finiscono nello stesso esagono **non** ti lasciano nello stesso stato: entrando da un lato
diverso, l'ultimo passo è diverso, e quindi lo sono le direzioni fra cui puoi scegliere.

Per questo un percorso un po' più lungo può essere la mossa migliore: se ti fa entrare nella cella dal lato
giusto, ci arrivi guardando dove serve. Quanto la cosa ti pesi dipende dal personaggio — Bastion, che alla
fine di un Move si gira di un solo step, deve pianificare l'ingresso; Vektor no.

Un percorso leggermente più lungo può valere di più, se ti fa arrivare guardando dalla parte giusta.

## Movimento forzato

Una unità spinta si orienta verso la **sorgente dell'ultimo spostamento forzato**. Uno spostamento ambientale senza sorgente lascia il facing invariato.

## Arco frontale

L'arco frontale è un cono esagonale di circa **120°**, ottenuto dalla stessa primitiva `HexCone` usata dal progetto.

Questa singola forma deve essere condivisa da difesa, vista e Overwatch: niente tre definizioni diverse di “davanti”.

## Difesa prevista

Un attacco proveniente fuori dall'arco frontale annulla:

- riduzione da **Low Cover (-10)**;
- riduzione da **Guard (-15)**.

Scudi, Deflect, Brace e protezioni non geometriche restano invece valide da ogni direzione.

## Percezione prevista

- vista piena nell'arco frontale fino a `VisionRange`;
- consapevolezza a **360° entro 2 celle**;
- oltre 2 celle, ciò che è dietro non viene percepito soltanto perché è entro il range numerico.

## Overwatch previsto

La direzione sorvegliata deriva dal facing. Non si sceglie un secondo orientamento separato per l'Overwatch.

Di conseguenza un'unità **non** si gira di 180° per consumare una guardia: sorveglia il settore verso cui era
orientata quando l'ha armata.

## Nel Planning

L'anteprima del movimento è prevista mostrare, oltre al percorso e alla destinazione, **il facing finale** e
le direzioni fra cui puoi scegliere. Senza, sceglieresti alla cieca un orientamento che vale per tutto il
round successivo.

Vale la regola generale della pianificazione: vedi le intenzioni della **tua** squadra, mai quelle avversarie.
L'orientamento che un avversario ha **assunto** è visibile perché è una posa; quello che *intende* assumere no.

## Cosa non è ancora deciso

Alcune domande sono aperte e questa pagina non deve inventarne la risposta:

- se una **reazione** possa girare chi reagisce;
- se `Interact` richieda di essere già rivolti verso l'oggetto oppure ti ci giri;
- se **stati alterati** o **terreni** (ghiaccio, condotti) possano limitare la rotazione;
- se `Brace` debba proteggere **solo di fronte** invece che da ogni lato (oggi protegge da ogni lato).

> **Deciso il 2026-08-10**: «personaggi diversi si girano di più o di meno l'uno dall'altro» era una domanda
> aperta, e la risposta è **sì**. La rotazione dipende ora da **chi sei**, non solo da come ti sei mosso —
> vedi la tabella degli step qui sopra.

## Perché è ancora marcato “non implementato”

L'ADR è stato accettato, ma la roadmap indica ancora la catena **E16 → E13 → E14**. La Wiki non deve far credere che il backstab/facing sia già attivo nella build corrente.

## Fonti normative

- `docs/decisions/adr-0005-orientamento.md` — decisione, con l'emendamento §2-bis · ⚠️ §1 e §3 **superate**
- `docs/decisions/adr-0008-rotazione-e-policy-di-facing.md` — la rotazione come **capacità del personaggio**, il facing durante i micro-step, le policy dichiarative
- `docs/decisions/RT_PDR_00_Decision_Log.md` — **D-020**, il facing cambia più volte dentro il round · **D-060**, il modello attuale
- `docs/OPEN_DECISIONS.md` — le sei domande ancora aperte (`FAC-3`, `FAC-5`…`FAC-9`)
