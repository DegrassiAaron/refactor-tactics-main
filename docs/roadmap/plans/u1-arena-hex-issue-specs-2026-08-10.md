# U1 · Mappa-arena hex — le tre issue, specificate

> `CURRENT` · **Stato**: issue create — [#449](https://github.com/DegrassiAaron/refactor-tactics-main/issues/449)
> (A), [#450](https://github.com/DegrassiAaron/refactor-tactics-main/issues/450) (B),
> [#451](https://github.com/DegrassiAaron/refactor-tactics-main/issues/451) (C).
> **A e B sono state applicate** nella stessa sessione; **C resta da eseguire in editor**.
> **Scritto su**: `main` a `a7d26e1` · **Panel**: Wiegers (requisiti), Fowler (confini), Cockburn (attore
> primario), Nygard (guasti silenziosi), Adzic (esempi), Crispin (testabilità)
> **Fonte**: [`editor-sessions.yaml`](../editor-sessions.yaml) → `id: U1` — la sorgente, non
> [`roadmap-editor.md`](../roadmap-editor.md), il cui corpo è `HISTORICAL`.
> **Regola applicata**: nessuna issue dichiara un criterio che il repository non sappia misurare oggi.

---

## 0. Il verdetto del panel, prima delle specifiche

U1 è già scritta bene come **seduta**: passi ordinati, motivazioni, debito dichiarato. Il panel non la
riscrive. Trova che sia **una seduta e tre lavori**, e che il suo `done_when` ne misuri uno solo.

**WIEGERS**: «Il DoD dice: *i due asset sono tracciati da git e le nove voci hanno un esito reale*. Rileggo i
passi. I passi 3, 4 e 7 producono proprietà dell'arena — copertura allineata, zona a costo alto, due rotte
distinte — e **nessuna delle nove voci le guarda**. Le nove verificano il mode: il gizmo dell'Arch, il
secchiello, il `MoveCost` che si aggiorna dal catalogo. Si può quindi soddisfare il DoD alla lettera con
un'arena priva di copertura e con una sola rotta. La seduta si chiude, e U13 e U19 ereditano il buco.»

| Passo | Cosa produce | Chi lo verifica oggi |
|---|---|---|
| 2 · esagono r=4 | forma della mappa | — |
| 3 · copertura allineata | 2–3 `bBlocksMovement` + 2–3 `bBlocksLineOfSight` | `PIE-HEXPLAY-6` (**U4**, non U1) |
| 4 · zona a costo alto | Mud/Water col Fill | `PIE-HEXPLAY-3` (**U3**, non U1) |
| 5 · piattaforma + 1 transizione | layer 1 collegato | `PIE-HEX-TRANS` — ma vedi Fowler |
| 7 · due rotte distinte | scelta di percorso | `PIE-V01-MAPSCALE` (**U19**) |

«Tre delle proprietà sono verificate da sedute **successive**. Non è un errore di per sé — è così che
funziona un artefatto condiviso. È un errore che U1 possa dirsi finita senza che nessuno abbia guardato se
le proprietà ci sono: il difetto emerge in U4, U3 o U19, cioè quando riaprire l'editor costa un'altra
sessione.»

**FOWLER**: «C'è un confine sbagliato, e si vede dalle precondizioni scritte nel registro.
`PIE-HEX-LAYER` chiede *"`ARTHexMapActor` con celle su ≥2 layer (**es. `GenerateIntoAsset` con
`ActiveLayer=0`, poi `ActiveLayer=1`**)"*. `PIE-HEX-TRANS` chiede *"due celle sovrapposte, stessi X/Y, Layer
diverso"*. Entrambe si eseguono su un asset **generato**: non hanno bisogno che l'arena esista. Sono legate a
U1 solo perché ci somigliano. Legarle costa: U1 è sul percorso critico, e due voci eseguibili subito
aspettano un artefatto che non serve loro.»

*Verificato*: `PIE-HEX-LAYER` e `PIE-HEX-TRANS` compaiono **solo** in U1 (`editor-sessions.yaml:146-147`).
Staccarle richiede di riassegnarle, non solo di toglierle — altrimenti finiscono in nessuna seduta, che è
esattamente il difetto da cui è nata U18.

**NYGARD**: «L'avviso sull'allowlist è il pezzo migliore della seduta: descrive un guasto **silenzioso** —
`git add` che non fa nulla, senza errore. Poi dice *"oggi dieci"*. Ne ho contate **sette**.»

```
$ git show a7d26e1:.gitignore | grep -c '^!Content.*\.\(uasset\|umap\)$'
7
```

> **Il comando è pinnato al commit apposta.** Sul `main` di oggi lo stesso `grep` senza `git show`
> restituisce **9**: U1-A ne aggiunge due. Un transcript che dicesse `7` sarebbe falso il giorno dopo il
> merge — cioè l'errore che questo paragrafo denuncia, commesso dal paragrafo stesso. Il numero vero da
> ricordare non è né 7 né 9: è che **il conteggio non va scritto in prosa**, ed è per questo che U1-A lo
> rimuove invece di aggiornarlo.

«Un numero sbagliato dentro un avviso non è un dettaglio: è l'avviso che perde credito. E c'è un problema
più grande di forma. Quel passo **non è lavoro da editor**: è una riga in `.gitignore`, si fa headless, in
cinque minuti, da chiunque. Sta dentro una seduta che solo una persona davanti a Unreal può eseguire. Il
risultato prevedibile è che si scopra di doverla fare **alla fine**, a lavoro fatto, quando `git add` tace.»

**COCKBURN**: «Chiedo chi è l'attore primario, e la risposta non è una sola. *Preparare il repository*:
chiunque, adesso. *Costruire l'arena*: l'autore davanti a Unreal. *Decidere quando l'arena è buona*: chi
scrive la specifica. Tre attori, tre momenti, tre condizioni di fine. Un'unica issue "esegui U1" le fonde e
non è chiudibile da nessuno in particolare.»

**CRISPIN**: «E sul passo 7 — *"due rotte distinte, una più corta ed esposta, una più lunga e coperta"* —
manca l'oracolo. Quanto più lunga? Esposta a cosa? È prosa condivisibile e non è un criterio: due persone la
leggono e costruiscono due mappe diverse, entrambe conformi. `PIE-V01-MAPSCALE` poi misura *il tempo di
contatto* su una mappa la cui proprietà rilevante non è mai stata definita.

Ho controllato dove il criterio possa essere scritto per esteso, e non c'è: **i due documenti si rimandano a
vicenda**.

> U1, passo 7: *"…e `PIE-V01-MAPSCALE` (**U19**) non è verificabile."*
> U19, passo 1: *"verifica che esistano almeno due rotte con trade-off diverso (`PIE-V01-MAPSCALE`, **e vedi
> U1 passo 7**)."*

Ciascuno tratta l'altro come il posto dove la cosa è definita. Non è distrazione: è il sintomo per cui una
proprietà condivisa fra due sedute non trova mai un proprietario. Va scritta **una volta**, in U1, che è
quella che la produce.»

**ADZIC**: «Si risolve con numeri, non con altra prosa. La seduta ha già lo stile giusto altrove —
`BrushRadius=4`, `MoveCost 2 per Rough`, *"una sola transizione"*. Il passo 7 va portato allo stesso
registro.»

---

## Le tre issue

Il panel non spezza per fase (costruisci → verifica): costruzione e verifica avvengono nella **stessa
apertura dell'editor**, separarle sarebbe artificiale. Spezza per **attore e momento**, come chiede
Cockburn.

| | Titolo | Attore | Eseguibile |
|---|---|---|:--:|
| **U1-A** | Allowlist `.gitignore` per i due artefatti dell'arena | chiunque, headless | **adesso** |
| **U1-B** | Criteri misurabili dell'arena, e le due voci che non le appartengono | chi scrive la spec | **adesso** |
| **U1-C** | La seduta: costruire `L_HexArena` e verificare le sette voci | autore, in editor | dopo A e B |

---

## U1-A — Allowlist `.gitignore` per i due artefatti dell'arena

**Attore primario** (Cockburn): chiunque abbia il repository. Nessun editor.

**Why**. `.gitignore` esclude `Content/**/*.uasset` e `Content/**/*.umap` e riammette i singoli file con un
`!` esplicito. Senza le due righe, `git add` sui due artefatti **non fa nulla e non segnala nulla**: la
seduta si conclude, gli asset restano fuori dal repository, e il DoD — *«i due asset sono tracciati da
git»* — risulta non soddisfatto per una ragione invisibile a chi ha appena lavorato due ore in editor.

**Scope**. Due righe in `.gitignore`:

```
!Content/RT/Maps/Dev/L_HexArena/L_HexArena.umap
!Content/RT/Maps/Dev/L_HexArena/Data/DA_HexMap_Arena.uasset
```

E la correzione del conteggio in `editor-sessions.yaml` (`«oggi dieci»` → **sette**, o meglio: togliere il
numero, che invecchia a ogni asset aggiunto).

**Out of scope**. Nessun asset. Nessuna revisione della politica di allowlist.

**Acceptance criteria** (Wiegers — misurabili senza aprire Unreal):
- `git check-ignore -q <artefatto>` esce **1** (non ignorato) per entrambi i percorsi, e esce **0** su un
  `.uasset` di controllo nella stessa cartella *non* in allowlist — la controprova serve, altrimenti il
  criterio passerebbe anche cancellando la regola generale;
- il conteggio scritto nella seduta coincide con
  `grep -c '^!Content.*\.\(uasset\|umap\)$' .gitignore`, **oppure** il numero è stato rimosso;
- nessun altro percorso cambia stato: `git status --porcelain` su `Content/` invariato prima e dopo.

**NYGARD**: «Il criterio è `check-ignore` e non `git add`, perché il modo sbagliato di verificarlo — provare
`git add` e vedere se "funziona" — è indistinguibile dal guasto: in entrambi i casi non succede niente.

E va usato **senza `-v`**. Con `-v` il comando esce `0` anche quando la regola che matcha è una negazione,
quindi restituisce successo sia con l'allowlist a posto sia senza: l'oracolo verboso è più leggibile e non
distingue i due casi. Senza `-v`, l'uscita è `1` esattamente quando il file è versionabile.»

**Dipende da**: nulla. **Sblocca**: il DoD di U1-C.

---

## U1-B — Criteri misurabili dell'arena, e le due voci che non le appartengono

**Attore primario**: chi scrive la specifica. Documentale, nessun editor.

**Why**. Due difetti con la stessa radice — U1 dichiara un DoD che non copre il proprio lavoro:

1. i passi 3, 4 e 7 producono proprietà che nessuna voce di U1 verifica, e le voci che le verificano stanno
   in sedute successive (U3, U4, U19). L'arena può essere dichiarata finita e risultare inadatta dopo;
2. `PIE-HEX-LAYER` e `PIE-HEX-TRANS` non dipendono dall'arena: le loro precondizioni nel registro citano
   `GenerateIntoAsset`, cioè un asset generato.

**Scope**.
- Trasformare i passi 3, 4 e 7 in criteri con un numero, sullo stile già usato dalla seduta stessa
  (`BrushRadius=4`, «una sola transizione»). Il passo 7 in particolare: cosa rende due rotte «distinte»,
  e di quanto la coperta è più lunga.
- Rompere il rimando circolare fra U1 passo 7 e U19 passo 1: il criterio si scrive in **U1**, che produce
  l'arena; U19 lo **cita**, e smette di rimandare indietro.
- Aggiungere a `done_when` una condizione che riguardi **l'artefatto**, non solo il suo tracciamento.
- Riassegnare `PIE-HEX-LAYER` e `PIE-HEX-TRANS` da U1 a **U18** (la seduta che non attende nulla), non
  eliminarle: una voce che non sta in nessuna seduta non viene eseguita mai — è il difetto da cui U18 è nata.
- Rigenerare `editormap.shortlist.md` (`python scripts/feature_registry.py shortlist`).

**Out of scope**. Costruire alcunché. Cambiare le nove voci nel registro `test-manuali-pie.md`: cambiano di
**appartenenza**, non di contenuto.

**ADZIC** — la forma che i criteri dovrebbero avere, da confermare con l'autore, non da imporre:

> *Copertura (passo 3)*: esistono ≥2 celle `bBlocksLineOfSight` tali che la retta fra i due spawn ne
> attraversi almeno una — «allineate fra le due metà» diventa una condizione sul segmento, non un giudizio a
> occhio.
> *Costo (passo 4)*: la zona ad alto costo rende il percorso più breve **non percorribile** nel budget
> Move di un turno — è ciò che `PIE-HEXPLAY-3` chiama «far mordere il budget».
> *Rotte (passo 7)*: due percorsi minimi fra gli spawn che **non condividono celle** oltre agli estremi,
> di costo entro un fattore **1,5** l'uno dall'altro, e con un numero **diverso** di celle esposte alla LOS
> avversaria.

**CRISPIN**: «Il valore non è nella formula esatta: è che qualcuno debba scegliere un numero. Finché resta
"più lunga e coperta", U19 misurerà una mappa e riporterà un dato che non si può riprodurre.»

> **Il fattore 1,5 è la parte da confermare.** Le tre condizioni servono insieme e il perché è
> argomentabile — due rotte gemelle non sono una scelta, e una lunga il doppio non è una scelta o è sempre
> la stessa. Il numero che separa i due casi è invece una decisione di design sulla mappa: **1,5 è la
> proposta scritta a catalogo, non un valore ricavato da una misura**. Va confermato o cambiato in sede di
> costruzione, e in quel caso cambia in un punto solo — U1 passo 7, che U19 cita.

**Acceptance criteria** (Wiegers):
- ciascuno dei passi 3, 4 e 7 ha un criterio verificabile guardando l'asset o una partita, senza giudizio
  estetico;
- `done_when` di U1 include almeno una condizione sulle proprietà dell'arena;
- `PIE-HEX-LAYER` e `PIE-HEX-TRANS` compaiono in **esattamente una** seduta, e non è più U1
  (`grep -c` su `editor-sessions.yaml`);
- nessuno dei due passi rimanda all'altro per il criterio delle rotte: `grep` di «U1 passo 7» in U19 non
  trova più un rimando *definitorio*;
- `python scripts/feature_registry.py shortlist --check` resta pulito (oggi: `shortlist gia' allineate`).

**FOWLER**: «Notare cosa *non* è in scope: decidere se le proprietà servano davvero. Servono — U3, U4 e U19
le presuppongono già. Qui si scrive solo cosa vuol dire averle.»

**Dipende da**: nulla. **Sblocca**: il DoD di U1-C.

---

## U1-C — La seduta: costruire `L_HexArena` e verificare le sette voci

**Attore primario**: l'autore, davanti a Unreal. È la seduta vera; A e B esistono perché questa possa
chiudersi.

**Why**. `DA_HexMap_Arena` e `L_HexArena` non esistono — né in git né su disco. Sono l'artefatto che U13
estende e U19 misura, e costruirli è il modo di esercitare gli strumenti del mode: le sette voci verificano
il **mode**, non il terreno.

**Scope**. I sette passi già scritti in `editor-sessions.yaml`, con i criteri fissati da U1-B, e le sette
voci che restano dopo lo stacco: `PIE-HEX-MODE-E`, `-F`, `-G`, `-H`, `-L`, `-N`, `-O`.

**Out of scope**. Il banco di prova della parità hex: `MapSource = GeneratedTestArena` lo genera già, e
U2…U6 **non aspettano questa seduta**. Il campo cover — arriva con E9 e migra in U13.

**Acceptance criteria**:
- i due artefatti risultano da `git ls-files` (non dalla loro esistenza su disco: è l'oracolo sbagliato,
  perché il guasto di A produce esattamente un file che esiste e non è tracciato);
- le sette voci hanno esito reale — ✅ o ❌ con nota — in `test-manuali-pie.md`; **un ❌ chiude comunque la
  seduta**: il suo prodotto è il verdetto, non il successo;
- l'arena soddisfa i criteri fissati da U1-B.

**NYGARD**: «Il primo criterio va scritto proprio così. `ls` dice che il file c'è; `git ls-files` dice
l'unica cosa che interessa. Sono la stessa risposta nel caso buono e diverse nell'unico caso che fa danno.»

**Rischio dichiarato, non mitigato** — dalla seduta stessa: `FRTHexCellData` oggi non ha il campo cover, E9
incrementa la versione del formato, e quest'arena andrà **migrata** in U13, non ricostruita. Il costo è
accettato consapevolmente.

Un dato che alza la posta: `DA_HexMap_Sandbox.uasset` pesa **1396 byte** — è di fatto vuoto. `DA_HexMap_Arena`
sarebbe quindi il **primo asset mappa con contenuto reale** del repository, e in U13 il primo soggetto vero
della migrazione di formato. Non cambia la decisione; cambia quanto costa sbagliarla.

**Dipende da**: U1-A (altrimenti il commit non avviene), U1-B (altrimenti il DoD non è verificabile).
**Sblocca**: U13, U19.

---

## Cosa il panel non ha toccato

- **L'ordine dei passi** e le loro motivazioni: reggono. In particolare il *perché non estendere
  `DA_HexMap_Sandbox`* — sandbox per prove distruttive, arena stabile per le verifiche — è un confine
  corretto e già argomentato.
- **La scelta di costruire prima di E9**, accettando la migrazione: è una decisione presa con il costo
  scritto accanto, non una svista.
- **Il registro `test-manuali-pie.md`**: le nove voci sono scritte bene, con precondizione ed esito atteso
  distinti. È da lì che è stato possibile accorgersi che due non appartengono a U1.
