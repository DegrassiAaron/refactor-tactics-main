# Matrici — stati e trasformazioni dei personaggi

> `CURRENT` · **Stato**: strumento di design e tracciabilità · **Data**: 2026-08-08
> **Owner della regola**: [`../gameplay/brief-stati-personaggio-e-trasformazioni.md`](../gameplay/brief-stati-personaggio-e-trasformazioni.md)
> · [D-035](../decisions/RT_PDR_00_Decision_Log.md) · epic **E34**
> **Autorità**: nessuna sui numeri e sulle regole. Qui si tiene traccia di **candidature e stato**, non si
> decide gameplay. In conflitto prevalgono il brief owner, i cataloghi [`../balance/`](../balance/) e le schede
> personaggio.

**Nessun personaggio ha oggi uno stato assegnato.** Ogni riga è una *candidatura*, e la colonna
`DesignStatus` dice quanto vale: `IDEA · PROPOSED · PROTOTYPE · VALIDATING · APPROVED · DEFERRED · REJECTED`.
Al 2026-08-08 nessuna riga supera `PROPOSED`.

## Perché in markdown e non in un foglio di calcolo

Il progetto ha tre `.xlsx`, e **nessuno script li legge**: sono dump di consultazione. Uno di essi è già stato
declassato a `RESEARCH` da [D-023](../decisions/RT_PDR_00_Decision_Log.md), che ha spostato l'autorità dei
numeri sui cataloghi markdown. Mettere qui una matrice di design in formato binario ricreerebbe il problema che
D-023 ha chiuso: un file non diffabile, non revisionabile in PR e che nessun gate può controllare.

Il markdown è quindi la **fonte**; chi ha bisogno del foglio lo genera:

```bash
python scripts/build-state-matrices-xlsx.py     # legge questo file, scrive un .xlsx non versionato
```

Così `RecommendedCandidate`, `DesignStatus` e `TargetVersion` non possono divergere fra documento e workbook —
il requisito §14B — perché esiste una sola copia scritta a mano.

> **Matrix 6 (Feature Traceability) non è qui.** È in costruzione come registro dedicato,
> `docs/roadmap/feature-registry.{json,yaml}` con `scripts/feature_registry.py`. Duplicarla qui creerebbe
> esattamente la seconda verità che quel registro esiste per evitare.

---

## Matrix 1 — Character × State Type

Quali categorie del framework userebbe ciascun personaggio, **se** il tema si aprisse. Una colonna vuota non è
un buco da riempire: la maggior parte dei personaggi non deve avere uno stato speciale.

| CharacterId | Nome | Versione | Stance | Form | Overdrive | Environmental | Configuration | PrimaryStateType | Candidato consigliato | DesignStatus |
|---|---|---|---|:--:|:--:|:--:|:--:|---|---|---|
| `Hero.Flux` | Flux | v0.1 | | | | ✅ | | Environmental | Charged | `PROPOSED` |
| `Hero.Riva` | Riva | v0.1 | ✅ | | | | | Stance (leggero) | Flow State | `PROPOSED` |
| `Hero.Bastion` | Bastion | v0.1 | ✅ | | | | ✅ | Stance/Configuration | Bulwark | `PROPOSED` |
| `Hero.Vektor` | Vektor | v0.1 | | ⚠️ | | | | Form | — **vedi nota** | `REJECTED` |
| `Hero.Steel` | Steel | v0.2 | ✅ | | | | | Stance | Guard ↔ Assault | `PROPOSED` |
| `Hero.Aurora` | Aurora | v0.2 | | ✅ | | ✅ | | Environmental | Frostbound | `PROPOSED` |
| `Hero.Murdock` | Murdock | v0.2 | ✅ | | | | ✅ | Configuration | Targeting Mode | `PROPOSED` |
| `Hero.Kwang` | Kwang | v0.2 | ✅ | | | ✅ | | Environmental | Stormbound | `IDEA` |
| *34 candidati Paragon* | — | — | | | | | | vedi Matrix 2 | — | `IDEA` |

> ⚠️ **Vektor è l'unico `REJECTED`, ed è deliberato.** Sia il documento sorgente sia il prompt di
> consolidamento lo indicano come candidato principale per `Mobile ↔ Siege`. La candidatura è respinta nel
> merito: `Slancio` **recupera muovendosi** e la player question è «dove passerà il nemico?». Una forma che
> toglie il Dash non sospende una statistica, spegne la meccanica firma. Non si scarta l'idea `Mobile ↔ Siege`
> — si scarta **Vektor** come suo portatore: i kit già costruiti sul trade-off mobilità/precisione sono
> Howitzer e Murdock.

---

## Matrix 2 — Transformation Candidate Matrix

La matrice principale: **conserva tutte le alternative**, anche quelle non scelte. `L`/`M`/`S` sono
alternative fra loro, **non** feature cumulative. Una candidata scartata come trasformazione può tornare come
passiva, skill, ultimate, meccanica di scenario o modificatore PvE — ed è la ragione per cui non si potano.

Peso indicativo su 10, dal documento sorgente: **non misurato**, serve a confrontare due proposte.

### Roster v0.1

| Character | L — Light | M — Medium | S — Signature | Consigliato | Tier | Decisione | Target |
|---|---|---|---|---|---|---|---|
| Flux | Charged State *(3)* | Conductor Mode *(5)* | Living Current *(7)* | **Charged** | L | `PROPOSED` | post-v0.1 |
| Riva | Flow State *(2)* | Mist Form *(5)* | Tidal Form *(7)* | **Flow State** | L | `PROPOSED` | post-v0.1 |
| Bastion | Fortified *(2)* | Bulwark Mode *(5)* | Citadel Form *(7)* | **Bulwark** | M | `PROPOSED` | post-v0.1 |
| Vektor | Stabilized *(3)* | Siege Mode *(5)* | Weapons Platform *(7)* | — | — | `REJECTED` | — |

### Roster v0.2

| Character | L | M | S | Consigliato | Tier | Decisione | Target |
|---|---|---|---|---|---|---|---|
| Steel | Guard Stance | Assault Stance | Juggernaut | **Guard ↔ Assault** | L/M | `PROPOSED` | v0.2+ |
| Aurora | Frostbound | Crystal Form | Winter Avatar | **Frostbound** | L | `PROPOSED` | v0.2+ |
| Murdock | Targeting Mode | Hunter Mode | Deadeye Protocol | **Targeting** | L | `PROPOSED` | v0.2+ |
| Kwang | Blade Stance | Storm Stance | Stormbound | **Stormbound** | S | `IDEA` | futuro |

### Candidati Paragon — alternative conservate

Nessuna è in scope. La colonna «Tier alto» segna i candidati che il prompt di consolidamento chiede di
mantenere come **da studiare**, non come lavoro.

| Character | L | M | S | Tier alto | Categoria consigliata |
|---|---|---|---|:--:|---|
| GRIM.exe | Defense Matrix | Attack Matrix | Kernel Override | ⭐ | Configuration — *il candidato più modulare del roster* |
| Crunch | Combo State | Power Routing | Overclock | ⭐ | Configuration |
| Serath | Radiant State | Corrupted Form | Ascended/Corrupted Duality | ⭐ | Form — *due identità tattiche chiare* |
| Terra | Stoneguard | Earthbound | Living Rampart | ⭐ | Environmental |
| Gideon | Void Charge | Gravity Stance | Event Horizon | ⭐ | Environmental |
| Morigesh | Marked State | Effigy Form | Swamp Avatar | ⭐ | Environmental |
| Wukong | Agile Stance | Combat Stance | Clone Form | ⭐ | Form — *costo cognitivo alto nel planning simultaneo* |
| Howitzer | Artillery Stance | Demolition Mode | Siege Platform | ⭐ | Form — *banco di prova coerente per `Mobile ↔ Siege`* |
| Riktor | Lockdown | Execution Mode | Chain Warden | ⭐ | Environmental |
| Iggy & Scorch | Heat State | Iggy ↔ Scorch | Inferno Engine | ⭐ | Environmental |
| Grux | Bloodrush | Rampage Stance | Berserker Form | | Overdrive |
| Khaimera | Hunt State | Frenzy | Primal Form | | Overdrive |
| Countess | Blood State | Shadow Mode | Blood Queen | | Overdrive |
| Sevarog | Soulfed | Soul Armor | Ascended Form | | Overdrive |
| Boris | Hunt State | Predator Mode | Cyber-Beast Overdrive | | Overdrive |
| Rampage | Adrenaline | Beast Mode | Monstrous Form | | Overdrive |
| Muriel | Guardian Mode | Flight Support | Seraph Form | | Overdrive |
| Greystone | Battle Resolve | Undying Stance | Revenant Form | | Overdrive — *non «seconda vita + buff»* |
| TwinBlast | Gun Mode | Overdrive | Akimbo Configuration | | Configuration |
| Gadget | Engineering Mode | Combat Engineer | Network Mode | | Environmental — *attenzione: somma con il sistema gadget* |
| Drongo | Hot Ammo | Scrapper Mode | Mad Bomber | | Environmental |
| Wraith | Recon Mode | Phase Hunter | Temporal Shift | | Form — *pesante: tocca Action Ghost e planning* |
| Lt. Belica | Suppression Mode | Enforcer Mode | Null Field Protocol | | Environmental |
| Zinx | Regeneration State | Chemical Mode | Bio-Reactor | | Environmental |
| Dekker | Control Stance | Mobility Stance | Arena Architect | | Environmental |
| Kallari | Cloaked State | Hunter Mode | Void Form | | Form |
| Feng Mao | Offensive Stance | Defensive Stance | Momentum Form | | Stance |
| Yin | Flow Stance | Wind Stance | Tempest Form | | Stance |
| Shinbi | Performance State | Spirit Pack | Spirit Form | | Overdrive — *attenzione: somma con il summon* |
| The Fey | Bloom State | Thorn Form | Wild Growth | | Environmental |
| Phase | Linked State | Rescue Mode | Symbiosis | | Form |
| Narbash | Rhythm State | Warbeat ↔ Healbeat | Battle Concert | | Stance — *M molto leggibile* |
| Sparrow | Focus State | Volley Stance | Storm of Arrows | | Stance |
| Revenant | Loaded State | Duelist Mode | Nether Form | | Form — *S molto invasiva sulle regole* |

---

## Matrix 3 — State × Gameplay System Interaction

Il **costo sistemico** di ogni candidata consigliata: quanti sistemi tocca. Serve a capire che una
trasformazione non è mai locale al personaggio.

| Stato | Attivazione | Prep | Dash | Blast | Move | Terreno | Cover | LOS | Overwatch | Ghost | Note |
|---|---|---|---|---|---|---|---|---|---|:--:|---|
| Flux · Charged | trigger ambientale | — | modificato | modificato | normale | **forte** | — | — | possibile | ✅ | Dipende dal canale ambientale di **E13** |
| Riva · Flow | trigger su sequenza | — | normale | bonus possibile | normale | possibile | — | — | — | consigliato | Il più leggero: nessun override di kit |
| Bastion · Bulwark | stance in Planning | transform | normale | normale | ridotto | — | **pseudo-cover** | **modifica** | modificato | ✅ | Tocca collisione e pathfinding: il più invasivo dei tre |
| Steel · Guard↔Assault | stance in Planning | transform | modificato | modificato | normale | — | — | — | modificato | ✅ | Nessun sistema nuovo: è un cambio di profilo |

**Lettura**: `Riva · Flow` è l'unico che non tocca nessun sistema condiviso — ed è per questo il primo
prototipo sensato, non l'ultimo. `Bastion · Bulwark` tocca cover, LOS, collisione e pathing: è un'epic a sé.

---

## Matrix 4 — Character Complexity Matrix

Euristica comparativa, **non un punteggio**. Serve alla regola del brief: *chi ha una trasformazione importante
deve essere più semplice altrove.*

| Character | Passive | Skill | Terreno | Reaction | Overwatch | Summon | Trap | Risorse | Trasformazione | Banda | Rischio |
|---|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|:--:|---|---|
| Flux | basso | medio | **alto** | basso | basso | — | — | medio | +basso (L) | `MEDIUM` | Il terreno è già la sua identità: uno stato ambientale **raddoppia** lo stesso asse |
| Riva | basso | medio | **alto** | medio | basso | — | — | medio | +minimo (L) | `MEDIUM` | Stesso rischio di Flux, mitigato dal fatto che Flow non tocca il terreno |
| Bastion | basso | medio | basso | **alto** | medio | — | — | medio | +medio (M) | `HIGH` | Reaction + strutture + pseudo-cover: è già il più carico del roster v0.1 |
| Vektor | basso | medio | basso | **alto** | **alto** | — | — | medio | — | `MEDIUM` | Predictive + reaction + mobilità. Una Form lo porterebbe a `VERY_HIGH` |
| GRIM.exe | ? | ? | ? | ? | ? | — | — | ? | +medio (S) | `?` | Kit non definito: la banda si misura quando esiste |

> **Il caso che la matrice rende visibile**: Flux e Riva sono entrambi personaggi *ambientali*, e i loro stati
> consigliati sono entrambi ambientali. Il budget non si spende due volte sullo stesso asse — è il motivo per
> cui `Riva · Flow` è preferibile a `Riva · Mist Form`.

---

## Matrix 5 — State Validation Matrix

Dove sta ogni candidata rispetto alla verifica. **Tutte le righe sono vuote a valle di `DesignStatus`**, ed è
un'informazione: nessuno stato è stato validato, perché nessuno è stato costruito.

| StateId | Character | DesignStatus | Target | Issue | Scenario | PIE | Automazione | Validato |
|---|---|---|---|---|---|---|---|---|
| `State.Riva.Flow` | Riva | `PROPOSED` | post-v0.1 | [#256](https://github.com/DegrassiAaron/refactor-tactics-main/issues/256) | `State.Riva.Flow` 📋 | `PIE-STATE-01…05` | ❌ | ❌ |
| `State.Flux.Charged` | Flux | `PROPOSED` | post-v0.1 | [#257](https://github.com/DegrassiAaron/refactor-tactics-main/issues/257) | `State.Flux.Charged` 📋 | `PIE-STATE-08` | ❌ | ❌ |
| `State.Bastion.Bulwark` | Bastion | `PROPOSED` | post-v0.1 | [#258](https://github.com/DegrassiAaron/refactor-tactics-main/issues/258) | `State.Bastion.Bulwark` 📋 | `PIE-STATE-06…07` | ❌ | ❌ |
| `State.Howitzer.Siege` | Howitzer | `PROPOSED` | post-v0.1 | [#259](https://github.com/DegrassiAaron/refactor-tactics-main/issues/259) | `State.Howitzer.Siege` 📋 | `PIE-STATE-02…04` | ❌ | ❌ |
| — *(trasversale)* | — | — | post-v0.1 | [#255](https://github.com/DegrassiAaron/refactor-tactics-main/issues/255) | `State.MultiState.Stress` 📋 | `PIE-STATE-09…10` | ❌ | ❌ |
| `State.Vektor.Siege` | Vektor | `REJECTED` | — | — | — | — | — | — |

📋 = **definito, non scritto**. Gli scenari vivono in
[`../gameplay/brief-stati-personaggio-e-trasformazioni.md`](../gameplay/brief-stati-personaggio-e-trasformazioni.md) §9
e si scrivono quando E34 apre: uno scenario che non gira è peggio di uno che manca, perché sembra copertura.

Gli `StateId` sono **proposti**, non Stable ID: non esistono nel codice e non vanno citati come se esistessero.

**La colonna «Validato» è tutta ❌, ed è l'informazione principale della matrice.** Nessuno stato è stato
verificato perché nessuno è stato costruito — e nessuna delle altre matrici deve far sembrare il contrario.

---

## Matrix 7 — Wiki Publication Matrix

Cosa può uscire dal design ed entrare nella guida al giocatore. La regola, dal prompt di consolidamento:
`APPROVED` → pubblicabile · `PROTOTYPE` → solo con lo stato sperimentale visibile · `PROPOSED` → design only ·
alternative L/M/S non scelte → **mai** player-facing.

| Contenuto | DesignStatus | Player-facing | Wiki | Pagina |
|---|---|:--:|:--:|---|
| Il framework esiste come direzione | `PROPOSED` | ❌ | ❌ | — |
| Candidate per personaggio (L/M/S) | `PROPOSED`/`IDEA` | ❌ | ❌ | solo design |
| **Profilo** di azione generica (fisso per eroe) | ✅ deciso ([D-033](../decisions/RT_PDR_00_Decision_Log.md)) | ✅ | ✅ | [`azioni-e-movimento` (Wiki)](https://github.com/DegrassiAaron/refactor-tactics-main/wiki/azioni-e-movimento) |

**Al 2026-08-08 nella wiki non entra nulla di trasformazioni.** L'unica riga pubblicata è il *profilo*, che è
una decisione consolidata e non una trasformazione — ed è già online. È l'applicazione letterale della regola:
la wiki non pubblicizza feature future.
