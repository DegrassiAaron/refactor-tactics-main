# Tre issue in una notte: lo spec panel su #3674, #3680 e #3679

> `SNAPSHOT` · fotografia del 2026-10-12: vale finché è l'ultima misura del suo oggetto.
>
> - **Data**: 2026-10-12, notte. L'autore dorme: nessuna sua scelta può essere chiesta.
> - **Misurato su** `origin/main` = `f5651e9a5`.
> - **Oggetto**: [#3674](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3674) (la piattaforma che a 120° e 180° non si vede), [#3680](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3680) (l'indicatore del piano nel dock), [#3679](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3679) (i piani rispetto a quello attivo).
> - **Decisioni**: [D-497](../../decisions/RT_PDR_00_Decision_Log.md) e [D-501](../../decisions/RT_PDR_00_Decision_Log.md).
> - **Esperti**: Wiegers (requisiti), Fowler (architettura), Nygard (rischi), Crispin (test), Adzic (esempi).
>
> Convenzione: niente totali che cambiano da soli. Dove un numero serve, sta accanto al comando che lo produce.

---

## 1. Il verdetto in una riga

Si può chiudere **#3680** stanotte, solo in C++. **#3674** si risolve con una misura, non con una correzione, se l'ipotesi dell'autore regge. **#3679** si fa a metà senza asset — i piani sotto si attenuano scrivendo il colore — e la traslucenza dei piani sopra chiede un materiale: si fa sul clone principale, d'accordo con la sessione che lo tiene.

## 2. Le regole della notte

- **Nessuna scelta d'autore presa a suo nome.** Dove la spec lascia aperto un punto, la scelta è **provvisoria**, scritta come tale, e reversibile senza ripagare il lavoro: una leva in console o un dato.
- **Nessun verdetto PIE a nome dell'autore.** Le sedute producono catture e log; una voce PIE può diventare verde solo per criteri misurabili da log, e ciò che chiede un giudizio a occhio resta da giudicare.
- **Una issue resta aperta** se la sua DoD chiede un verdetto dell'autore che non c'è.
- **Il clone principale si usa solo se la sessione che lo tiene lo cede**, e torna su `main` pulito con l'Editor chiuso senza salvare (salvo l'asset del lavoro, committato su un branch).

## 3. #3674 — «non si vede» è ambiguo

**WIEGERS**: il titolo dice «non si vede», e non dice se la piattaforma non è disegnata o se è disegnata e non si distingue. Sono due difetti diversi con due rimedi diversi, e la issue elenca tre ipotesi tutte del primo tipo. L'autore ne propone una del secondo: la sovrapposizione.

**ADZIC**: l'esempio che decide. *Dato* `Visual.Map.MultiLevel` in strategica a 120°, *quando* si disegna il solo piano 1, *allora* o la piattaforma compare dove la linea del percorso finisce (sovrapposizione: era lì, grigia su grigio), o non compare (non è disegnata).

**CRISPIN**: con un controllo positivo e uno negativo. A 0° lo stesso isolamento deve mostrarla (positivo). Col solo piano 0, a 120°, non deve esserci (negativo, per sapere che l'isolamento funziona).

**FOWLER**: l'isolamento esiste già, ed è di sola presentazione: `ARTHexMapActor::LayerView = ActiveOnly` con l'`ActiveLayer` di authoring. Sull'attore vivo della PIE, via `ObjectTools.set_properties`, senza toccare nessun asset.

**NYGARD**: se è sovrapposizione, il rimedio non sta in #3674 ma in #3679 — un piano sopra traslucido e uno sotto attenuato si distinguono — e #3674 si chiude con la prova. Se non lo è, restano le ipotesi della issue (bounds, facce, velo), e la si lascia aperta con la misura.

## 4. #3680 — l'indicatore nel dock, senza asset

**FOWLER**: tre scelte di struttura.

1. **Un widget costruito in C++**, `URTLayerIndicatorWidget`, che compone il suo Slate in `RebuildWidget` come fa il Context Inspector: testo e due bottoni. Nessun `.uasset` nuovo, quindi nessun authoring sul clone principale.
2. **Montato dal dock in `NativeConstruct`**, accanto alla lettura del movimento (`MovementReadout`, `BindWidgetOptional`): si inserisce nel pannello che la contiene. È «nel dock» alla lettera, segue il layout del dock, e `ScreenHud.TenantsFitTheirBands` lo misura con il resto.
3. **Una strada sola per il cambio di piano**: i bottoni chiamano `ARTPlayerController::StepActiveLayer`, come `PageUp`/`PageDown`.

**WIEGERS**: il punto aperto della issue, la numerazione. ✅ **Scelta provvisoria**: l'ordinale fra i piani che la mappa ha, da 1 — «Piano N di M» con `N = attivo − min + 1` e `M = max − min + 1`, perché i limiti si misurano sulle celle (`SetActiveLayer`). La quota grezza resta nel log.

**CRISPIN**: i test. Una funzione pura (attivo, minimo, massimo → testo, visibilità, bottoni abilitati), con il caso a un piano solo nascosto; il montaggio nel dock costruito; il click su ▲ che passa da `StepActiveLayer`, con il controllo positivo che ▼ al limite basso è disattivato.

**NYGARD**: il rischio è il dock. Se il `.uasset` non dichiara `MovementReadout`, il dock non ha un pannello dove inserire: allora il widget si aggiunge in coda alla radice del dock, e lo si dice nel log, invece di non montarlo in silenzio.

## 5. #3679 — metà in C++, metà nel materiale

**FOWLER**: la forma.

- **Una funzione pura** decide il trattamento di un piano da (piano, piano attivo, piani della mappa): opacità, saturazione, luminosità, enfasi del bordo. Il piano attivo è identico a oggi; una mappa a un piano non ha trattamento.
- **Un ingresso di presentazione** sulla board, distinto dall'`ActiveLayer` di authoring: lo scrive il controller a ogni cambio, ed è la sola strada.
- **Il colore si compone dopo il velo**: oggi il velo moltiplica l'RGB per istanza, e il trattamento moltiplica il risultato. Una cella mai vista resta velata qualunque sia il suo piano.

**NYGARD**: la parte che costa.

- **Piani sotto (desaturati e scuriti) e bordo marcato: si fanno senza asset.** In `M_HexCell` i tre float per istanza vanno sull'Emissive (#956); il velo già li moltiplica per non toccare il materiale. Il trattamento fa lo stesso.
- **Piani sopra traslucidi: senza materiale non si fanno.** Serve un quarto float per istanza letto da `M_HexCell` come opacità, con `Masked` e dithering (niente ordinamento della traslucenza, profondità e picking invariati). `M_HexCell.uasset` è binario e finora lo si è evitato di proposito: la modifica si fa via bridge (`MaterialTools`) sul clone principale, in un commit a sé.
- **Le famiglie senza dati per istanza** (`Relief`, `Blockers`, `StructuralBodies`, `EdgeFeatures`, `SurfaceVolumes`) non ricevono il trattamento: limite dichiarato. Su `Visual.Map.LayerStack` non ce ne sono.

**WIEGERS**: le intensità sono dell'autore. ✅ **Scelte provvisorie**, tutte leve in console, da scegliere in PIE: opacità dei piani sopra 0,35; ogni piano sotto perde il 50% di saturazione e il 35% di luminosità, cumulati; bordo del piano attivo schiarito del 50%.

**CRISPIN**: i test della DoD, tutti headless:
- la funzione pura;
- il controller che aggiorna la board, con l'`ActiveLayer` di authoring fermo;
- il velo preservato su un piano trattato;
- il raggio che attraversa un piano sopra e risolve sul piano attivo (collisione invariata);
- le unità senza trattamento;
- una verifica di mutazione.

Il giudizio a occhio — «si capisce quale piano è attivo» — resta all'autore: **#3679 non si chiude stanotte**, la sua ultima casella è la sua.

## 6. L'ordine

1. **#3674**, la misura (un avvio PIE dal worktree, `Visual.Map.MultiLevel`): decide se #3679 è anche il suo rimedio.
2. **#3680**, solo C++: build, test, mutazione, review, merge.
3. **#3679**, prima il C++ nel worktree (piani sotto, bordo, ingresso, test), poi il materiale sul clone principale se la sessione che lo tiene lo cede, poi le catture per l'autore.

## 7. Fuori da questa notte

- Le intensità definitive e il verdetto a occhio di #3679 (autore).
- Il trattamento delle famiglie senza dati per istanza.
