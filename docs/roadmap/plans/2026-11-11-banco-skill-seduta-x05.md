# La seduta del banco: una skill alla volta, a x0,5, e l'OK dell'autore

> Decisione: [`D-607`](../../decisions/RT_PDR_00_Decision_Log.md) (2026-11-11). Issue: [#4071](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4071) (x0,5), [#4072](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4072) (il bersaglio adatto), [#4073](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4073) (le scene delle reazioni), [#3795](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3795) (la riga «BANCO i/n» coperta).
> Misure sul codice: lettura di `origin/main` = `fb0b726e6` il 2026-11-11.
> Rivisto con `/sc:spec-panel` in modalità critica: i rilievi sono in §6, già applicati.

## 1. Lo scopo

L'autore guarda **una skill alla volta**, a **x0,5**, nel banco Ability Lab, e dà il suo OK alle voci PIE che chiedono di vedere una singola skill in azione. La prima è il fumetto (`PIE-HUD-FUMETTO`), nel playback della risoluzione.

## 2. Cosa c'è già

- **Il banco** è il pannello *Ability / Hero Lab* dell'Editor: si sceglie un'azione e si preme «Esegui in PIE». Lo verifica `PIE-LAB-PIE`.
  - Monta una scena minima (`URTAbilityLabLibrary::BuildFixture`): una mappa di raggio 3 con due unità, l'eroe di prova in (0,0) e il bersaglio in (2,0).
  - Esegue il dato di produzione nel playback vero.
- **Il passo del banco**: `,` e `.` passano da un'azione all'altra del kit (`PIE-LAB-PASSO`).
- **I comandi di playback**: `K` pausa, `L` micro-passo (`rt.Debug.PlaybackControls`).
- **Gli scenari di reazione e di carica**, fuori dal banco: `Visual.Reaction.Deflection`, `Visual.Reaction.Interposition`, `Visual.Reaction.OverwatchWindow*` e `Visual.Movement.Charge`.

## 3. Cosa mancava, e le fette che lo portano

1. **x0,5 non esisteva.** La scala del tasto è x1 · x2 · x4 ([`D-350`](../../decisions/RT_PDR_00_Decision_Log.md)). `rt.Debug.PlaybackSpeed=0.5` era ignorata, e anche un valore sulla scala valeva solo in autobattle. → **#4071.** x0,5 vale solo nel banco. Anche dopo una finestra di reazione il playback torna a x0,5, mentre in partita torna a x1. Il tetto di frame per turno della scena si allunga nella stessa misura: altrimenti una scena lunga chiuderebbe in ERROR a metà playback ([`D-607`](../../decisions/RT_PDR_00_Decision_Log.md) (5)(c)).
2. **Il bersaglio era sempre un nemico.** Facevano eccezione solo le azioni su se stessi e quelle che posano una superficie o una struttura. → **#4072.**
   - Il **tipo** di bersaglio (unità, cella, bordo o nessuno) lo decide `URTPointerLibrary::TargetKindForAction`.
   - Per un'unità, il bersaglio è un **alleato** se gli effetti fanno bene a chi li riceve (`Heal`, `Shield`, `CancelStatus`, `DamageReduction`), altrimenti un **nemico**.
   - Una cella è vuota. Un bordo è il lato verso chi lancia, con la porta per `Action.Interact`.
   - Chi monta la scena può forzare il bersaglio.
3. **Il banco non aveva scene di reazione.** → **#4073.** La provocazione si legge dal trigger del catalogo:

   | Reazione | Cosa la provoca |
   |---|---|
   | `Counter`, `Deflect` | un nemico colpisce l'eroe di prova |
   | `Intercept` | un alleato in più, colpito da un nemico |
   | `Anchor` | un nemico lo spinge |
   | `Cleanse` | un nemico gli applica uno stato di controllo |
   | `HazardEscape` | il fuoco acceso sotto di lui |
   | `Overwatch` | un nemico entra nella sua linea |
   | `Brace` | un nemico lo spinge; con `Profile.Sidestep` e `playerReactionWindows` si apre la finestra |

4. **La riga «BANCO i/n» è coperta** dalle etichette delle unità. → **#3795**, da chiudere prima della seduta se possibile.
5. **Il clone principale** va portato su `main` e ricompilato prima della seduta, perché il fumetto di #4059 arrivi all'Editor. Si fa con l'OK dell'autore, a motore libero.

## 4. Il copione della seduta

La pilota l'autore dal pannello del Lab in PIE ([`D-607`](../../decisions/RT_PDR_00_Decision_Log.md) (3)). L'assistente prepara l'ordine delle scene e legge i log.

**All'inizio** si cerca nel log la riga `[RT] rt.Debug.PlaybackSpeed: il playback del banco parte a x0.5 ([D-607]).` Un x0,5 ignorato tornerebbe a x1 in silenzio.

| # | Scena | Bersaglio | Cosa si guarda | Voce e criteri |
|---|---|---|---|---|
| 1 | `Hero.Aevik.LinearDischarge` | nemico | l'header dice «Velocità: x0.5»; il fumetto col nome del kit, per circa 6 s reali (3 s di playback), sopra il nome e senza coprirlo; la riga nel Registro; `K` lo ferma; con `Tab` si legge nel Registro | `PIE-LAB-PASSO` (5), `PIE-HUD-FUMETTO` (a)(h)(e)(e′)(L2), `PIE-HUD-REGISTRO-ATTIVA` (a)(b) |
| 2 | `Hero.Branth.ImpactShot` | nemico | anche l'attacco base grida | FUMETTO (g), REGISTRO (d) |
| 3 | `Action.Guard` | nessuno | anche una generica grida | FUMETTO (g) |
| 4 | `Gadget.PortableCover` | bordo | anche l'equipaggiamento grida | FUMETTO (g), REGISTRO (d) |
| 5 | `Action.Heal` | alleato | grida, e compare «+N» sull'alleato | FUMETTO (g) |
| 6 | `Hero.Ivrin.PassingBlade` | nemico sulla traiettoria | grida nel Dash, poi scatta | FUMETTO (g), `PIE-LAB-PASSO` (0) |
| 7 | `Action.Move`, `Action.Interact` | cella, porta | **non** gridano; l'interazione ha la sua riga | FUMETTO (k), REGISTRO (d) |
| 8 | `Counter`, `Deflect`, `Intercept` | provocate | il fumetto all'arrivo del colpo; la parata si legge come una difesa; il colpo cambia destinatario | FUMETTO (j), `PIE-VIS-DEFLECT`, `PIE-VIS-INTERPOSE` |
| 9 | `Brace` con la finestra | spinto | la risposta grida dopo la finestra, e il playback torna a x0,5 | FUMETTO (i)(j), `PIE-V01-RXBRACE` |
| 10 | `Overwatch`, anche con un bersaglio non visto | entra nella linea | il fumetto quando il bersaglio arriva sulla cella | FUMETTO (j) |
| 11 | `Reaction.HazardEscape` | il fuoco sotto i piedi | la fuga grida dopo il movimento | FUMETTO (j) |
| 12 | `Visual.Movement.Charge` | Aevik | la carica si distingue dal passo | `PIE-VIS-CHARGE` |
| — | `Spazio` in una scena qualsiasi | — | dopo il salto restano tutte le righe, senza doppioni | REGISTRO (e)(f) |

**Fuori dalla seduta**, perché vogliono una partita intera:

- `PIE-HUD-FUMETTO` (c), il nemico fuori vista; (d), due attivazioni nello stesso istante; (f);
- `PIE-HUD-REGISTRO-ATTIVA` (c).

Restano `NOT RUN`, scritti come tali.

## 5. Come si registra l'OK

1. Per ogni scena l'assistente mostra il testo del criterio.
2. L'autore dice «ok», oppure cosa vede.
3. L'assistente scrive il verdetto parola per parola nel registro PIE (`docs/technical/test-manuali-pie.md`), con il commit, la scena e la velocità. Una risposta incerta si riporta con la sua incertezza, come in `PIE-VIS-CHARGE`.

Il verdetto è dell'occhio dell'autore. La lettura dei log dell'assistente serve solo per gli orari e per i controlli.

## 6. La revisione del panel (modalità critica)

- **Wiegers.** «OK» vale per criterio, non per voce, e su un testo che l'autore ha visto. → La colonna «Voce e criteri» dà i criteri per lettera.
- **Adzic.** Ogni scena ha un esempio osservabile. Dato il banco a x0,5, quando parte `LinearDischarge`, allora il fumetto dura circa 6 s reali. → La colonna «Cosa si guarda».
- **Cockburn.** Non mescolare lo sviluppo dei tool con il giudizio. → Prima #4071-#4073 con i loro test, poi la seduta.
- **Crispin.** Dichiarare cosa il banco non può mostrare, e mettere i casi muti come controllo. → La scena 7 e il paragrafo «Fuori dalla seduta».
- **Nygard.** Oggi x0,5 fallisce in silenzio, cioè torna a x1. → Il controllo della riga di log all'inizio della seduta, e #3795.
- **Fowler.** La regola del lato del bersaglio vive nel banco, che è uno strumento e non una regola di gioco. → Riusa `TargetKindForAction` e gli effetti del catalogo; nessun secondo classificatore nel gameplay.
