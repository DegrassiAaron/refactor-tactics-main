# Azioni e movimento

> **Tipo:** guida giocatore, non normativa

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-BASIC-ATTACK-PROFILES -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-BASIC-ATTACK-PROFILES` · Release: `v0.1` · Roadmap: `E4`  
> Stato: **IMPLEMENTING** · Gate: `6/8`  
> Scenario: `Combat.BasicAttack`  
> Verificato il `2026-08-09` su `8a43866`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-BASIC-ATTACK-PROFILES -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-SUPERS -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-SUPERS` · Release: `v0.2` · Roadmap: `—`  
> Stato: **IMPLEMENTING** · Gate: `0/8`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-SUPERS -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-COOLDOWNS -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-COOLDOWNS` · Release: `v0.1` · Roadmap: `E4.4`  
> Stato: **TESTABLE** · Gate: `5/8`  
> Scenario: `—`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-COOLDOWNS -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-DASH-DISPLACEMENT -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-DASH-DISPLACEMENT` · Release: `v0.1` · Roadmap: `E2.5, E2.5`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `Visual.Combat.PushResistance`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-DASH-DISPLACEMENT -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-MOVE-PROFILES -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-MOVE-PROFILES` · Release: `v0.1` · Roadmap: `E4.2, E4.5`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `Visual.Movement.Charge`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-MOVE-PROFILES -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-GENERIC -->

> 🚧 **Parzialmente giocabile.** Il codice esiste ma la feature non è completa: i gate qui sotto dicono quanto manca. Blocco generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-GENERIC` · Release: `v0.1` · Roadmap: `E4.4, E4.6, E4.7`  
> Stato: **IMPLEMENTING** · Gate: `3/8`  
> Scenario: `Combat.BasicAttack`  
> I pezzi che mancano li porta: `RT-FEAT-REACTION-OVERWATCH` · `RT-FEAT-OBJECTIVE-SYSTEM`  
> Quattro azioni generiche su sette sono complete: `Wait`, `Move`, `BasicAttack`, `Guard`. `Brace` funziona ma come reazione; `Interact` è a catalogo **senza effetto**; `Overwatch` non esiste ancora.  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-GENERIC -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-ACTION-ENGINE -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-ACTION-ENGINE` · Release: `v0.1` · Roadmap: `E4.1, E4.3, E4.8`  
> Stato: **RELEASE_READY** · Gate: `7/8`  
> Scenario: `Movement.Collision`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-ACTION-ENGINE -->

<!-- RT_FEATURE_STATUS:BEGIN RT-FEAT-MAP-PATHFINDING -->

> **Stato di sviluppo** — generato dal Feature Registry, non modificare a mano.  
> Feature: `RT-FEAT-MAP-PATHFINDING` · Release: `v0.1` · Roadmap: `E2.2`  
> Stato: **RELEASE_READY** · Gate: `6/7`  
> Scenario: `Movement.Basic`  
> Verificato il `2026-08-08` su `2094b86`

<!-- RT_FEATURE_STATUS:END RT-FEAT-MAP-PATHFINDING -->

## Le azioni generiche

La grammatica comune del gioco è:

```text
Wait · Move · BasicAttack · Guard · Brace · Interact · Overwatch
```

Il kit di un personaggio aggiunge abilità specifiche sopra questa base.

### Wait

Rinunci a un'azione offensiva. Può comunque avere senso per mantenere posizione, facing o preparazioni consentite.

### Basic Attack

L'attacco base del personaggio. Danno, range, forma **e ciò che lascia sul bersaglio** dipendono dall'eroe.

È il comando più usato del gioco, ed è anche quello che si capisce peggio: «attacco base» suona come
«l'abilità debole di riserva». Non lo è. È una **categoria universale** — stessa fase, stesso posto
nell'economia del turno — a cui ogni personaggio dà un ruolo diverso.

I ruoli sono quattro, e sapere quale ha in mano cambia come si gioca:

| Ruolo | Che cosa vuol dire | Chi lo ha in v0.1 |
|---|---|---|
| **Arma primaria** | Lo userai spesso: è una parte vera della tua pressione, e il resto del kit serve a creare le occasioni in cui conviene premerlo | [Vektor](../../characters/v0.1/vektor.md) |
| **Motore** | Non conta solo per il danno: alimenta o prepara una meccanica del personaggio | [Flux](../../characters/v0.1/flux.md) — *in v0.1 il motore elettrico passa dalle sue abilità, non dall'attacco base* |
| **Preparazione** | Il danno è basso di proposito: serve a creare la condizione che qualcun altro sfrutta | [Riva](../../characters/v0.1/riva.md) — applica `Wet`, e la scarica di Flux vale di più su un bersaglio bagnato |
| **Utilità / emergenza** | Danno modesto, ma resta la mossa giusta in situazioni precise: finire un bersaglio, rallentare chi passa, rispondere senza spendere niente | [Bastion](../../characters/v0.1/bastion.md) — applica `Slow` |

**Danno basso non significa pulsante finto.** È la regola di design che tiene insieme le quattro famiglie: se
l'attacco base di un personaggio è *sempre* la scelta sbagliata, è il profilo a essere rotto, non il
giocatore ad avere torto. Ogni scheda personaggio dichiara quando il suo attacco base è corretto, quando non
lo è, e che counterplay esiste — nella sezione *Profilo di attacco base*.

Il rovescio vale altrettanto: un attacco base **forte** non autorizza a ignorare il resto del kit. Bastion
faceva 24 danni fino al 2026-08-09, cioè più di chiunque altro, e quel numero contraddiceva tutto ciò che
l'eroe è.

> La regola sta in [ADR-0007](../../decisions/adr-0007-attacco-base-per-eroe.md); i numeri stanno nel
> [catalogo eroi](../../balance/RT_HeroCatalog_v0.1.md). Questa pagina non è la fonte né dell'una né degli
> altri: se divergono, hanno ragione loro.

### Interact

Interazione con elementi della mappa come porte, console, ponti o obiettivi quando le relative regole lo permettono.

### Guard

Ti metti in guardia. Il **primo** colpo diretto che ricevi nel turno fa **15 danni in meno**; dal secondo in poi incassi per intero. Reggi anche una spinta di **una cella** — non di più: la guardia non è un'ancora.

Copre il **davanti**. Un colpo che arriva alle spalle trova la guardia inutile, e questo è il motivo per cui il facing conta anche quando non ti muovi.

### Brace

Ti irrigidisci. Ogni colpo diretto del turno fa **10 danni in meno** — non solo il primo — e la **prima** spinta non ti sposta, a qualsiasi distanza. In cambio rinunci al movimento volontario.

A differenza di `Guard`, `Brace` protegge la **persona** e non un lato: regge anche da dietro.

> ⚠️ **Il confine fra Guard e Brace è una domanda aperta, non una regola stabile.**
> Sulla carta `Guard` è «reggo il colpo» e `Brace` è «non mi sposti». In partita, oggi, la differenza sullo
> spostamento **non si vede**: nessuna spinta del gioco supera una cella, quindi entrambe la annullano, e
> `Guard` resta avanti di 5 danni. `Brace` conviene solo se ti colpiscono **più di una volta** nello stesso
> turno.
>
> È registrato come `BAL-1` e si chiude con un playtest, non con un documento. I numeri qui sopra sono quelli
> in vigore adesso e possono cambiare.

### Move

Il movimento normale segue un percorso e risolve **dopo il Blast**.

### Overwatch

Rinunci all'azione offensiva immediata per sorvegliare un'area o un trigger e reagire se la previsione si verifica.

## Move ha profili

La direzione di design consolidata tratta:

- **Sneak:** meno distanza, meno rumore/esposizione;
- **Normal:** profilo di riferimento;
- **Sprint:** più distanza, più rumore/esposizione.

I valori numerici definitivi sono ancora da playtestare e la migrazione dal vecchio `Action.Sprint` non è completata.

## Non tutti eseguono la stessa azione allo stesso modo

Il **profilo** non riguarda solo il Move. Le azioni generiche sono lo stesso comando per tutti — stessa fase,
stesse regole, stesso posto nell'economia del turno — ma **come** si comportano dipende dal personaggio.

L'Overwatch è l'esempio più visibile: tutti possono armarlo, ma un tiratore sorveglia un arco stretto a lunga
distanza e risponde con `FIRE` o `HOLD`, mentre un difensore sorveglia un arco largo attorno a un alleato e
risponde intercettando. Non sono due abilità diverse: è la stessa azione con due profili.

Vale la pena saperlo per una ragione pratica: **conoscere il personaggio non basta a prevederlo, serve
conoscere il suo profilo.** Due eroi che dichiarano Overwatch sulla stessa cella non stanno minacciando la
stessa cosa.

Un profilo non è un potenziamento: se dà un vantaggio, ha un prezzo dichiarato — è la stessa logica delle
varianti, dove non esistono scelte gratuitamente migliori.

> In questa versione il profilo di un personaggio è **fisso**: non si cambia durante la partita.

## Mobilità speciale

Queste non sono varianti del Move normale:

- Dash
- Charge
- Leap
- Blink
- Reposition
- displacement forzato

Possono risolvere prima del Blast perché appartengono alla mobilità speciale della relativa fase.

## Regola fondamentale

> **Sprint non è Dash.** Sprint è un profilo del Move; Dash è uno spostamento speciale pre-Blast.

## Fonti normative

- `docs/gameplay/brief-azioni-generiche-overwatch.md`
- `docs/gameplay/spec-sequenza-turno.md`
- `docs/balance/RT_ActionCatalog_v0.1.md`
