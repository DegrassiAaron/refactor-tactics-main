# `#1663` e `#1665` — il meccanismo di cook e una premessa scaduta, spec panel

> `CURRENT` · **Stato**: revisione chiusa, le azioni sono state applicate · **Data**: 2026-09-03
> **HEAD**: `1bbdd7f6` (`origin/main`), worktree `rt-wt-cook`, branch `fix/1663-1665-cook-e-premessa`
> **Oggetto**: le acceptance criteria residue di
> [`#1665`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1665) e il meccanismo di cook che
> [`D-262`](../../decisions/RT_PDR_00_Decision_Log.md) ordina per
> [`#1663`](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663).
>
> **Perché esiste**: è il seguito di
> [`1663-1665-blocker-packaged-spec-panel-2026-08-30.md`](1663-1665-blocker-packaged-spec-panel-2026-08-30.md),
> che restava aperto su due punti — *«non ho isolato cosa lo abbia risolto»* per `#1665`, e la scelta del
> meccanismo per `#1663`. Entrambi si chiudono qui, e nessuno dei due richiedeva un pacchetto.

## Il verdetto, in breve

✅ **`#1665` è chiusa nella sostanza**, e la domanda che la teneva aperta aveva una risposta in `git`.

🔴 **Tre premesse su cui si è ragionato erano false**, e nessuna delle tre era stata misurata:

| premessa | dove viveva | esito |
|---|---|---|
| *«il canale forma non esiste ancora per le superfici»* | registro PIE, stato di `PIE-V01-BOARD` | ❌ **falsa dal 2026-08-23** |
| *«elencare gli asset per nome (`DirectoriesToAlwaysCook`, o una lista di asset)»* | `OPEN_DECISIONS.md`, istruttoria di `COOK-1` | ❌ **non esprimibile**: solo directory |
| *«le mesh entrano perché i `BP_Unit_*` le referenziano duro»* | corpo di `#1663` | ✅ **vera, e misurata qui per la prima volta** |

⚠️ Le prime due hanno lo stesso difetto di metodo: **una premessa d'infrastruttura assunta invece che
verificata**, e in entrambi i casi ha orientato una decisione.

## `#1665` — la causa della non-riproduzione, isolata senza costruire nulla

**Karl Wiegers · la domanda era ben posta, e la risposta era a portata**

Il commento del 2026-09-02 elencava tre esiti possibili e dichiarava di non saper scegliere:

> *«dire "è passato" senza saperlo significherebbe non poter riconoscere il ritorno»*

✅ **Merito: è la reticenza giusta.** Un difetto che sparisce senza spiegazione non è un difetto chiuso.

❌ **Ma la domanda era decidibile con `git merge-base`, non con un pacchetto.** Il difetto fu osservato su
`7f6efff4` (2026-08-29 11:33); i quattro fix sono del 2026-08-30; il pacchetto che non lo riproduce è
`641713e3` (2026-09-02).

```
$ git merge-base --is-ancestor 43206237 7f6efff4   # PR #1745 — NO
$ git merge-base --is-ancestor d0d4b188 7f6efff4   # PR #1747 — NO
```

∴ **esito 1**, e l'«altro lavoro» che l'ha risolto **è questa stessa issue**. Non c'è nessuna terza
condizione ignota: la ragione è nominata da due simboli, `MaterialIndex = -1` e il quinto ramo senza
`RebuildInstances`, entrambi con presidio in suite.

**Lisa Crispin · l'`AC-4` era rimandata a una premessa caduta**

❌ **CRITICO · il registro dichiarava bloccante qualcosa che era arrivato sette giorni prima.**
`PIE-V01-BOARD` portava lo stato *«il canale forma non esiste ancora per le superfici — è il lavoro di
#956»*. **#956 è chiusa `COMPLETED` dal 2026-08-23** (merge di #1291, `67458c65`).

✅ **Verificato nel codice e non solo nel tracker** — `RTHexMapActor.h` porta `SurfaceGlyphs[4]`,
`GlyphCells[4]`, `GetCellGlyphMesh(int32 RingCount)`.

🔴 **E il registro lo diceva già da solo, 112 righe più sotto**: `PIE-GBX-SURFACE` scrive *«il suo canale
forma è arrivato con CP 47.3 (#956, chiusa)»*. **Due voci dello stesso file in contraddizione**, e a essere
letta il 2026-08-30 è stata quella scaduta.

📝 **Applicato**: commit `2751376a`. La voce resta ⏳ perché nessuno l'ha eseguita — che è un'altra cosa dal
non poterla eseguire.

⚠️ **La lezione non è sul singolo stato.** Una colonna di stato che nomina una issue come bloccante non ha
nessun meccanismo che la aggiorni quando quella issue chiude. `issue-refs.ts` verifica che i **percorsi**
citati esistano, non che le **premesse** citate reggano.

## `#1663` — il meccanismo, e le due vie che non esistevano

**Martin Fowler · il vincolo del motore cambia la forma della decisione**

❌ **CRITICO · `D-262` e `OPEN_DECISIONS` discutono un'opzione che il motore non offre.**
L'istruttoria di `COOK-1` recita: *«Elencarli per nome (`DirectoriesToAlwaysCook`, o una lista di asset)»*.

Verificato sulla documentazione ufficiale, non dedotto:

1. **`DirectoriesToAlwaysCook` accetta directory, mai singoli asset.** *«This setting is specifically
   designed to specify entire directory paths… The documentation doesn't indicate support for specifying
   individual asset paths.»*
2. **L'Asset Manager governa i Primary Asset.** Una `UAnimSequence` è un asset **secondario**: *«Secondary
   Assets are not handled directly by the Asset Manager… To cook non-primary assets, they must be
   referenced by Primary Assets.»*

∴ **il «set minimo esplicito» che `D-262` ordina non è esprimibile in `.ini`.** Non è una scelta fra due
vie equivalenti: una delle due non esiste.

**Alistair Cockburn · il precedente era nel repository, mai misurato**

✅ **La premessa del corpo di `#1663` è vera, e questa è la prima volta che qualcuno l'ha guardata.**
Misurato il 2026-09-03 sui quattro `.uasset` versionati, con lo strumento **calibrato** su token di
controllo prima di fidarsi di uno zero:

| asset | riferimento duro a `FabAsset` | animazioni |
|---|---|--:|
| `BP_Unit_Gadget` | `.../ParagonGadget/Characters/Heroes/Gadget/Meshes/Gadget` | **0** |
| `BP_Unit_Phase` | `.../ParagonPhase/Characters/Heroes/Phase/Meshes/Phase_GDC` | **0** |
| `BP_Unit_Riktor` | `.../ParagonRiktor/Characters/Heroes/Riktor/Meshes/Riktor` | **0** |
| `BP_Unit_Wraith` | `.../ParagonWraith/Characters/Heroes/Wraith/Meshes/Wraith` | **0** |

⚠️ **Il primo tentativo di questa misura ha dato zero su tutto, ed era falso**: `strings` non esiste su
questa macchina e il comando falliva in silenzio. Uno zero da uno strumento assente ha lo stesso aspetto di
uno zero da un file pulito — è la stessa trappola che `DefaultGame.ini` documenta per il container.

🔑 **Conseguenza che scioglie l'obiezione di `COOK-1`.** L'istruttoria temeva che elencare gli asset per
nome facesse passare *«chi clona senza i pack da un degrado silenzioso a un cook che fallisce»*. Ma il
repository ha **già** quattro asset versionati con riferimenti duri a un pack gitignorato: **quel costo è
già pagato**, e per la famiglia sorella degli stessi file. Aggiungere le clip accanto alla mesh nello stesso
`BP_Unit_*` non introduce una classe di rischio nuova — la estende di due voci per eroe.

**✅ Decisione d'autore, 2026-09-03**: il meccanismo è il **riferimento duro dai `BP_Unit_*`**.

**Gojko Adzic · il criterio scritto come funzione, e non come numero**

✅ **Applicato.** Lo spec panel del 2026-08-30 raccomandava *«ogni `TSoftObjectPtr` dichiarato da
`URTUnitAnimInstance` risolve nel container: l'insieme si ricava dal codice, e cresce da solo»*, contro un
`AC-1` che *«passa con una clip su otto»*.

Il gate consegnato deriva il set dal **CDO**, non da una lista:

```cpp
for (const TPair<FName, FRTLocomotionClips>& Voce : Cdo->ClipsPerHero)
    Richieste.AddUnique(Path.GetLongPackageName());
```

∴ i dodici montaggi di `#288` porteranno il perimetro da 8 a 20 **senza toccare il test**.

**Michael Nygard · l'oracolo che non può essere verde per vacuità**

Un gate che conta «le clip scoperte» ha due modi opposti di essere inutile. Il test ne porta **tre**
difese, e la terza è quella che conta:

| controllo | cosa impedisce |
|---|---|
| il roster dichiara ≥1 clip | un set vuoto renderebbe verde il ciclo senza guardare niente |
| ci sono package sotto `Content/RT` | zero file letti ⇒ tutte scoperte, e il rosso non direbbe niente |
| 🔑 **la mesh Paragon di Gadget È trovata** | se la scansione non trova **nemmeno** un riferimento che so esserci, il rosso significa *«l'oracolo non sa guardare»* |

✅ **Misurato: il controllo positivo passa e il gate è rosso su 8/8.** Le otto clip elencate sono
**esattamente** le otto degli `SkipPackage` letti su un pacchetto vero il 2026-08-29. Il gate riproduce in
automation un difetto che finora si vedeva solo aprendo una build.

⛔ **E resterà rosso finché la seduta Editor non aggiunge i riferimenti**: è ciò che `D-262` ordina
(*«un pacchetto che si avvia con unità immobili deve essere rosso, non silenziosamente verde»*), ma **non
va mergiato rosso in `main`**. Il ramo resta aperto sulla seduta.

## Cosa questa revisione NON ha fatto

⛔ **Nessun pacchetto costruito.** `Content/FabAsset/` è **assente in questo checkout**: le clip non ci sono
neppure sul disco, quindi da qui l'`AC-1` di `#1663` — le otto clip nel `.utoc` — non è misurabile. È lo
stesso limite che il 2026-08-30 produsse `SkipPackage = 0` su un worktree senza pack, e che il commento di
allora giustamente non lesse come buona notizia.

⛔ **Nessuna seduta Editor.** I riferimenti duri nei `BP_Unit_*` richiedono l'Editor **e** i pack sul disco.
È il passo che rende verde il gate, ed è d'autore.

⛔ **`PIE-V01-BOARD` non è stata eseguita**: chiede un giudizio umano a picco e in scala di grigi.

⛔ **Nessuna decisione nuova aperta.** `D-262` copre il meccanismo; ciò che si aggiunge qui è la misura che
ne **restringe** le opzioni, e va riportata nella sua istruttoria invece che decisa daccapo.
