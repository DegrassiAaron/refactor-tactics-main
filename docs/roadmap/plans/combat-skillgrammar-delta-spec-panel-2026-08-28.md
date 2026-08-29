# Combat Effect Model + Skill Card Grammar Delta — spec panel

> **Referto di revisione**, non owner. Consuma
> [`../../archive/src/handoff/2026-08-28-combat-skillgrammar-delta.md`](../../archive/src/handoff/2026-08-28-combat-skillgrammar-delta.md)
> (in radice come `CLAUDE_RT_Combat_SkillGrammar_Consolidation_Delta_2026-08-28.md` fino a questo commit).
>
> **Data**: 2026-08-28 · **Base**: `main` @ `483e031a` · **Modo**: critique · **Focus**: requirements + architecture
>
> Il kit dichiara di essere un **delta** su due domini distinti (§1) e chiede al §19 dieci output. Questo
> referto li produce. Le decisioni che ne escono sono **D-237** (card) e **D-238** (combat).

---

## 1. Il verdetto in una riga

> **Il kit è disciplinato dove distingue e ridondante dove afferma: undici delle sue invarianti di combat
> erano già scritte nell'owner esistente, tre dei suoi sei gruppi visivi erano già decisi da D-231 con lo
> stesso significato — ma i due contributi che restano sono reali, e uno di essi è un conflitto che nessuna
> delle due fonti poteva vedere.**

Il contributo che vale il consumo: **`Armor` e `BaseShield` occupano lo stesso asse.** Il PRD è stato
verificato il **2026-08-27**; `D-224` ha spedito `BaseShield = 5` il **2026-08-28**. Nessuno dei due documenti
sa dell'altro, e la loro somma raddoppia in silenzio la mitigazione del danno `Direct`.

---

## 2. Ciò che è stato misurato

Ogni riga è un comando eseguito su `main` @ `483e031a`, non una lettura.

| Domanda | Comando | Esito |
|---|---|---|
| Il kit è già in lavorazione altrove? | `git ls-tree` su **11** ref (10 remoti + 1 locale) | ❌ **No.** L'unico match è `RT_ClaudeDesign_Prompt_SkillGrammar_v0.1.md`, un sorgente già archiviato |
| L'handoff gemello citato al §0.4 è stato consumato? | `ls docs/archive/src/handoff/ \| grep 2026-08-28` | ✅ **Sì**, come `2026-08-28-icon-grammar-consolidation.md`, e ha prodotto `D-231` `D-232` `D-233` |
| Esiste un owner della Skill Card Grammar? | `ls docs/technical/systems/` | ✅ [`spec-icon-card-grammar.md`](../../technical/systems/spec-icon-card-grammar.md), **286** righe, nato ieri dal gemello |
| Esiste un owner del Damage Model? | `git grep -ril "DamagePacket\|DamageResistance" -- docs` | ✅ [`prd-damage-model-armor-shield.md`](../../research/prd/prd-damage-model-armor-shield.md), livello **8**, non normativo |
| Ultimo `D-nnn` assegnato | Decision Log + scansione dei **11** ref | **D-233** ovunque, e `D-234`/`D-235` risultavano **liberi** — misura corretta alla propria base. 🔴 **Persi nella finestra**: `main` ha assegnato `D-232`→`D-236` prima che questo branch aprisse la sua PR, e le due voci sono state **rinumerate a `D-237`/`D-238`** al merge (2026-08-29), su 24 occorrenze in 5 file. È la finestra che `CLAUDE.md` §7 descrive: *«fra il commit che prende un ID e l'apertura della sua PR c'è una finestra in cui l'ID è preso e invisibile»* — diciottesima collisione del progetto |
| `ERTDamageSource` esiste? | `git grep -n ERTDamageSource -- Source` | ✅ `RTCombatLibrary.h:33`, binario `Direct`/`Environmental` |
| `Armor` esiste? | `git grep -c Armor -- Source` | ⛔ **0 file** |
| `DamageType`, `DamageResistance`, `DamagePacket` | idem, tre termini | ⛔ **0 file** ciascuno |
| `ControlResistance`, `StatusResistance` | idem | ⛔ **0 file** ciascuno |
| `Shred` | idem | ⛔ **0 file** |
| `KineticResistance`, `FireResistance`, `ElectricResistance` | idem, tre termini | ⛔ **0 file** ciascuno |
| L'unica resistenza che esiste | `git grep -n Resistance -- 'Source/**/*.h'` | `PushResistance` (`RTUnit.h:104`), **dormiente per D-075, senza produttore** (`RTTurnManager.h:1197`) |
| I numeri di danno reali | `RTCombatLibrary.h` | `BaseShield 5` · `BurningCleanupDamage 8` · `PropagatedElectricDamage 12` · `GuardFirstHitReduction 15` · `Action.Counter` 16 · `Action.Electrify` 20 |

⛔ **Dieci termini su cui poggiano il settore COUNTER (§10.5) e la formula (§4) hanno zero riscontri nel
codice.** Non è un difetto del kit — è la sua condizione di partenza, e il kit non la dichiara mai.

---

## 3. Il panel

### 📚 WIEGERS — qualità del requisito

> «Undici delle vostre invarianti non sono requisiti nuovi: sono una seconda copia.»

La tabella *«Decisioni congelate nel report»* del PRD contiene **già**, verbatim nella sostanza: Armor signed ·
Armor solo `Direct` · DamageResistance signed per `DamageType` · un solo `DamageType` per packet · mixed
damage = più packet · zero dopo active defense non riattivabile · Piercing ignora solo Armor positiva · Shred
può portarla negativa · eventi `Hit`/`ShieldDamage`/`HealthDamage` distinti · Affinity/Weakness ≠ Resistance ·
Shield non cancella status/control. Il PRD dedica perfino una sezione a *«DamageResistance contro
ControlResistance»* (`#440`, E36).

**Il §3 dell'intero kit è ri-affermazione.** Non è dannoso, ma non va registrato come decisione nuova: una
regola scritta due volte è una regola che può divergere.

E il §2.1 — *«vietare `Shape == Single ⇒ Attack ⇒ Hit ⇒ Damage`»* — è **già chiuso da `D-221`**, che ha
messo il cancello in un punto solo (`bCountsAsAttack` su `FRTActionDef`, `false` di default). Riscriverlo
come principio, senza citare il flag che lo implementa, riapre in prosa ciò che il codice ha già chiuso.

**Resta requisito nuovo e verificabile solo questo**: la formula additiva (§4), Vulnerability come famiglia
(§5), gli elementi come verbi sistemici (§7), e i due trigger `OnStatusApplied`/`OnDisplaced` oltre ai tre già
congelati.

### 🔨 ADZIC — falsificabilità

> «La vostra formula non ha ingressi. Non posso scrivere un esempio che la faccia fallire.»

`ApplicableArmor` legge `Armor`: **0 occorrenze**. `DamageResistance[DamageType]` legge due termini che
insieme fanno **0 occorrenze**. Un `Given/When/Then` su questa formula non può oggi essere eseguito da nessun
test, e nessun gate lo direbbe.

✅ **La formula è però internamente coerente, e lo verifico prima di criticarla:**

| Caso | `ApplicableArmor` | `Defense` | `FinalDamage` su `Base 10` | Corretto? |
|---|---:|---:|---:|---|
| `Direct`, Armor +3, Res 0 | 3 | 3 | 7 | ✅ mitiga |
| `Direct`, Armor −4, Res 0 | −4 | −4 | 14 | ✅ vulnerabilità |
| `Environmental`, Armor +3 | 0 | 0 | 10 | ✅ ignora Armor |
| `Direct` + Piercing, Armor +5 | `min(5,0)` = 0 | 0 | 10 | ✅ ignora solo la positiva |
| `Direct` + Piercing, Armor −4 | `min(−4,0)` = −4 | −4 | 14 | ✅ **non** la riporta a zero |
| `Direct`, Armor +3, FireRes +9 | 3 | 12 | 0 | ⚠️ **vedi Nygard** |

Le prime cinque righe sono esatte e la §3.6 in prosa concorda con `min(Armor, 0)`. È la sesta che apre un
buco.

### 🎲 NYGARD — cosa si rompe

> «Avete due strade che portano a zero e ne dichiarate terminale una sola.»

Il §3.8 dichiara terminale lo zero prodotto da **Active Defense**, e il §4 lo ribadisce: *«valutato PRIMA di
questa formula e non riaperto»*. Ma `FinalDamage = max(0, Base − Defense)` produce **un secondo zero**, per
saturazione della difesa, e di quello nessuno dice se sia terminale. Il giorno in cui esisterà un *«minimo 1
danno»*, un *«on damage dealt»* o un DoT che rilegge il packet, i due zeri si comporteranno diversamente e
nessuna regola dirà quale sia quale. **Il tipo di ritorno deve distinguerli, o la distinzione vive solo nella
prosa.**

🔴 **E c'è il conflitto vero, che né il kit né il PRD potevano vedere.**

`D-224` ha spedito `BaseShield = 5` il 2026-08-28. La sua condizione di applicabilità è, testualmente,
*«ferma solo il danno `Direct`»* — **la stessa** che il kit assegna ad `Armor > 0`: *«mitigazione degli
impatti `Direct`»*. Il PRD è stato verificato il **2026-08-27**: un giorno prima. Due meccaniche, un asse.

| | `Armor` (proposta) | `BaseShield = 5` (spedito, D-224) |
|---|---|---|
| Si applica a | solo `Direct` | solo `Direct` |
| Effetto | **riduce** il danno del colpo | **assorbe** dal pool |
| Si consuma? | no, vale a ogni colpo | sì, e si ricarica nel Cleanup |
| Aggirabile da | Piercing | niente, oggi |

Su un'unità con `Armor 3`, due colpi `Direct` da 10 nello stesso turno fanno `7 → 5 assorbiti → 2 HP`, poi
`7 → 0 scudo → 7 HP`: **9 HP invece di 15**. Introdurre `Armor` senza dichiarare il rapporto con `BaseShield`
è una modifica di bilanciamento del **40%** presa per omissione. E `D-224` ha già misurato quanto costi
sbagliare quest'asse: *«a 5 punti indistinti un contrattacco da 10 perderebbe metà del suo peso»*.

### 🏗️ FOWLER — confini

> «Tre dei vostri sei gruppi esistono già, con lo stesso significato e un altro nome.»

Confronto letterale fra il §10 del kit e la tabella §5 dell'owner ([D-231](../../decisions/RT_PDR_00_Decision_Log.md)):

| Gruppo del kit | Forma in D-231 | Definizione già spedita | Delta |
|---|---|---|---|
| ♦ **SKILL MODIFIER** | Rombo | *«modificatore applicato alla skill»* | **nessuno**, solo il nome |
| ■ **CONTEXT MODIFIER** | Quadrato | *«modificatore esterno su unità / cella / contesto»* | **nessuno**, solo il nome |
| ▲ **CONDITION / TRIGGER** | Triangolo | *«condizione, trigger, requisito, warning»* | **nessuno**, solo il nome |
| **APPLICATION** | 3 piccoli esagoni | *«proprietà intrinseca: Target, elemento, Shape, Delivery»* | **raggruppamento** di 4 assi su 5 |
| **EFFECT** | 3 piccoli esagoni | idem (l'elemento vive lì) | **scorporo** dell'output dall'intrinseco |
| **COUNTER** | — | ⛔ **non esiste** | **gruppo nuovo** |

Il kit presenta come nuova un'architettura che per metà è già spedita. Il delta reale è di **due** movimenti
(scorporare EFFECT dai piccoli esagoni; nominare APPLICATION il resto) più **uno** nuovo, COUNTER — che è
anche l'unico da rifiutare, e per la ragione di Adzic: mostrerebbe `Armor · KineticResistance ·
FireResistance · ElectricResistance · ControlResistance · StatusResistance`, cioè **sei termini a zero
occorrenze**.

✅ **Ma il kit corregge un difetto reale dell'owner, e va riconosciuto.** La tabella §5 di `D-231` assegna
`alto-destra` **sia** ai piccoli esagoni **sia** al triangolo: due categorie sullo stesso ancoraggio. Il §16-D
del kit — *«non usare dinamicamente lo stesso vertice per categorie diverse, perché la posizione deve restare
imparabile»* — elimina la collisione. **È il contributo più solido del delta**, e non è nella lista dei suoi
titoli.

### 🧭 COCKBURN — chi è l'attore

> «Chi legge la card in partita non ha bisogno del vostro sesto settore. Chi la autora sì, ma di un'altra cosa.»

Il settore COUNTER risponde alla domanda *«cosa può ridurre questa skill»*. In partita, chi la legge sta
scegliendo **fra due azioni proprie**, non facendo l'analisi difensiva dell'avversario: e il §10.5 lo ammette
da sé — *«non mostrare ogni possibile passaggio della pipeline»*. Un settore permanente per un'informazione
che quasi sempre è *«Armor + la resistenza del tipo»* è ridondanza posizionale, non spiegabilità.

L'informazione difensiva ha già un attore e un posto: chi ispeziona un bersaglio, nel pannello dell'unità,
non nel ring di una skill.

### 🧪 CRISPIN — come si valida

> «Quattro dei vostri cinque esempi obbligatori usano numeri che non esistono nel gioco.»

| Kit | Owner (`spec-icon-card-grammar.md` §6) | Ordine di grandezza reale |
|---|---|---|
| Rail Shot `Damage 30 Kinetic` | Rail Shot `Damage 5 · Pierce · Range 7` | `Counter` 16 · `Electrify` 20 · `Burning` 8 |
| Chain Shock `Damage 24 Electric`, `Chain 2` | Chain Lightning `Electric Damage 3`, `Max Targets 3` | `PropagatedElectricDamage` 12 |

Un `Damage 30` è **fuori scala di un fattore sei** rispetto al più piccolo danno reale e **del 50%** oltre il
più grande. Un esempio didattico che insegna una scala inesistente produce card che sembrano giuste e
sbagliano il colpo d'occhio — ed è esattamente ciò che gli esempi dovrebbero prevenire. **Prevale l'owner**,
per la regola di precedenza che il kit stesso scrive al §0.

✅ **`14.3 Water Burst` è invece l'esempio migliore del kit** e va tenuto: `Cell · Circle R2 · Wet · Push 1 ·
No Direct Damage` falsifica tre cose insieme (`Effect ≠ Damage`, `Skill ≠ Attack`, elemento senza danno). È un
regression test percettivo, non un'illustrazione. Stessa cosa per `14.4 Armor Breaker`, che rende visibile
`Shred ≠ Piercing`.

⚠️ **E i tetti non tornano.** L'owner conta **9** satelliti (1 marker + 1 cerchio + 3 esagoni + 2 quadrati +
1 rombo + 1 triangolo); il §13 del kit fissa *«ring: target max 8 glifi leggibili»*. Aggiungere COUNTER porta
a **10-11**. Due fonti, due tetti, nessuna che citi l'altra: il numero va scelto una volta, non ereditato.

---

## 4. Sintesi — cosa consolidare, cosa no

| # | Tesi del kit | Verdetto | Dove atterra |
|---|---|---|---|
| 1 | Sei gruppi semantici sui vertici | ✅ **come vista di lettura**, non come seconda grammatica | **D-237** + owner §5.1 |
| 2 | ♦ / ■ / ▲ distinti | ✅ **già deciso da D-231**, si registra l'equivalenza | owner §5.1 |
| 3 | Posizione fissa e imparabile per gruppo | ✅ **accolta, corregge un difetto dell'owner** | **D-237** |
| 4 | Settore COUNTER | ⛔ **rifiutato ora**: sei termini a zero occorrenze | **D-237** |
| 5 | Raggruppare in UI ≠ fondere nel data model | ✅ **accolta e resa vincolo** | **D-237** |
| 6 | Esempi con `Damage 30` / `24` | ⛔ **respinti**: fuori scala. Prevale l'owner | owner §6 |
| 7 | `Water Burst` e `Armor Breaker` come regression | ✅ **accolti** | owner §6 |
| 8 | Invarianti §3 (undici regole) | ⚠️ **già scritte nel PRD**: nessuna decisione nuova | PRD, nota |
| 9 | `Shape ⇏ Attack ⇏ Hit` | ⚠️ **già chiuso da D-221** | PRD, nota |
| 10 | Formula additiva §4 | ⛔ **non si congela**: nessun ingresso esiste | **D-238** |
| 11 | Conflitto `Armor` / `BaseShield` | 🔴 **registrato come blocco** al §4 | **D-238** + PRD |
| 12 | Due zeri non distinti | 🔴 **registrato** | **D-238** + PRD |
| 13 | Vulnerability = famiglia, non stat | ✅ **accolta**, è nuova | **D-238** + PRD |
| 14 | Elementi come verbi sistemici | ✅ **accolta**, è nuova | PRD |
| 15 | `OnStatusApplied` / `OnDisplaced` | ✅ **accolti** come estensione dei tre congelati | PRD |
| 16 | Non creare epic nuove | ✅ **rispettato**: nessuna issue creata | — |

---

## 5. Open point rimasti

| # | Domanda | Chi la chiude |
|---|---|---|
| **A** | Rapporto fra `Armor` e `BaseShield`: assi separati, o `Armor` sostituisce lo scudo base? | 🔴 **blocca** l'introduzione di `Armor`. Decisione di bilanciamento, non di modello |
| **B** | Policy `Armor` su Hit multi-packet (kit §16-B, PRD *«per packet logico»*) | resta aperta in **entrambe** le fonti |
| **C** | I due zeri (Active Defense vs saturazione) sono distinguibili nel tipo di ritorno? | va deciso **prima** del resolver, non dopo |
| **D** | `MinArmor` data-driven | già aperto nel PRD, invariato |
| **E** | Densità del settore COUNTER (1 o max 2) | ⛔ **decaduta**: senza il settore, la domanda non si pone |
| **F** | Il sesto vertice resta riservato e vuoto? | **D-237** dice sì: riservato, non riassegnato |
| **G** | Tetto del ring: 8 (kit) o 9 (owner) | **D-237** sceglie **9**, il numero dell'owner spedito |

---

## 6. Il difetto strutturale, e come non ripagarlo

Il kit apre con una regola operativa esemplare — *«questo file NON crea un secondo sistema»*, cinque letture
obbligatorie prima di toccare il repository, una gerarchia di precedenza esplicita. Poi scrive **undici
invarianti che erano già scritte** e **tre gruppi visivi che erano già decisi il giorno prima**.

La causa non è disattenzione: è che il suo §0.2 dice *«leggere il documento owner corrente della Skill Card
Grammar»* e quel documento **è nato lo stesso giorno**, dal suo gemello. Il kit è stato scritto contro uno
stato del repository che è cambiato mentre lo si scriveva.

**La lezione operativa non è «leggere di più».** È che un kit di consolidamento va **datato contro un HEAD**,
non contro un giorno: `2026-08-28` copre `483e031a` e i quattro commit che lo precedono, e fra il primo e
l'ultimo sono atterrate `D-231`, `D-232`, `D-233` e `D-224`. Quattro delle decisioni che questo kit avrebbe
dovuto leggere non esistevano quando è stato scritto.

⚠️ **E il difetto si ripaga subito se questo referto viene letto domani**: le sue misure valgono su
`483e031a`. I dieci `git grep` a zero sono la parte che scade per prima — bastano una fetta di `E49` o una
issue di `Armor` per invalidarli. **Rieseguirli, non citarli.**

---

## 7. Puntatori

| Cosa | Dove |
|---|---|
| Il kit consumato | [`../../archive/src/handoff/2026-08-28-combat-skillgrammar-delta.md`](../../archive/src/handoff/2026-08-28-combat-skillgrammar-delta.md) |
| Il gemello, consumato ieri | [`../../archive/src/handoff/2026-08-28-icon-grammar-consolidation.md`](../../archive/src/handoff/2026-08-28-icon-grammar-consolidation.md) · [referto](icon-card-grammar-spec-panel-2026-08-28.md) |
| Owner Skill Card Grammar | [`../../technical/systems/spec-icon-card-grammar.md`](../../technical/systems/spec-icon-card-grammar.md) — **D-231** |
| Owner Damage Model (livello 8) | [`../../research/prd/prd-damage-model-armor-shield.md`](../../research/prd/prd-damage-model-armor-shield.md) |
| Scudo base e sorgente del danno | **D-224** · `RTCombatLibrary.h:99` |
| Un colpo è un concetto solo | **D-221** · `bCountsAsAttack` |
| Decisioni prodotte da questo referto | **D-237** · **D-238** |
