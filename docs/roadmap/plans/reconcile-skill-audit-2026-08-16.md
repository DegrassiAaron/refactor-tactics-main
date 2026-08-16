# Riconciliazione skill/ability — audit delle claim falsificabili

> `SNAPSHOT` · **2026-08-16** · Mandato: `CLAUDE_Reconcile_v0.1_Skill_Ability_Issues_2026-08-16.md`
> **Cosa è**: il §12 del mandato eseguito sulla parte **meccanicamente falsificabile** — le affermazioni
> che una misura può smentire. 23 issue aperte esaminate.
> **Cosa non è**: la scheda completa per issue che il mandato descrive (owner doc, code reality, DoD
> changes). Quella richiede di leggere 23 body interi più i loro owner, ed è un altro ordine di
> grandezza: il perimetro è dichiarato qui sotto invece di essere lasciato intendere.

## Il mandato regge alla verifica

Prima di applicarlo l'ho misurato, perché due kit dello stesso giorno avevano asserito cose false. Questo no:

| Affermazione del mandato | Misura |
|---|---|
| #583 dichiara ancora «bloccata da #165» | ✅ **sei** punti |
| #165 è chiusa | ✅ `CLOSED` |
| #152 dice «l'ultima epic della v0.1» | ✅ riga 73 |
| `Gadget.Sprinkler` è sorgente alternativa d'acqua | ✅ e regge in partita |
| I gate accettano `--check` | ✅ `links`, `naming`, `symbols`: verdi |

🔴 **Un solo difetto, e non è un errore: è invecchiamento.** Il §2 tratta **#1006 come da riconciliare** e chiede di verificare la matrice A/B/C/D. #1006 è `CLOSED`: opzione **C** decisa e implementata lo stesso giorno (PR #1008). `CircularTide` non applica più `Wet`, `FluidTrail` è tornata `Action.Dash`, e il catalogo **produce davvero** il grado `Access`. La frase *«con la grammatica di #995 questo rende Phase `Master`»* era vera quando è stata scritta. Applicare quel paragrafo alla lettera rifarebbe una misura già fatta e riaprirebbe una decisione già presa.

## Metodo

Due controlli automatici sui body delle issue **aperte**, con le righe stampate intere per la classificazione umana — nessuna euristica ha deciso da sola:

1. **Dipendenza stale**: una riga che contiene una parola di dipendenza (`bloccat`, `dipend`, `prerequisit`, `innesco`, `finché`, …) **e** un riferimento `#N` il cui stato è `CLOSED`.
2. **Claim di unicità**: `l'ultima`, `l'unico`, `nessun altro`, applicate a epic, checkpoint, produttori o consumatori.

Il primo controllo è ciò che **#738** vuole rendere un gate. Finché quella issue è aperta il gate non esiste, e questo referto è uno strumento one-shot — non la sua sostituzione.

## Esito

### 🔴 Stale blocker veri — 2

**#583** — *La condizione dichiarata di D-109 ha bisogno di un produttore*. Dichiara il blocco da **#165** (`CLOSED`) in **sei** punti: il banner, il paragrafo di contesto, **due caselle di DoD** marcate `⛔ Bloccata da #165`, la riga *«Innesco per riaprire il lavoro: il merge di #165»* e la nota sul secondo commento. Il mandato lo aveva già trovato.

**#403** — *BAL-1: decidere il confine fra Guard e Brace*. Dichiara *«Bloccata da #400 e #401»*: **entrambe `CLOSED`**. ⚠️ **Il mandato non lo nomina** — lo tratta come decisione d'autore aperta (§7) senza sapere che è dichiarata bloccata da due issue chiuse. Ed è una `question`: la decisione BAL-1 è **sbloccata**, e il suo body dice il contrario.

### 🔴 Claim di unicità falsa — 1

**#152** — riga 73: *«È l'ultima epic della v0.1 e la prima da tagliare»*. Esistono **E46** (#934, frontend shell) ed **E47** (#952, autobattle), entrambe `v0.1` e `P1`. La seconda metà della frase — «la prima da tagliare» — non è stata verificata qui: è una scelta di roadmap, non una misura.

⚠️ La riga 38 della stessa issue dice *«`14.7` è l'unico CP che può cadere da solo»*: **non verificata**, è una claim interna a E14 e richiede di leggere le dipendenze dei suoi checkpoint. Resta da misurare.

### 🟡 Dipendenze soddisfatte, non aggiornate — 3

Non sono false, ma non dicono che il checkpoint è **eseguibile**, che è l'informazione utile:

| Issue | Riga | Stato reale |
|---|---|---|
| **#61** | «Dipende da: #60» | #60 `CLOSED`. E la casella *«dichiarano la dipendenza da #64/#69 **se questi non sono chiusi**»* è **vacua**: entrambe chiuse |
| **#63** | «Dipende da: #62» | #62 `CLOSED` |
| **#78** | «Dipende da: #53, #77» | #53 `CLOSED`, #77 aperta — parzialmente soddisfatta, e non lo dice |

### ✅ Il modello corretto esiste già nel repository

**#77** scrive: *«**Dipende da**: #45, #59 (**entrambe chiuse** — il checkpoint è eseguibile)»*. È la forma da replicare nelle tre righe qui sopra: nomina la dipendenza, ne dichiara lo stato, e ne trae la conseguenza operativa.

### ⚪ Falsi positivi del filtro — 6

#166, #314, #319, #159, #738, #995 citano issue chiuse come **provenienza** (*«arrivata da CP 14.5»*), come **cronaca di una correzione** (*«il prerequisito era scaduto: #318 è chiusa»*), o dentro caselle **già spuntate**. Vanno lasciate stare: riscriverle toglierebbe la storia senza aggiungere verità.

⚠️ Il rapporto è **6 falsi positivi su 12 segnalazioni**. Un gate per #738 che segnalasse tutte queste righe verrebbe disattivato entro una settimana: la lezione per chi implementa quella issue è che il filtro deve distinguere **«dipende da»** da **«è arrivata da»**, e ignorare le caselle spuntate.

## Cosa questo audit NON ha coperto

- **La scheda §12 completa** per le 23 issue: owner doc, code reality, DoD changes, test. Qui c'è solo ciò che una misura falsifica.
- **Le claim sul codice**: il mandato §8 chiede di verificare che nessun documento abbia ri-trasformato `InterceptShot` in Reaction. Non eseguito.
- **La coerenza di roadmap, feature registry, scenario map, PIE** (§13). Non eseguita.
- **Le issue non nominate dal mandato**: l'audit copre le 23 del suo elenco, non tutte le v0.1 aperte.

## Prossimo passo consigliato — uno solo, come il mandato chiede

**#583.** È il difetto più grande (sei punti falsi in una issue `P1`), è già misurato, e la sua correzione non tocca un file del repository — solo il body su GitHub. Chiude il passo **C** della priorità del mandato e sblocca la valutazione di quanto lavoro di #583 sia stato assorbito da **#886**, che è il nodo successivo.

**#403** è il secondo, e costa una riga: dichiarare che le due bloccanti sono chiuse rende visibile che una decisione d'autore è in attesa da giorni senza che nessuno lo sappia.
