# Spec Panel — Consolidamento Base Action Signatures, Brace e Overwatch

> `CURRENT` · **Creato**: 2026-08-10 · **Owner** di **una sola domanda**: che cosa del sorgente
> [`CLAUDE_Consolidamento_BaseAction_Signatures_Brace_Overwatch_2026-08-10.md`](../../archive/src/handoff/2026-08-10-baseaction-signatures-brace-overwatch.md)
> entra nel canone, che cosa è già canone con un esito **diverso**, e che cosa resta aperto.
>
> Non è una specifica e non decide: è il **triage** che precede la decisione. Gli owner restano il
> [Decision Log](../../decisions/RT_PDR_00_Decision_Log.md), gli ADR e i brief citati voce per voce.

## 1. Verdetto

Il sorgente **non era stato consumato**: nessun documento del repository lo cita, e i dieci scenari
`CHAR-BASE-*` che propone non esistono in nessuna forma. Ma «non consumato» non significa «da
applicare»: **la maggior parte della sua sostanza è già canone**, arrivata per altre strade fra il
2026-08-08 e il 2026-08-09, e in tre punti con un **esito diverso** da quello che il documento propone.

| Sezione del sorgente | Stato nel repository | Esito |
|---|---|---|
| §4–5 Base Action Signature | **D-033** — il modificatore si chiama `profilo` | ⚠️ **terzo nome** per la stessa cosa |
| §8 Basic Attack per eroe | **D-058** + [ADR-0007](../../decisions/adr-0007-attacco-base-per-eroe.md), `RT-FEAT-ACTION-BASIC-ATTACK-PROFILES` `runtime: done` | ✅ già chiuso, ma **Flux diverge** |
| §10–12 Brace | **D-047** — `Brace` arma un Reaction Profile, risposta universale `Hold Ground` | ⚠️ **contraddetto** sul danno |
| §13–16 Overwatch (framework) | **D-012**, **D-014**, **CP 14.4** — profilo «dato per eroe, non ramo nel resolver» | ✅ meccanismo pianificato |
| §13–16 Overwatch (i quattro profili) | **non deciso** | 🟢 **contributo nuovo** |
| §11 Brace (i quattro profili) | **non deciso** | 🟢 **contributo nuovo** |
| §14 lifecycle post-Overwatch | superato dall'handoff gemello pari-data | ❌ **non consolidare** |
| §3.2 otto azioni con `Activate` | **D-014** + **D-025** — sette voci, `Activate` assorbita | ❌ **conflitto diretto** |

Il valore reale del documento è **stretto e concreto**: CP 14.4 e CP 14.7 hanno già deciso che i profili
di `Overwatch` e `Brace` sono **dati per eroe**, e hanno il test che lo impone
(`Overwatch.ProfileIsDataNotBranch`). Quello che nessuno ha ancora scritto è **quali** siano i quattro
profili. È esattamente il buco che questo sorgente riempie — e per riempirlo non serve nulla di ciò che
propone intorno.

## 2. Panel — modalità critique

Esperti convocati sul dominio: **Wiegers** (qualità dei requisiti), **Adzic** (esempi eseguibili),
**Cockburn** (attore e obiettivo), **Fowler** (confini e nomi), **Crispin** (oracoli di test),
**Nygard** (modi di fallimento). Ogni rilievo porta l'evidenza misurata sul branch, non l'impressione.

### 2.1 CRITICO — `Activate` è già stata assorbita da `Interact`

**FOWLER**: «Il §3.2 elenca **otto** azioni universali e aggiunge: *"Non fondere `Activate` e `Interact`
senza una nuova decisione esplicita"*. Quella decisione esplicita **esiste già, ed è nel senso opposto**.»

- **D-014**: «`Activate` è **assorbita semanticamente da `Interact`**».
- **D-025**: «L'elenco canonico diventa di **sette** voci: `Wait · Move · BasicAttack · Guard · Brace ·
  Interact · Overwatch`», ed emenda D-014 **solo** su `Guard`.

Il §17 costruisce poi l'intera sezione «Activate / Interact affinity» sopra una distinzione che il canone
non ha. **Raccomandazione**: le affinità (Flux→generatori, Riva→valvole, Bastion→strutture) sono utili e
si tengono — ma vanno espresse come *capability/affinity di `Interact`*, non come seconda azione. Il
documento lo permette già senza modifiche di sostanza: cambia solo l'etichetta.

**Priorità**: alta — un elenco a otto voci ricopiato in Wiki o catalogo diventa una seconda verità.

### 2.2 CRITICO — «Brace non riduce il danno» contraddice il gioco che gira

**WIEGERS**: «Il §10 afferma che il Brace standard *"NON riduce automaticamente il danno"* e *"NON è
difesa universale"*. Questo non è un requisito nuovo: è la **negazione di un comportamento in produzione**,
scritta come se fosse una precisazione.»

Evidenza:

- `Source/RefactorTactics/Combat/RTCombatLibrary.h:112` — `BraceDamageReduction = 10`, «riduce di 10
  **OGNI** danno diretto fino al Cleanup».
- **D-047** — la risposta universale `Hold Ground` «**coincide con il comportamento che il gioco ha già**:
  −10 su ogni danno diretto e blocco della prima spinta. **Nessun numero di bilanciamento cambia**».
- Due scenari verdi lo pinnano: `Visual.Combat.BraceReducesEveryHit`, `Spec.Facing.BraceHoldsFromBehind`.

**NYGARD**: «Il modo di fallimento è silenzioso. Chi implementa leggendo il §10 toglie la riduzione, due
scenari diventano rossi, e la diagnosi punta al codice invece che al documento che l'ha chiesto.»

**Raccomandazione**: il §10 va letto come «Brace **non è** riduzione danno *generica di categoria*» —
che è vero e compatibile con `Hold Ground` — non come «Brace non riduce danno». Se l'intenzione fosse
davvero rimuovere i −10, serve una decisione di bilanciamento con playtest, **non** un handoff.

**Priorità**: alta — tocca numeri già in partita.

### 2.3 CRITICO — un terzo nome per la stessa cosa

**FOWLER**: «`Base Action Signature`, con i tre livelli `STANDARD / VARIANT / SIGNATURE`, descrive
precisamente ciò che **D-033** chiama `profilo`. E D-033 non si limita a scegliere il nome: **respinge
esplicitamente** `GenericActionModifier` proposto da un handoff precedente, con la motivazione che
"sarebbe stata una seconda verità". Questo documento propone la terza.»

**Raccomandazione**: tenere `profilo` come tipo. `STANDARD / VARIANT / SIGNATURE` sopravvivono utilmente
come **etichetta di intensità** in tabella di design (quanto un profilo si allontana dalla baseline), mai
come nome del meccanismo né come campo dati. Il vincolo «max 1–2 base action fortemente SIGNATURE per
eroe» è una **guideline di design leggibile e verificabile a occhio**: si tiene, come guideline.

### 2.4 MAGGIORE — `Deflection` è già preso, e significa il contrario

**COCKBURN**: «Chiedo chi è l'attore e quale obiettivo persegue. Il §11.4 dice: *Vektor, colpito da
Forced Movement, devia lateralmente la traiettoria*. Ma nel roster `Vektor.Deflection` **esiste già** e
l'attore persegue un obiettivo diverso: **subire meno danno**.»

Evidenza — `Source/RefactorTactics/Ability/RTHeroCatalogLibrary.cpp:534-539` (CP 6.7):

> «REAZIONE cablata sulla semantica di `Action.Deflect`: **−20 sul colpo diretto** che l'ha innescata…
> Stessa famiglia di `Action.Guard` (−15 al primo colpo) ma con un trigger invece di una stance.»

Il documento vieta al Brace proprio la riduzione danno (§10), e poi propone per quel Brace il nome di
una reazione che **è** riduzione danno. Due entità, un nome, semantiche opposte.

**Raccomandazione**: decidere una delle due, e la scelta non è cosmetica —
è la domanda **`BAS-3`** del §4.

### 2.5 MAGGIORE — `Flow` è già preso, con un trigger diverso

`Riva.FlowReaction` esiste (`RTHeroCatalogLibrary.cpp:339-347`): `Reposition 1` **dopo un attacco
subito**, dichiarata con slot `None` e nessun trigger perché il suo aggancio è E14. Il §11.2 propone
`Flow` come risposta a **Forced Movement**. Stesso nome, stesso eroe, stessa famiglia (movimento
reattivo), **trigger diverso**.

E non sono due, sono **tre**: `State.Riva.Flow` è già registrato come candidato *stance* in
`RT-FEAT-CHARACTER-STATE` (E34, post-v0.1, `PROPOSED`). Tre accezioni di «Flow» per lo stesso eroe —
una reazione cablata, uno stato futuro, un profilo di `Brace` — di cui due già scritte prima che questo
sorgente arrivasse.

**ADZIC**: «Serve un esempio che distingua i casi, perché la differenza non è teorica: *Riva viene
colpita e non spostata* attiva l'una e non l'altra. Se sono la stessa cosa, il documento sta promuovendo
`FlowReaction` da reazione-a-colpo a profilo-di-Brace, e va detto.»

### 2.6 MAGGIORE — il §14 è già superato da un handoff pari-data

Il sorgente gemello `docs/src/CLAUDE_Overwatch_Runtime_Lifecycle_Watch_Reposition_Consolidation_2026-08-10.md`
(1994 righe, stessa data) dichiara alla riga 7: «questo handoff contiene decisioni **più recenti** rispetto
ad alcuni handoff Overwatch precedenti», e alla riga 101 cita **proprio questo documento** fra gli input
da auditare. Il suo modello del post-Overwatch è **diverso**:

| | Questo documento (§14) | Handoff gemello |
|---|---|---|
| Dopo l'Overwatch | Move normale con **budget ridotto** | **Reposition limitato**, pianificato prima della Resolution |
| Sprint | vietato | non pertinente: non è un Move |
| Struttura | nessuna | **Watch Stage** → End Watch → Reposition simultaneo |

**Raccomandazione**: **non consolidare il §14**. La regola «Overwatch termina prima del proprio Move» è
comune a entrambi e si può tenere; il *cosa succede dopo* appartiene al gemello. Lo scenario CHAR-BASE-008
resta bloccato finché quel modello non è deciso.

### 2.7 MINORE — Flux riapre una decisione chiusa

Il §8 vuole il Basic Attack di Flux come «Engine/Setup elettrico… supporta la sua engine di Conduction».
Ma **D-058** e ADR-0007 hanno già assegnato i quattro ruoli, e per Flux l'esito è documentato nel registry:
«**Flux resta damage-only**, e il suo motore elettrico ha già un owner che non è l'attacco base
(`ConductiveNode`, **D-046**)».

Le quattro famiglie del §8 — `Primary Weapon · Engine · Setup · Utility` — coincidono invece **parola per
parola** con D-058: su questo il documento è già consumato.

### 2.8 MINORE — acceptance criteria senza oracolo

**CRISPIN**: «Dei 22 criteri del §34, quelli che un tester può eseguire sono pochi. *"UI mostra solo scelte
legali"*, *"privacy preservata"*, *"TurnLog spiega gli esiti"* non dicono chi guarda, cosa, e con quale
esito atteso.»

Il metro esiste già nel repository, ed è più alto: ogni checkpoint della roadmap porta i **nomi dei test**
che lo chiudono (CP 14.4 ne elenca sei, fra cui `Overwatch.ProfileIsDataNotBranch`). Il §24 del documento
elenca invece frasi. **Raccomandazione**: i criteri utili sono già mappati nei DoD dei CP 14.4/14.7; non
servono i 22 del §34, serve che i profili d'eroe siano esprimibili dai test già nominati lì.

### 2.9 MINORE — tre convenzioni di ID inventate

| Il documento propone | Il repository usa | Dove |
|---|---|---|
| `Feature.Character.BaseActionSignature` (§22) | `RT-FEAT-ACTION-*`, `RT-FEAT-REACTION-*` | `feature-registry.yaml` |
| `CHAR-BASE-001…010` (§23) | `Spec.<Area>.<Nome>` | `Scenarios/Spec/` |
| milestone `F1…F5` (§28) | epic `E1…E21` | `roadmap-v0.1.md` |

Il documento avverte esso stesso «se il registry usa ID diversi, usare quelli reali»: la mappatura è al §5.

### 2.10 MINORE — tre scenari su dieci sono già coperti

**ADZIC**: «Prima di registrare dieci scenari, verifico quali esistono. Tre non vanno scritti.»

| Proposto | Stato reale |
|---|---|
| CHAR-BASE-006 — HOLD poi INTERCEPT su secondo nemico | **è** `Spec.Overwatch.HoldThenFire`, già dichiarato (esce `BLOCKED`) |
| CHAR-BASE-010 — identità del Basic Attack | già coperto da `Heroes.BasicAttackDeclaresItsBaseAction` + `BasicAttackIsIndexZeroForEveryHero` (verdi) |
| CHAR-BASE-009 — Guard ≠ Brace | è un test unit, e il suo posto è occupato: `Reactions.Brace.IsNotAReaction` **va sostituito** (CP 14.7), non affiancato |

## 3. Che cosa si tiene

Il contributo netto, dopo il triage, sono **due tabelle di contenuto** che nessun documento del
repository possiede, sopra un meccanismo che è già deciso e già pianificato.

### 3.1 Profili `Brace` per eroe — contenuto per CP 14.7

| Eroe | Profilo | Risposte oltre `Hold Ground` | Identità |
|---|---|---|---|
| Flux | Grounding | `GROUND` | condizione/terreno → setup |
| Riva | Flow | `FLOW` (deviazione verso hex adiacente legale) | asseconda e ricolloca |
| Bastion | Anchor | `ANCHOR` | il riferimento anti-displacement del roster |
| Vektor | Deflection ⚠️ | `DEFLECT LEFT` / `DEFLECT RIGHT` | cambia traiettoria, non la ferma |

Regge sul canone: D-047 dice che un profilo con **≥ 2 risposte legali** apre la finestra, e questi ne
dichiarano due o tre. Il vincolo «mostrare solo risposte legali» (§11.2, §11.4) è già l'invariante di
ADR-0004. ⚠️ = collisione di nome, §2.4.

### 3.2 Profili `Overwatch` per eroe — contenuto per CP 14.4

| Eroe | Geometria | Risposta | Identità |
|---|---|---|---|
| Flux | settore medio, direzionale | `DISCHARGE` | riusa la conduction esistente, non la reimplementa |
| Riva | settore medio-corto | `PUSH` | rompe la geometria del piano avversario |
| Bastion | corto, largo, frontale | colpo semplice | presidio del choke |
| Vektor | stretto, lungo, a corridoio | `INTERCEPT` | controlla una traiettoria, non una zona |

Regge sul canone: CP 14.4 impone che il profilo sia «area, arco, trigger e risposte legali» **come dato**,
e che la direzione **nasca dal facing** (ADR-0005 §4c) — questi quattro profili sono esprimibili così.
Nessun numero è dichiarato, coerentemente con §0 del sorgente.

### 3.3 Budget d'identità (§18)

Utile come **criterio di revisione**, non come regola: *Bastion non deve dominare anche l'Overwatch;
Vektor non deve diventare tank per via del Brace*. Si tiene nel triage, non nel canone.

## 4. Decisioni aperte

Nessuna viene chiusa qui. Nessun valore numerico viene inventato.

| ID | Domanda | Perché non si chiude da sola |
|---|---|---|
| **`BAS-1`** | I quattro profili `Brace` (§3.1) entrano nel canone come contenuto di CP 14.7? | Il meccanismo è deciso (D-047), il contenuto no |
| **`BAS-2`** | I quattro profili `Overwatch` (§3.2) entrano come contenuto di CP 14.4? | Idem, D-012/D-014 + CP 14.4 |
| **`BAS-3`** | `Vektor.Deflection` — un nome, due semantiche: si rinomina il Brace, si rinomina la reazione, o si unificano? | Tocca il catalogo eroi già cablato (CP 6.7) |
| **`BAS-4`** | `Riva.Flow` — il Brace è la promozione di `FlowReaction` o una seconda reazione? | Trigger diversi: colpo subìto vs forced movement |
| **`BAS-5`** | Post-Overwatch: budget ridotto (§14) o Watch Stage + Reposition (handoff gemello)? | Due modelli pari-data, il gemello si dichiara più recente |

> **Le affinità di interazione del §17 non aprono una voce nuova.** «Quali capability di interazione porta
> ciascun eroe» è già registrata come **`INT-1`** dal consolidamento del 2026-08-09, con assegnazioni
> discusse — Flux → `Electric`/`Tech`, Riva → `Fluid`, Bastion → `Engineering`/`Force`, Vektor →
> `Precision`/`Sensor` — che il §17 **ricalca** (generatori/pannelli, valvole/pompe, cover/porte/barricate,
> standard). Il sorgente non pone una domanda nuova: **propone una risposta** a una già aperta, e va letto lì.
> Registrarla due volte produrrebbe due ID che si chiudono in momenti diversi, col secondo libero di mentire —
> è la stessa ragione per cui `INT-3` non esiste.

Restano inoltre aperti, come li elenca il §36 del sorgente e senza che questo triage li tocchi: il valore
del movimento residuo, la resistenza numerica del Brace, la quantità di Charge del Grounding, danno o
non-danno della Pressure Overwatch, geometria esatta della Predictive.

## 5. Mappatura sugli ID reali

| Sorgente | Reale |
|---|---|
| `Feature.Reaction.Brace.CharacterProfiles` | `RT-FEAT-REACTION-PROFILE` (E14, CP 14.7) |
| `Feature.Reaction.Overwatch.CharacterProfiles` | `RT-FEAT-REACTION-OVERWATCH` (E14, CP 14.4) |
| `Feature.Character.BaseActionSignature` | **non serve**: è D-033, e vive in `RT-FEAT-ACTION-GENERIC` |
| `Feature.Character.*.BaseActions` | **non serve**: `RT-FEAT-ACTION-BASIC-ATTACK-PROFILES` già chiude il Basic Attack |
| `Feature.Reaction.Overwatch.PostUseMovement` | **sospesa**: `BAS-5` |
| CHAR-BASE-001 | `Spec.Overwatch.ConductiveDischargeUsesStandardConduction` |
| CHAR-BASE-002 | `Spec.Overwatch.PressurePushChangesResolvedPath` |
| CHAR-BASE-003 | `Spec.Brace.AnchorResistsDisplacement` |
| CHAR-BASE-004 | `Spec.Brace.FlowRedirectsToLegalHexOnly` |
| CHAR-BASE-005 | `Spec.Brace.DeflectOffersOnlyLegalSides` |
| CHAR-BASE-006 | già `Spec.Overwatch.HoldThenFire` |
| CHAR-BASE-007 | `Spec.Overwatch.FrontlineFollowsFacing` |
| CHAR-BASE-008 | **sospeso**: `BAS-5` |
| CHAR-BASE-009 | è `Reactions.Brace.BaseProfileHasSingleResponse`, già previsto da CP 14.7 |
| CHAR-BASE-010 | già coperto, §2.10 |
| milestone `F1`–`F5` | `E14` (F1/F2), `E8`+`E9` (F3), `E15` (F4), `M10` (F5) |

## 6. Che cosa è stato scritto nel repository

Il triage non ha cambiato nessuna regola, nessun numero e nessuno stato: ha registrato **domande** e
**scenari dichiarati**, che è il massimo che un sorgente possa produrre da solo.

| File | Modifica |
|---|---|
| `docs/roadmap/feature-registry.yaml` | i sette scenari nuovi entrano come `planned` sotto le due feature reali; le note dichiarano che il contenuto dei profili è aperto |
| `docs/technical/scenario-map.md` | i sette scenari entrano in **classe D** (dichiarati, non ancora scritti) |
| `docs/roadmap/roadmap-v0.1.md` | CP 14.4 e CP 14.7 rimandano qui per il contenuto dei profili |
| `docs/OPEN_DECISIONS.md` | le cinque `BAS-*` |
| `docs/archive/src/handoff/` | il sorgente, archiviato come consumato |

**Non** è stato toccato: il Decision Log (nessuna nuova `D-nnn`: gli ID si assegnano al merge, e nessuna
di queste domande è decisa), i cataloghi di bilanciamento, il codice.
