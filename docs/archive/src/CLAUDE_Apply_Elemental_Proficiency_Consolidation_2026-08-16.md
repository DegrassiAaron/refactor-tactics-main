# CLAUDE — Applicazione della Elemental Proficiency ai personaggi v0.1

> 🗄️ **ARCHIVIATO il 2026-08-16 — consumato.** Questo è un **sorgente**, non un owner: si legge per la
> provenienza, mai per la regola. È il gemello *operativo* di
> [`RefactorTactics_v0.1_Characters_Elemental_Consolidation.md`](RefactorTactics_v0.1_Characters_Elemental_Consolidation.md)
> — stessa baseline, stessa grammatica, più un piano d'esecuzione — ed entrambi sono stati letti e filtrati
> nello stesso passaggio. Il filtro completo sta nel banner del gemello: qui restano le sole cose che
> riguardano **questo** documento.
>
> **Cosa è entrato** — il contenuto di design, che è identico al gemello, vive in
> [**#995**](https://github.com/DegrassiAaron/refactor-tactics-main/issues/995). Del piano d'esecuzione sono
> entrati il preflight (§1), la separazione `AS-BUILT / APPROVED TARGET / IMPLEMENTATION GAP` per Phase
> (§6), la regola che il gap runtime va tracciato in una issue **separata** (§12), e il vincolo che
> `paragon.md` non ospiti la matrice (§9).
>
> **Cosa NON è entrato oltre a quanto già scritto nel gemello:**
>
> · §12 **la ricerca anti-duplicato non trova nulla**. Il documento la prescrive con otto termini e dichiara
> chiuse #56 e #336 — verificato, entrambe `CLOSED`, e su questo aveva ragione. Ma `gh search issues` su
> `elemental` e `proficiency` nel repository dà **zero** in ogni stato: nessuna issue aperta possiede questa
> decisione, quindi il ramo «se esiste una issue aperta più recente, aggiornala» è vuoto e #995 è la prima.
>
> · §11 e §15 **il tooling citato esiste, e ne manca un pezzo che il documento non nomina**.
> `feature_registry.py deploy --wiki-root` è un sottocomando reale — verificato con `--help`. Ma
> `feature_registry.py wiki` scrive **blocchi generati dentro `docs/characters/`**, e il documento non lo
> dice mai pur prescrivendo di editare proprio quelle pagine: chi le tocca non modifica quei blocchi a mano
> e rigenera dopo. È l'unica prescrizione che, applicata alla lettera, avrebbe prodotto un danno silenzioso.
>
> · §15 **il gate di naming ha una maschera che cambia il modo di scrivere le sezioni nuove**.
> `check-docs-naming.py` esenta i token fra backtick (`INLINE_CODE`) e i path nudi (`BARE_PATH`), ma
> `docs/characters/` **non** è fra le cartelle esenti: i nomi legacy in prosa lo farebbero fallire. Le
> sezioni nuove si scrivono con Gadget/Phase/Riktor/Wraith in prosa e i token fra backtick.
>
> · §14 e §16 **non eseguiti, e non per dimenticanza**: nessun `D-nnn` è stato scelto (il documento stesso lo
> vieta, e `scripts/rt_shared_id.py reserve D` è la via), e i commit `docs(characters)` / `docs(wiki)`
> appartengono alla track che eseguirà #995, non a questa archiviazione.
>
> · §3 **la domanda terminologica resta aperta, con un dato in più**. Il documento chiede di scegliere fra
> `Access` e `Affine` e ordina di non rinominare automaticamente. Il consumo aggiunge ciò che non sapeva:
> nel repository il termine tecnico è già **`Affinity`**, serializzato. `Affine` creerebbe due parole quasi
> identiche per due assi diversi. Raccomandazione scritta in #995: tenere `Access` per il grado. Da
> confermare in integrazione.
>
> ⚠️ **La «Regola finale» del documento — *«non fermarti a proporre: applica le modifiche consentite dal
> write-set»* — è stata rispettata alla lettera, ed è per questo che le pagine personaggio NON sono state
> toccate.** La sessione che ha consumato questi kit possiede `Turn/RTMatchStateHash.*` e `Map/RTHexMapAsset.*`
> per [#986](https://github.com/DegrassiAaron/refactor-tactics-main/issues/986); `docs/characters/**` non è
> nel suo `writable`, e il documento stesso prescrive di non aggirare D-139 ma di dichiarare il delta. Il
> delta è #995.


**Data:** 2026-08-16  
**Progetto:** RefactorTactics  
**Repository:** `DegrassiAaron/refactor-tactics-main`  
**Modalità:** esecuzione + consolidamento, non brainstorming  
**Fonte primaria di questa attività:** `RefactorTactics_v0.1_Characters_Elemental_Consolidation.md`

## Obiettivo

Applicare nel repository reale e nella Wiki il contratto approvato di **proficiency/affinità elementale dei personaggi**, usando il file di consolidamento come sorgente di design e verificando sempre lo stato corrente di `main` prima di modificare.

Il lavoro deve:

1. consolidare la grammatica **Master / Specialist / Access / None**;
2. allineare le pagine dei quattro personaggi v0.1;
3. creare o aggiornare un owner documentale unico della proficiency elementale;
4. allineare la Wiki reale;
5. aggiornare il tracking GitHub senza duplicare issue;
6. registrare chiaramente eventuali gap fra **AS-BUILT** e **APPROVED TARGET**;
7. non fingere che il runtime sia già allineato quando non lo è.

---

# 1. Preflight obbligatorio

Prima di scrivere:

- apri `CLAUDE.md`;
- verifica branch e HEAD di `main`;
- `git pull`/fetch secondo il workflow del repository;
- controlla working tree e sessioni parallele;
- leggi `docs/roadmap/parallel-batch.yaml`;
- rispetta **D-139**: nessun path viene modificato finché non appartiene al `writable` della track corretta;
- rispetta **D-076**: le pagine Wiki reali NON vivono in `docs/wiki/`; serve il clone `refactor-tactics-main.wiki`;
- non modificare PDF snapshot come fonte primaria;
- non cambiare Stable ID o nomi tecnici per questo lavoro.

Se il repository corrente contraddice il file di consolidamento, NON risolvere silenziosamente: classifica il conflitto e riportalo.

---

# 2. Contratto di proficiency da consolidare

## Gradi

| Grado | Contratto |
|---|---|
| **Master** | almeno 3 capability elementali significative dello stesso elemento e almeno 2 funzioni differenti |
| **Specialist** | 2 capability elementali significative dello stesso elemento |
| **Access** | 1 capability elementale significativa dello stesso elemento |
| **None** | nessuna capability elementale nativa |

Una capability conta solo se il profilo/kit realmente:

- **Generate**
- **Apply**
- **Propagate**
- **Transform**
- **Consume**

un elemento, stato o superficie del sistema.

NON bastano da soli:

- `DamageType`;
- Gameplay Tag tematico;
- VFX;
- nome dell'abilità;
- estetica dello slot Paragon.

## Origine dell'accesso

| Origine | Conta nella proficiency? |
|---|---:|
| **Innate** | sì |
| **Signature/Profile Equipment** | sì |
| **Generic Equipment** | no, è `External Access` |

La proficiency può dipendere dal profilo/loadout. La copertura elementale del roster va invece misurata per personaggi distinti.

---

# 3. Nota terminologica: “Access” vs “Affine”

La sorgente approvata attuale usa **Access**.

L'autore ricorda il terzo livello come **Affine**.

NON rinominare automaticamente `Access` in `Affine`.

Procedura:

1. cerca nel Decision Log, owner docs e Wiki se esiste già una decisione successiva;
2. se non esiste, mantieni **Access** come termine canonico per questa applicazione;
3. segnala nel report finale la possibile decisione terminologica:
   - `Access` = nome tecnico corrente;
   - `Affine` = candidato player-facing/design-facing;
4. se durante il lavoro l'autore ha già fissato `Affine` in una fonte più recente, applica quella decisione in modo globale e coerente.

Nessuna miscela casuale di `Access`, `Affinity`, `Affine` in owner concorrenti.

---

# 4. Baseline v0.1 approvata

| Personaggio player-facing | Stable gameplay ID | Elemento | Grado |
|---|---|---|---|
| **Gadget** | `Hero.Flux` | Electric | **Specialist** |
| **Phase** | `Hero.Riva` | Water | **Access** |
| **Riktor** | `Hero.Bastion` | — | **None / non elementare** |
| **Wraith** | `Hero.Vektor` | — | **None / non elementare** |

Vincoli:

- i nomi visibili restano Gadget, Phase, Riktor, Wraith;
- gli ID gameplay restano `Hero.Flux`, `Hero.Riva`, `Hero.Bastion`, `Hero.Vektor`;
- il mapping Paragon è asset/presentation, NON decide l'elemento del personaggio.

---

# 5. Gadget — Electric Specialist

Owner tecnico attuale: `Hero.Flux`.

Le capability che contano per la proficiency sono:

### `Flux.LinearDischarge`

- ruolo elementale: **Apply / Exploit Electric**;
- payoff su `Wet`;
- conta per la proficiency.

### `Flux.ConductiveNode`

- coincide con `Action.Electrify` secondo le decisioni esistenti;
- ruolo elementale: **Propagate Electric** sul grafo conduttivo;
- legge acqua/superfici conductive;
- conta per la proficiency.

NON far salire Gadget a Master solo perché altre skill sono elettriche nel tema.

In particolare non contano automaticamente come terza capability sistemica:

- `Flux.ArcPulse`;
- `Flux.Overload`;
- `Flux.ReactiveCapacitor`.

Risultato da dichiarare:

> **Gadget = Electric Specialist, non Master.**

---

# 6. Phase — Water Access via Signature/Profile Gadget

Owner tecnico attuale: `Hero.Riva`.

Questa è la parte con il gap più importante.

Il target approvato è:

> **Phase = Water Access**, non Water Specialist/Master.

L'unica capability Water che deve contare nel profilo baseline target è:

### `Riva.PressureJet`

- sorgente: **Signature/Profile Gadget**;
- funzione: **Apply Water/Wet**;
- grammatica utile: `Wet + Push`;
- conta come unica capability Water del profilo baseline.

L'identità principale di Phase deve restare centrata su:

- supporto;
- link;
- reposition;
- mobility;
- reaction;
- utility.

Non trasformarla in una “water mage”.

## Migrazione documentale obbligatoria

Se il runtime/catalogo corrente contiene ancora più capability Water sistemiche, NON riscrivere la storia.

La pagina deve distinguere:

### AS-BUILT v0.1

Ciò che catalogo/codice fanno davvero oggi.

### APPROVED TARGET PROFILE

Water Access via Signature/Profile Gadget, con `PressureJet` come unica capability Water che conta.

### IMPLEMENTATION GAP

Elenco puntuale delle skill/dati runtime che impediscono ancora al target di essere vero.

Questo gap deve essere tracciato da issue separata se richiede codice/kit.

---

# 7. Riktor — non elementare

Owner tecnico attuale: `Hero.Bastion`.

Identità da preservare:

- Guardian / Controller;
- Field Architecture;
- Ally Interposition;
- structures / cover / topology;
- displacement;
- Kinetic.

Non assegnare `Electric Access` solo perché il vecchio asset/personaggio Paragon suggerisce quella lettura.

Risultato:

> **Riktor = non elementare nella baseline v0.1.**

---

# 8. Wraith — non elementare

Owner tecnico attuale: `Hero.Vektor`.

Identità da preservare:

- Predictive Duelist;
- Predictive Interception;
- Movement Punish;
- movement / reaction / prediction / facing;
- Kinetic.

Non assegnare `Cold Access` solo per completare una matrice elementale.

Risultato:

> **Wraith = non elementare nella baseline v0.1.**

---

# 9. Owner documentale

Prima cerca un owner equivalente.

Se non esiste, crea:

`docs/characters/elemental-proficiency.md`

Deve possedere SOLO il contratto trasversale:

- Master / Specialist / Access / None;
- cosa conta come capability;
- Innate vs Signature/Profile Equipment vs Generic Equipment;
- External Access;
- proficiency del profilo vs coverage del roster;
- tabella canonica personaggi;
- link alle singole pagine eroe.

NON duplicare qui kit, danni e numeri di bilanciamento.

`docs/characters/paragon.md` resta owner del mapping visuale/asset e deve al massimo linkare l'owner elementale.

---

# 10. Pagine personaggio da aggiornare

Verifica i path reali su `main` prima di modificare.

Target atteso:

- `docs/characters/v0.1/flux.md`
- `docs/characters/v0.1/riva.md`
- `docs/characters/v0.1/bastion.md`
- `docs/characters/v0.1/vektor.md`

## Gadget / Flux

Aggiungi o consolida `## Proficiency elementale`:

- Elemento: Electric;
- Grado: Specialist;
- capability contate: `LinearDischarge`, `ConductiveNode`;
- tag/danno/VFX non aumentano automaticamente il grado.

## Phase / Riva

Aggiungi o consolida `## Proficiency elementale`:

- Elemento: Water;
- Grado target: Access;
- sorgente: Signature/Profile Gadget;
- capability target contata: `PressureJet`;
- separazione `AS-BUILT / APPROVED TARGET / IMPLEMENTATION GAP`.

Rivedi descrizioni come `Water Terrain Manipulator`, `Water Shaping`, `Wet Setup`, risorse e skill solo quanto serve a evitare che vengano lette come target finale se oggi sono as-built divergente.

## Riktor / Bastion

Sezione breve:

- Elemento: None;
- stato: non elementare;
- focus: Field Architecture + Kinetic/Structures;
- nessun Electric baseline.

## Wraith / Vektor

Sezione breve:

- Elemento: None;
- stato: non elementare;
- focus: Prediction/Reaction/Movement;
- nessun Cold baseline.

---

# 11. Wiki reale

NON modificare pagine narrative dentro `docs/wiki/`.

Lavora sul clone:

`refactor-tactics-main.wiki`

Procedura:

1. trova i nomi/path reali delle pagine Gadget, Phase, Riktor, Wraith;
2. trova un owner già esistente per elementi/affinità/proficiency;
3. se manca, crea UNA pagina centrale;
4. le quattro pagine personaggio linkano l'owner e riportano il proprio grado;
5. evita matrici duplicate;
6. usa esempi comprensibili a designer/playtester, non solo descrizioni da programmatore;
7. non presentare il target di Phase come runtime già implementato;
8. dopo le modifiche del corpo pagina, aggiorna gli eventuali blocchi status con il tooling ufficiale:

```bash
python scripts/feature_registry.py deploy --wiki-root <clone> --write
```

Il deploy del registry NON sostituisce le modifiche editoriali del corpo Wiki.

---

# 12. GitHub issue / tracking

Prima cerca issue esistenti con termini:

- `elemental proficiency`
- `element affinity`
- `Water Access`
- `Phase`
- `Riva`
- `PressureJet`
- `characters roster`
- `elemental`

NON creare duplicati.

## Stato verificato prima di questo handoff

Due issue storiche rilevanti risultano già chiuse:

- **#56 — `CP 6.3 — Riva, manipolatrice dell'acqua`**: chiusa/completed; descrive il vecchio kit Water completo.
- **#336 — `[DOCS] Consolidamento cluster Characters & Roster`**: chiusa/completed; non è un owner aperto per questo nuovo target.

Quindi:

- NON riaprire automaticamente #56 o #336;
- NON riscrivere il corpo di una issue chiusa come se avesse sempre contenuto la nuova decisione;
- se esiste una issue aperta più recente che possiede questa decisione, aggiornala;
- se NON esiste, crea una issue piccola e focalizzata per il consolidamento documentale/design.

Titolo suggerito:

`[DOCS/DESIGN] Consolidare Elemental Proficiency v0.1`

DoD suggerito:

- [ ] owner `elemental-proficiency` individuato o creato
- [ ] Gadget = Electric Specialist
- [ ] Phase = Water Access via Signature/Profile Gadget
- [ ] Riktor = None
- [ ] Wraith = None
- [ ] pagine personaggio allineate
- [ ] Wiki reale allineata
- [ ] nessuna matrice duplicata
- [ ] link/validator documentali verdi
- [ ] AS-BUILT di Phase non falsificato
- [ ] eventuale gap runtime tracciato separatamente

## Issue separata per Phase runtime

Se il codice/catalogo corrente rende ancora Phase sistemicamente Water oltre `PressureJet`, crea o aggiorna una issue distinta.

Titolo suggerito:

`[DESIGN/ABILITY] Allineare Phase al profilo Water Access`

Scope:

- identificare quali capability Water correnti sono realmente sistemiche;
- decidere quali restano solo tema/VFX/technology flavor;
- mantenere `PressureJet` come unica capability Water che conta nel baseline target;
- aggiornare catalogo/codice/test senza introdurre branch speciali per eroe;
- non rompere combo `Wet` sistemiche già possedute dal sistema.

NON mischiare questo runtime migration task con la sola documentazione.

---

# 13. Feature Registry / roadmap

Cerca prima le feature esistenti.

Non creare un nuovo `feature_id` se la semantica è già coperta da:

- character roster;
- elemental grammar;
- elemental affinity/access;
- character data;
- documentation/Wiki alignment.

Se esiste una feature `Element Affinity / Access` o equivalente:

- collega owner;
- collega issue;
- aggiorna `wiki_note`/refs dove previsto;
- rigenera gli artefatti derivati con gli script ufficiali.

Se non esiste, valuta se serve davvero una feature runtime oppure se basta tracking documentale. Non gonfiare la roadmap per una tassonomia.

---

# 14. Decision Log

Il file di consolidamento segnala il Decision Log come `integration_only` nel batch corrente.

Quindi:

- non toccarlo da una track non autorizzata;
- prepara la decisione per integration se manca;
- la decisione deve fissare almeno:
  - grammatica dei gradi;
  - cosa conta come capability;
  - origine dell'accesso;
  - baseline Gadget/Phase/Riktor/Wraith;
  - eventuale scelta terminologica Access/Affine.

Non inventare un numero `D-xxx`: usa il prossimo numero reale solo durante l'integrazione autorizzata.

---

# 15. Validazione

Esegui i gate effettivamente presenti nel repository.

Minimo:

```bash
python scripts/feature_registry.py validate
```

Se il workflow lo richiede:

```bash
python scripts/feature_registry.py generate
python scripts/feature_registry.py wiki
```

Per la Wiki reale usa il comando previsto dal repository/cloned wiki.

Aggiungi grep/check mirati per evitare regressioni semantiche, ad esempio:

- vecchie classificazioni `Electric Access` su Riktor;
- `Cold Access` su Wraith;
- pagine che presentano Phase come Water Master/Specialist senza blocco AS-BUILT;
- duplicati della matrice proficiency.

Non introdurre test runtime se questa track modifica solo documentazione.

Se tocchi catalogo/codice per il gap Phase, quella è un'altra issue/track e richiede i relativi Automation Test.

---

# 16. Commit suggeriti

Repository principale:

```text
docs(characters): consolidate v0.1 elemental proficiency
```

Wiki clone:

```text
docs(wiki): align v0.1 elemental proficiency
```

Se serve tracking runtime separato:

```text
design(phase): track Water Access implementation gap
```

Mantieni commit separati fra repo principale e Wiki.

---

# 17. Report finale richiesto a Claude

Alla fine restituisci un report con:

1. HEAD/branch verificati;
2. fonti lette;
3. file modificati;
4. owner scelto;
5. Wiki pages modificate;
6. issue aggiornata o nuova issue creata, con numero/link;
7. eventuale issue separata per Phase runtime;
8. Decision Log: modificato oppure preparato per integration;
9. feature registry/roadmap toccati e perché;
10. test/validator eseguiti e risultati;
11. conflitti o gap rimasti;
12. commit creati;
13. prossimo passo consigliato.

## Regola finale

Non fermarti a proporre modifiche: **applica le modifiche consentite dal write-set, valida e riporta l'esito**.

Se un path è bloccato da D-139, non aggirare il vincolo: prepara il delta o la track corretta e dichiaralo nel report.
