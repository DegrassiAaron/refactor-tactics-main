# Runbook — costruire `DA_IconCatalog` e portare `FindMissingRequiredIcons` a zero

> **Owner del significato**: [`08-catalogo-v0.1.md`](../../research/design/icon/visual-language/08-catalogo-v0.1.md)
> per il catalogo, [`07-export-e-naming.md`](../../research/design/icon/visual-language/07-export-e-naming.md)
> per naming ed export. Qui c'è **solo la procedura**.
>
> **Stato misurato**: 2026-08-26. Nessun `DA_IconCatalog` esiste nel repository, e
> `FindMissingRequiredIcons(nullptr)` restituisce l'intera lista — «nessun catalogo non è zero mancanze:
> è la mancanza totale». Al termine di questo runbook restituisce **0**.

## 0. Cosa stai per fare, in una riga

Generare 62 PNG, importarli come texture, e scrivere un data asset che lega **61 chiavi semantiche** alle
loro texture. Le chiavi non le digiti: le deriva `URTIconLibrary::RequiredIconIds()`, che è la stessa
funzione che poi verifica la copertura. È il motivo per cui questa procedura è un commandlet e non una
sessione di clic.

Prerequisiti: branch di lavoro tuo, UE **5.8.1**, il progetto compila, Python 3 con `cairosvg`.

---

## 1. Genera gli asset

```bash
pip install cairosvg          # solo la prima volta
python3 tools/hud-assets/generate_hud_assets.py
```

Devi leggere esattamente questo:

```text
66 icone + 9 cornici -> Content/RT/UI/_Generated
✅ copertura completa: 61 chiavi richieste, tutte disegnate
ℹ️  5 icone fuori dal set richiesto (attese: ability degli eroi)
344 PNG rasterizzati
```

**Se leggi `⛔ N chiavi richieste SENZA icona`, fermati qui.** Significa che il gioco ha guadagnato una
chiave da quando il generatore è stato scritto — una azione nuova a catalogo, un tag `Status.` nuovo, un
eroe in più. Il generatore stampa quali: vanno disegnate prima, non aggirate. Un catalogo con una chiave
scoperta è rotto in un modo che la validazione non distingue da «qualcuno ha cancellato un'icona».

Le 5 «fuori dal set richiesto» sono attese: sono le ability di Gadget. Hanno una chiave regolare sotto
`Action.` ma non stanno nel catalogo generico, quindi `RequiredIconIds()` non le pretende — servono alla
skill bar, non alla copertura.

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
Chiavi richieste: 61
Sorgente PNG: .../Content/RT/UI/_Generated/Icons (taglia 48)
DryRun: 62 PNG presenti, nessun asset scritto. Rilancia senza -DryRun.
```

62 e non 61 perché c'è anche `MissingIcon`, che non è una chiave del dizionario ma un campo di
`URTIconCatalogData` — e senza di lei il catalogo **non passa la validazione**.

---

## 4. Esegui

```bash
UnrealEditor-Cmd RefactorTactics.uproject -run=RTBuildIconCatalog
```

Cosa fa, in ordine:

1. importa i 62 PNG in `/Game/RT/UI/Icons/` con il nome derivato dalla chiave
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

1. `Icons` ha **61** voci;
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
62 texture e 1 data asset.

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

Sono **61 righe**, ognuna con chiave, categoria e texture. Sono 61 occasioni di scrivere
`UI.Icon.Status.Wett` senza che nessuno se ne accorga fino a schermo, e nessuna di quelle 61 righe è una
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
