# Runbook — costruire `DA_IconCatalog` e portare `FindMissingRequiredIcons` a zero

> **Owner del significato**: [`08-catalogo-v0.1.md`](../../research/design/icon/visual-language/08-catalogo-v0.1.md)
> per il catalogo, [`07-export-e-naming.md`](../../research/design/icon/visual-language/07-export-e-naming.md)
> per naming ed export. Qui c'è **solo la procedura**.
>
> **Stato misurato**: 2026-08-26. Nessun `DA_IconCatalog` esiste nel repository, e
> `FindMissingRequiredIcons(nullptr)` restituisce l'intera lista — «nessun catalogo non è zero mancanze:
> è la mancanza totale». Al termine di questo runbook restituisce **0**.

## 0. Cosa stai per fare, in una riga

Generare i PNG, importarli come texture, e scrivere un data asset che lega ogni **chiave semantica** alla
sua texture. Le chiavi non le digiti: le deriva `URTIconLibrary::RequiredIconIds()`, che è la stessa
funzione che poi verifica la copertura. È il motivo per cui questa procedura è un commandlet e non una
sessione di clic — e il motivo per cui **quante** siano non è scritto qui: lo dice il comando, non questa
pagina.

Prerequisiti: branch di lavoro tuo, UE **5.8.1**, il progetto compila, Python 3 con `cairosvg`, e
**GTK3 Runtime** — vedi la riga qui sotto, che è la sola parte non ovvia di questa procedura.

---

## 1. Genera gli asset

```bash
pip install cairosvg          # solo la prima volta
python3 tools/hud-assets/generate_hud_assets.py
```

> 🔴 **`pip install cairosvg` NON basta, e crederlo è costato tre giorni** ([#2551](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2551)).
> Il pacchetto si installa e si importa; poi `cairocffi` cerca la libreria **nativa** `libcairo-2.dll` e
> solleva `OSError` **dentro l'import**. Il generatore cade nel ripiego «scritti solo gli SVG» ed esce **0**:
> nessun PNG, nessun errore, e il commandlet del passo 2 si ferma fail-closed su una causa che non nomina.
>
> Su Windows la libreria la porta **GTK3 Runtime**. Se è già installato — controlla
> `C:\Program Files\GTK3-Runtime Win64\bin` — non serve installare nulla: manca solo dal `PATH`.
>
> ```bash
> PATH="/c/Program Files/GTK3-Runtime Win64/bin:$PATH" python tools/hud-assets/generate_hud_assets.py
> ```
>
> ⚠️ **Distinguere i due fallimenti costa un secondo, e la loro confusione è tutto il difetto**:
>
> ```bash
> python -c "import cairosvg"
> ```
>
> `ModuleNotFoundError` → è `pip`. `cannot load library 'libcairo-2.dll'` → è il `PATH`, e il pacchetto
> c'era già. Il messaggio di ripiego del generatore diceva `pip install cairosvg` in **entrambi** i casi:
> corretto nello stesso passaggio.

Devi leggere una cosa di questa **forma** — `xx` sta per un numero che cambia da solo:

```text
xx icone + yy cornici -> Content/RT/UI/_Generated
✅ gate dell'alfabeto: T1 banda libera · T3 riquadro libero · T5 fasi note · T6 aperiodico · T7 fase derivata · T8 colore = fase · T9 palette distinguibile
✅ copertura completa: xx chiavi richieste, tutte disegnate
ℹ️  xx icone fuori dal set richiesto — <categoria> xx, ...
xx PNG rasterizzati
```

> ⚠️ **Le cifre sono volatili, ed è il motivo per cui non stanno più qui.** Questo blocco ha portato
> numeri stantî almeno due volte — diceva «66 icone» quando erano 121, poi «61 chiavi» quando erano 73 —
> e ogni volta l'ha scoperto qualcuno che li confrontava a mano. Nessun gate rimisura un numero scritto in
> prosa ([`AGENTS.md` §14](../../../AGENTS.md)). **Confronta la forma**: le due righe che contano sono
> `✅ gate dell'alfabeto` e `✅ copertura completa`; se sono `⛔`, il generatore ti dice cosa manca.
>
> Se ti serve il numero corrente, misuralo invece di fidarti di questa pagina:
>
> ```bash
> PYTHONIOENCODING=utf-8 python -c "import sys; sys.path.insert(0,'tools/hud-assets'); \
>   import generate_hud_assets as g; print(len(g.required_icon_ids()), 'chiavi richieste')"
> ```
>
> ⚠️ La riga dei PNG dipende da `cairosvg`: se manca la libreria non compare, e i gate `T1/T3` si
> dichiarano «non misurabili» invece di tacere.

> 🔴 **Fino al 2026-09-11 quelle righe non comparivano affatto su console Windows, e questo blocco
> chiedeva di leggere qualcosa di irraggiungibile** ([#3002](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3002)).
> `sys.stdout.encoding` vale `cp1252` e ogni riga di verdetto porta un simbolo che quella codifica non
> ha: il generatore moriva di `UnicodeEncodeError` **dopo** aver scritto tutti i PNG. Gli asset erano
> corretti, spariva solo ciò che dice com'è andata — compreso il `⛔` del paragrafo qui sotto, quello su
> cui questo runbook ti dice di fermarti. Ora il verdetto esce sempre: i simboli veri dove il terminale
> li accetta, sostituiti dove no.

**Se lo lanci da uno script, leggi l'exit code invece dell'output:**

| exit | significato |
|---|---|
| `0` | tutto a posto |
| `1` | chiavi richieste senza icona — il caso in cui questo runbook dice di fermarsi |
| `2` | gate dell'alfabeto caduti |

> ⚠️ **Non metterlo in pipe se poi leggi `$?`.** `python … | tail` restituisce l'exit code di `tail`,
> che è `0` quasi sempre: il fallimento scorre via e il comando sembra riuscito. Quando ti serve sia
> l'output sia l'esito, passa da un file — `python … > gen.log 2>&1; echo $?`.

**Se leggi `⛔ N chiavi richieste SENZA icona`, fermati qui.** Significa che il gioco ha guadagnato una
chiave da quando il generatore è stato scritto — una azione nuova a catalogo, un tag `Status.` nuovo, un
eroe in più. Il generatore stampa quali: vanno disegnate prima, non aggirate. Un catalogo con una chiave
scoperta è rotto in un modo che la validazione non distingue da «qualcuno ha cancellato un'icona».

Le «fuori dal set richiesto» sono attese, e sono **due gruppi**: le **ability d'eroe DERIVATE** da una
core, e le **chiavi del censimento delle sette categorie**
([`10-catalogo-sette-categorie.md`](../../research/design/icon/visual-language/10-catalogo-sette-categorie.md)),
che sono **asset e non chiavi richieste**: `RefactorTactics.IconCatalog.V01CategoriesPopulated` fallisce se una
di loro comparisse in `RequiredIconIds()`, e la roadmap le assegna a **E25**.

> 🔴 **Questo paragrafo diceva che le ability degli eroi stanno TUTTE fuori dal set richiesto, e dal
> [#2963](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2963) è falso.** Il criterio non è
> «sono d'eroe», è **non hanno dove degradare**: un'abilità DERIVATA mostra l'icona della sua core
> (`TideGuard` quella di `Action.Shield`) e resta fuori; un'abilità **propria** non ha quella via —
> `MakeActionIconFallbackId` torna `None` — e senza il proprio glifo il dock mostra `MissingIcon`. Quelle
> sono ora chiavi richieste a pieno titolo, e il commandlet le pretende fail-closed.
>
> ⚠️ Se aggiungi un'abilità propria, il suo glifo va disegnato **prima**: non è un ripiego che manca, è
> la regola che si autoalimenta.

> ⚠️ L'output sta in `Content/RT/UI/_Generated/`, che è **ignorato da git**. È corretto: si rigenera. Non
> aggiungerlo al repository.

---

## 2. Compila il modulo Editor

Il commandlet è nuovo e il modulo ha una dipendenza in più (`AssetTools`). Rigenera i project files e
ricompila **RefactorTacticsEditor**, non solo il modulo runtime:

- Windows: click destro su `RefactorTactics.uproject` → *Generate Visual Studio project files*, poi
  compila la configurazione `Development Editor`.

Se salti questo passo, il comando del punto 3 fallisce con «commandlet non trovato», che è il sintomo di
un modulo non ricompilato e non di un errore di sintassi nel comando.

---

## 3. Prova a vuoto — **fallo sempre**

```bash
UnrealEditor-Cmd RefactorTactics.uproject -run=RTBuildIconCatalog -DryRun
```

`-DryRun` non scrive niente. Verifica una cosa sola, ed è quella che conta: che ogni chiave richiesta
abbia il suo PNG. Attesa:

```text
Chiavi richieste: xx
Sorgente PNG: .../Content/RT/UI/_Generated/Icons (taglia 48)
DryRun: yy PNG presenti, nessun asset scritto. Rilancia senza -DryRun.
```

🔑 **`yy` è sempre `xx + 1`, e quell'uno è `MissingIcon`**: non è una chiave del dizionario ma un campo di
`URTIconCatalogData` — e senza di lei il catalogo **non passa la validazione**. È la relazione fra i due
numeri a dover tornare, non il loro valore.

---

## 4. Esegui

```bash
UnrealEditor-Cmd RefactorTactics.uproject -run=RTBuildIconCatalog
```

Cosa fa, in ordine:

1. importa i PNG in `/Game/RT/UI/Icons/` con il nome derivato dalla chiave
   (`UI.Icon.Action.Move` → `RT_UI_Icon_Action_Move`);
2. imposta ogni texture come icona di HUD — `TEXTUREGROUP_UI`, `UserInterface2D`, niente mipmap, sRGB,
   `NeverStream`. Non è estetica: con la compressione di default i bordi netti prendono artefatti, ed è
   il motivo per cui un import «a occhio» sembra sempre peggiore del PNG di partenza;
3. crea o aggiorna `/Game/RT/UI/DA_IconCatalog`, **ricostruendo le voci da zero**;
4. valorizza `MissingIcon`;
5. salva;
6. stampa il verdetto, che non è suo — lo danno le funzioni del gioco.

Verdetto atteso:

```text
--- verdetto ---
FindMissingRequiredIcons: 0
ValidateIconCatalog: 0 errori
Catalogo completo e valido.
```

Il commandlet esce con codice **1** se una delle due non è a zero. Non salva un asset che sembra a posto.

---

## 5. Verifica dentro l'Editor

Apri `DA_IconCatalog` e controlla tre cose, in quest'ordine:

1. `Icons` ha una voce per chiave richiesta;
2. `MissingIcon` è valorizzato (non `None`);
3. apri due voci a caso e guarda che `Category` combaci col segmento dentro `IconId` — è il confronto che
   `ValidateIconCatalog` fa con `StartsWith`, e l'unico errore che un catalogo compilato a mano fa
   davvero: chiave giusta, categoria sbagliata.

Poi i test, che sono la verifica vera:

```text
Window → Test Automation → RefactorTactics.IconCatalog
```

---

## 6. Committa

⚠️ Qui, a differenza degli asset generati, **si committa**: `.gitignore` versiona esplicitamente
`Content/RT/UI/**/*.uasset` (blocco «ECCEZIONE: UI e mappe di RT SONO versionate»). Entrano
una texture per chiave richiesta, più `MissingIcon`, più 1 data asset.

⛔ E vale la regola dei binari: **un `.uasset` non si fonde**. Se qualcun altro sta importando icone
sullo stesso branch, uno dei due lavori va perso senza conflitto visibile. Un import alla volta.

---

## 7. Quando qualcosa non torna

| Sintomo | Causa | Rimedio |
|---|---|---|
| «commandlet non trovato» | modulo Editor non ricompilato | punto 2 |
| `⛔ N chiavi richieste SENZA icona` | il gioco ha una chiave nuova | disegnala nel generatore; non aggirare |
| `N chiavi senza PNG` dal commandlet | generatore non eseguito, o `-Size` diverso da quelle esportate | ripeti il punto 1; le taglie esportate sono 16/20/24/32/48 |
| `'X' non comincia con una categoria di ERTIconCategory` | la chiave non è formata come `UI.Icon.<Categoria>.<Nome>` | è un errore della **chiave**, non dell'import: si corregge dove la chiave nasce |
| `N chiavi senza texture importata` | import parzialmente fallito | guarda gli errori dell'import sopra nel log; rilancia — è idempotente |
| icone sfocate a schermo | texture importate senza le impostazioni UI | rilancia il commandlet: le riapplica |

---

## 8. Se proprio vuoi farlo a mano

Si può: *Content Browser → Add → Miscellaneous → Data Asset → RTIconCatalogData*, poi riempire `Icons`.

Una riga per chiave, ognuna con chiave, categoria e texture. Sono altrettante occasioni di scrivere
`UI.Icon.Status.Wett` senza che nessuno se ne accorga fino a schermo, e nessuna di quelle righe è una
decisione: sono tutte derivabili. Se lo fai a mano, fallo per una voce sola — per capire la forma — e poi
lascia fare il resto al commandlet.

---

## 9. Cosa cambierà (e presto)

L'handoff del 2026-08-26 su Action Phases sposta `Dash` a **sola macro-fase** e introduce `Dodge` come
movimento generico della fase Dash. Se quel rename atterra nel catalogo generico:

- `UI.Icon.Action.Dodge` **esce** dalle chiavi richieste e `UI.Icon.Action.Dodge` entra — automaticamente,
  perché `RequiredIconIds()` legge il catalogo e non una lista;
- il generatore lo segnalerà come chiave scoperta al primo lancio successivo;
- `UI.Icon.Phase.Dash` **resta**: la fase non cambia nome.

Non anticipare disegnando `Dodge` adesso: un'icona per un'azione che nessuno può pianificare è un debito
senza soggetto. Il giorno del rename, il generatore te lo chiede.
