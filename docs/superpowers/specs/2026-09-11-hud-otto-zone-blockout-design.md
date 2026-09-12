# Screen HUD — le otto zone, e il blockout che le rende visibili

> **Statuto**: design accettato in sessione, non ancora implementato. Il contratto corrente resta
> [`guida-screen-hud-umg.md` §3](../../technical/runbooks/guida-screen-hud-umg.md) finché questo non
> atterra nel `.uasset` e nei gate.
>
> **Stato misurato**: 2026-09-11, branch `design/hud-otto-zone-blockout`, partito da `37d771d4`.
> Ogni numero qui sotto porta il comando che lo produce.

---

## 1. Il problema, e perché non è «aggiungere dei bordi»

La richiesta era vedere le zone dell'HUD disegnate con bordi spessi e colorati, **senza testi**, e da lì
definire e creare gli elementi che ci stanno dentro.

Sembra un lavoro di presentazione. Non lo è, per due ragioni misurate.

### 1.1 Le zone implementate sono quattro; il progetto ne descrive di più

```
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro Zone --unici
→ ZoneBottom, ZoneBottomContainer, ZoneLeft, ZoneRight, Zone_Top
```

[`guida-screen-hud-umg.md` §3](../../technical/runbooks/guida-screen-hud-umg.md) descrive `TOP` / `LEFT` /
`RIGHT` / `BOTTOM` più un `CENTER` a contratto negativo, e combacia con l'asset.

[`progettazione-hud.md` §6](../../technical/systems/progettazione-hud.md) descrive invece posizioni distinte —
Top center, Top left, Top right, Lower left, Right side, Bottom center, Bottom right — fra cui la
**«superiore destra»** da cui la richiesta è partita, che nell'implementazione **non esiste**: l'objective vive
dentro `WBP_RT_TurnHeader`, che occupa tutta la fascia alta.

È un `IMPLEMENTATION DRIFT`: §6 è progetto, §3 più il `.uasset` sono l'implementazione. Questo documento lo
risolve **a favore di §6**, per decisione presa in sessione.

### 1.2 Una zona oggi non è misurabile

Una zona è un `UCanvasPanelSlot` con un nome scelto nel Designer. Per un test è indistinguibile da qualunque
altro nodo: non si può chiedere *«ci sono tutte le zone previste, una sola volta ciascuna?»*.

Non è teoria. `cc5ca967` ha risalvato l'albero dei widget nello stato **precedente** al fix di
[#2760](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2760) — verificato confrontando la
tabella dei nomi del pacchetto a `cc5ca967` e a `bbca9a36`, identiche nome per nome — e **la suite è rimasta
verde**, perché nessun gate guardava l'albero. Due gate lo guardano dal 2026-09-10
([#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697)), ma chiedono *chi è montato*,
non *dove sta*.

---

## 2. Censimento: cosa esiste, e cosa va creato

| Posizione §6 | Contenuto | Widget C++ oggi |
|---|---|---|
| Top left | Team roster | ✅ `URTTeamRosterWidget` |
| Top center | Turno · fase · timer | ✅ `URTTurnHeaderWidget` |
| Top right | Objective | ❌ da creare |
| Lower left | Selected Unit | ✅ `URTSelectedUnitPanelWidget` |
| Right | Team Intent | ❌ da creare |
| Bottom center | Ghost Timeline | ❌ da creare |
| Bottom center | Action Dock | ✅ `URTActionDockWidget` |
| Bottom right | Confirm · Undo | ❌ da creare |
| — | Player Event Log | ✅ `URTPlayerEventLogWidget`, **senza posizione in §6** |

Le assenze sono misurate, non dedotte:

```
grep -ril "teamintent\|team_intent\|AllyIntent" Source/RefactorTactics/   → nessun file
grep -rn -i "ConfirmPlan\|LockIn\|UndoPlan" Source/RefactorTactics/UI/*.h → nessuna riga
grep -ril "ghosttimeline" Source/ Content/  → solo Content/RT_UI_AssetPack_FromHUD/manifest.json
```

### 2.1 Il feed non prende una nona zona

§6.5 promette il lato destro a Team Intent. `ZoneRight` ospita oggi `WBP_RT_EventLogRight`, montato con
[#2697](https://github.com/DegrassiAaron/refactor-tactics-main/issues/2697); §14 descrive il Combat Log
**senza assegnargli una posizione di schermo**. La destra risulta promessa due volte.

Si risolve senza aggiungere zone: `Zone_MiddleRight` **è** la zona; il feed e il futuro Team Intent sono due
**elementi dentro** di essa. Team Intent non esiste ancora, quindi oggi la destra contiene solo il feed — che
non si sposta, e `ScreenHud.TheHudMountsTheFeedThatExplainsTheTurn` resta verde.

---

## 3. Geometria — le otto zone sono la griglia 3×3 meno il centro

Il keep-out di `ScreenHud.PanelsLeaveTheCenterFree` è il **60% centrato** di 1920×1080, cioè `X 384..1536` e
`Y 216..864` (`RTCenterFree::CenterFraction`, `RTMatchWidgetAssetTests.cpp`). Quei tagli cadono a **0.2** e
**0.8** su entrambi gli assi.

Il centro libero *è* la cella centrale di una griglia a fasce **20% / 60% / 20%**. Le zone sono le altre celle.

```
        0.0        0.2                    0.8        1.0
   0.0  ┌──────────┬──────────────────────┬──────────┐
        │ TopLeft  │     TopCenter        │ TopRight │   20%   0 .. 216 px
   0.2  ├──────────┼──────────────────────┼──────────┤
        │ Middle   │      ⛔ CENTRO        │  Middle  │
        │ Left     │      keep-out        │  Right   │   60%   216 .. 864 px
   0.8  ├──────────┼──────────────────────┼──────────┤
        │ Bottom   │    BottomCenter      │  Bottom  │   20%   864 .. 1080 px
        │ Left     │                      │  Right   │
   1.0  └──────────┴──────────────────────┴──────────┘
             20%            60%               20%
```

| Zona | Anchors Min | Anchors Max | Rettangolo a 1920×1080 |
|---|---|---|---|
| `TopLeft` | (0.0, 0.0) | (0.2, 0.2) | X 0..384, Y 0..216 |
| `TopCenter` | (0.2, 0.0) | (0.8, 0.2) | X 384..1536, Y 0..216 |
| `TopRight` | (0.8, 0.0) | (1.0, 0.2) | X 1536..1920, Y 0..216 |
| `MiddleLeft` | (0.0, 0.2) | (0.2, 0.8) | X 0..384, Y 216..864 |
| `MiddleRight` | (0.8, 0.2) | (1.0, 0.8) | X 1536..1920, Y 216..864 |
| `BottomLeft` | (0.0, 0.8) | (0.2, 1.0) | X 0..384, Y 864..1080 |
| `BottomCenter` | (0.2, 0.8) | (0.8, 1.0) | X 384..1536, Y 864..1080 |
| `BottomRight` | (0.8, 0.8) | (1.0, 1.0) | X 1536..1920, Y 864..1080 |

Offset `L4 T4 R4 B4` su tutte.

### 3.1 Tre scelte, e le loro ragioni

🔑 **Anchor stirati su entrambi gli assi, mai a punto.** Le zone scalano con la risoluzione. È anche il ramo
della formula che evita il difetto già trovato su `ZoneBottom`, che con anchor in basso e `Alignment.Y = 0`
finiva a `Y 1080..1280` — fuori schermo — e passava il criterio del centro libero *lasciandolo libero non
esistendo*.

🔑 **I 4 px non sono estetica.** `PanelsLeaveTheCenterFree` fallisce se l'intersezione col keep-out ha
larghezza **e** altezza positive: appoggiarsi esattamente a `Y 216` passerebbe con intersezione nulla, ma un
arrotondamento lo renderebbe un rosso intermittente. Con 4 px il gate è soddisfatto per costruzione.

⚠️ **`MiddleLeft`, non «Lower left».** §6.4 dice *«Lower left — Selected Unit»* senza distinguere la fascia di
mezzo da quella in fondo; in una griglia 3×3 sono due celle. Selected Unit va in `MiddleLeft`, sotto il roster
e di fianco alla mappa. **È un cambiamento visibile in partita**: oggi quel pannello sta in basso.

⚠️ **`BottomLeft` nasce dichiarata e vuota.** §6 non le assegna nulla. Non viene omessa: completa la griglia,
ed è il posto che si guarda per decidere cosa ci va. Se resta vuota a lungo, è una decisione da prendere
guardandola.

---

## 4. `URTHudZoneWidget` — la zona diventa una cosa che i test possono nominare

File nuovo: `Source/RefactorTactics/UI/RTHudZoneWidget.h` / `.cpp`. **Non** dentro
`RTScreenHudWidgets.h`, che è già a 826 righe e ospita sei classi non imparentate.

```cpp
UENUM(BlueprintType)
enum class ERTHudZone : uint8
{
    TopLeft, TopCenter, TopRight,
    MiddleLeft,          MiddleRight,
    BottomLeft, BottomCenter, BottomRight,

    Count   // sentinella per il conteggio: non e' una zona, non assegnarla mai a un ZoneId
};

UCLASS(BlueprintType)
class REFACTORTACTICS_API URTHudZoneWidget : public UUserWidget
{
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD")
    ERTHudZone ZoneId = ERTHudZone::TopLeft;

    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD")
    bool bBlockoutVisible = true;

    UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
    static FLinearColor BlockoutColor(ERTHudZone Zone);

    // L'etichetta diagnostica di una zona, per i messaggi di test e i log. Mai a schermo.
    static FString ZoneName(ERTHudZone Zone);
};
```

🔑 **`UUserWidget`, non `URTScreenHudWidgetBase`.** Una zona è geometria: non interroga il ViewModel, non sa
cosa sia un turno, non ha nulla da aggiornare quando la partita cambia. Derivarla dalla base le darebbe un
accesso al ViewModel che non le serve — e un accesso che esiste, prima o poi qualcuno lo usa. È la stessa
disciplina per cui `URTActionSlotWidget` e `URTFastDecisionOptionWidget` sono `UUserWidget` puri.

🔑 **Il centro non è un valore dell'enum.** È definito da ciò che non contiene; un `ERTHudZone::Center`
inviterebbe a riempirlo, ed è esattamente il difetto che §3.1 di `progettazione-hud.md` vieta.

### 4.1 I colori del blockout non vengono dalla palette di gioco

La palette del progetto è **Okabe-Ito**, e ogni tinta è impegnata semanticamente
([`spec-icon-card-grammar.md`](../../technical/systems/spec-icon-card-grammar.md)): `#009E73` è Movement,
`#D55E00` Attack, `#0072B2` Utility, `#56B4E9` Defense — con i ΔE per dicromazia misurati. Un bordo verde
verrebbe letto come «movimento», perché in questo progetto quel verde **significa** movimento.

Il blockout usa quindi tonalità HSV equispaziate a saturazione e valore pieni, derivate dall'indice
dell'enum: tinte che nella palette di gioco non esistono. Stonano di proposito — un bordo da cantiere che
sembra design finito è un bordo che resta montato.

⛔ **Non passano i criteri di accessibilità del progetto, e non ci provano.** Non raggiungono il giocatore. Se
ci arrivassero, il difetto sarebbe quello, non la scelta cromatica.

---

## 5. L'albero risultante

```
WBP_RT_TacticalHUD  (Canvas Panel — radice, invariata)
├── Zone_TopLeft       [WBP_RT_HudZone · TopLeft]
│     └── Content ▸ WBP_RT_TeamRosterLeft          ← da ZoneLeft
├── Zone_TopCenter     [WBP_RT_HudZone · TopCenter]
│     └── Content ▸ WBP_RT_TurnHeader              ← da Zone_Top
├── Zone_TopRight      [WBP_RT_HudZone · TopRight]
│     └── Content ▸ (vuoto — Objective)
├── Zone_MiddleLeft    [WBP_RT_HudZone · MiddleLeft]
│     └── Content ▸ WBP_RT_SelectedUnitPanelLeft   ← da ZoneBottom, rinominato (v. sotto)
├── Zone_MiddleRight   [WBP_RT_HudZone · MiddleRight]
│     └── Content ▸ WBP_RT_EventLogRight           ← da ZoneRight
├── Zone_BottomLeft    [WBP_RT_HudZone · BottomLeft]
│     └── Content ▸ (vuoto)
├── Zone_BottomCenter  [WBP_RT_HudZone · BottomCenter]
│     └── Content ▸ WBP_RT_ActionDockBottom        ← da ZoneBottomContainer
└── Zone_BottomRight   [WBP_RT_HudZone · BottomRight]
      └── Content ▸ (vuoto — Confirm · Undo)
```

`Zone_Top`, `ZoneLeft`, `ZoneRight`, `ZoneBottom` e `ZoneBottomContainer` **spariscono**. Nessun nodo resta
orfano: tutti i widget montati oggi restano nell'albero, ed è ciò che tiene verdi i due gate di montaggio.

⚠️ **`WBP_RT_SelectedUnitPanelBottom` va rinominato in `WBP_RT_SelectedUnitPanelLeft`**, perché si sposta in
`Zone_MiddleLeft` e il suffisso direbbe il falso. È l'unica istanza il cui nome smette di combaciare con la
posizione: `TeamRosterLeft` resta a sinistra, `EventLogRight` a destra, `ActionDockBottom` in basso. Un nome
che mente sulla posizione è il genere di dettaglio che sopravvive per anni e devia la prossima lettura —
[#1896](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1896) è stata diagnosticata proprio
leggendo il nome di un'istanza nel log PIE.

✅ **Nessun C++ nomina le zone**, quindi la rinomina non rompe codice:

```
grep -rn "ZoneBottomContainer\|ZoneLeft\|ZoneRight\|Zone_Top\|ZoneBottom" \
  Source/RefactorTactics/ --include=*.cpp --include=*.h | grep -v /Tests/   → nessuna riga
```

---

## 6. I gate

### 6.1 I due test nuovi

**`RefactorTactics.ScreenHud.TheEightZonesAreDeclaredExactlyOnce`** — l'albero contiene un
`URTHudZoneWidget` per ogni valore di `ERTHudZone`: nessuno mancante, nessun doppione.

🔑 È la domanda che oggi **nessuno può porre**, ed è la ragione per cui la classe esiste. È anche la domanda
che `cc5ca967` ha eluso.

**`RefactorTactics.ScreenHud.ZoneRectanglesMatchTheThreeByThreeGrid`** — per ogni zona, ricalcola il
rettangolo con la formula di `SConstraintCanvas::OnArrangeChildren` (riusando `RTCenterFree`, non
riscrivendola) e lo confronta con la cella attesa.

🔑 Serve perché `PanelsLeaveTheCenterFree` è un gate **negativo**: dice che nessuna zona invade il centro, e
passerebbe con tutte le zone schiacciate in un angolo. Questo dice dove sono.

### 6.2 I quattro gate esistenti

| Test | Perché tiene |
|---|---|
| `PanelsLeaveTheCenterFree` | Misura i figli di primo livello del Canvas: le zone lo sono, il contenuto dei `NamedSlot` no. Misurerà otto zone invece di quattro. |
| `TheHudMountsTheFeedThatExplainsTheTurn` | Il contenuto di un `NamedSlot` appartiene al `WidgetTree` di chi lo riempie, cioè `WBP_RT_TacticalHUD`. |
| `NoNodeWearsTheNameOfAWidgetWithoutBeingOne` | Le zone si chiamano `Zone_*`, senza prefisso `WBP_`. |
| `EveryZoneOwnerIsMountedByClass` | Resta verde per lo stesso motivo del secondo: verifica che ogni classe-inquilino sia montata nell'albero, e il contenuto di un `NamedSlot` appartiene al `WidgetTree` di chi lo riempie. |

⚠️ **Il secondo è l'assunzione meno solida di questo documento.** Dipende dal comportamento dei `NamedSlot` in
UMG, che **non è stato misurato su questo albero**. Si verifica eseguendo il test dopo il rimontaggio; se cade
lì, il fallimento è previsto da questa riga, non una sorpresa.

### 6.3 Cosa dichiarare

`Compile` e `Tests` si eseguono nel clone che scrive il C++. `PIE` richiede l'Editor: senza, resta
`NOT RUN` **col nome del clone che tiene il motore** — mai un verde ottenuto in finestra sporca.
`Determinism`, `Replay`, `Privacy`, `Packaged`: `N/A` — questo lavoro è presentazione e non tocca stato
canonico né replication.

---

## 7. Procedura — l'ordine non è burocrazia

1. **Clone di sviluppo**: `ERTHudZone`, `URTHudZoneWidget`, i due test. Compilare, eseguire, committare.
2. **Passaggio al clone principale**: pull e ricompilazione. L'authoring asset **non può** avvenire da un
   clone non principale — il bridge MCP è uno solo, e un clone che non ha i file gitignorati salva asset i cui
   riferimenti duri leggono `None`, **azzerandoli senza errore** (`CLAUDE.md` §10).
3. **Editor**: creare `WBP_RT_HudZone` da `URTHudZoneWidget`, col `NamedSlot Content` e il bordo pilotato da
   `bBlockoutVisible`. Il passo 1 deve precedere questo: senza la classe compilata non c'è da cosa derivare.
4. **Rimontare `WBP_RT_TacticalHUD`**: le otto istanze con `ZoneId` distinti, la geometria del §3, i widget
   esistenti nei rispettivi `Content`.
5. 🔴 **Salvare, poi RILEGGERE l'asset dal disco**:
   ```
   python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro Zone --unici
   ```
   Deve rendere gli otto nomi nuovi e **nessuno** dei vecchi. È il passo che mancava alle tre dichiarazioni
   sbagliate del 2026-09-10: l'Editor può risalvare una copia in memoria anteriore, e rileggere il blob è
   l'unico modo di accorgersene.
6. **Suite headless**, poi **PIE** per il giudizio a occhio.

---

## 8. Cosa questo documento NON copre

- **Il contenuto delle zone vuote.** `TopRight` (Objective), `MiddleRight` (Team Intent), `BottomCenter`
  (Ghost Timeline), `BottomRight` (Confirm · Undo) nascono dichiarate e vuote. Progettarle è il lavoro
  successivo, e ciascuna merita il proprio giro.
- **Lo stile finale.** I bordi da cantiere si spengono con `bBlockoutVisible`; che aspetto abbiano le zone in
  produzione è un'altra domanda.
- **`BottomLeft`.** Dichiarata senza inquilino.

---

## 9. Follow-up trovati durante la ricognizione

⛔ **`WBP_RT_FastDecision` non è montato da nessuna parte**, e la guida dice il contrario.
[`guida-screen-hud-umg.md` §3](../../technical/runbooks/guida-screen-hud-umg.md) dichiara che `BOTTOM`
contiene *«`WBP_RT_FastDecision` a runtime»*. Misura:

```
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_TacticalHUD.uasset --filtro FastDecision  → 0 nomi
grep -rn "URTFastDecisionWidget" Source/RefactorTactics/ --include=*.cpp | grep -v /Tests/
#   → solo i metodi della classe: nessun CreateWidget, nessun AddChild
```

L'asset non lo referenzia e nessun C++ lo crea. La finestra di reazione di
[#166](https://github.com/DegrassiAaron/refactor-tactics-main/issues/166) **non può comparire in partita**, e
nessun gate se ne accorge perché nessuno chiede se sia montata. Merita una issue propria: non è causato da
questo lavoro e non va risolto di straforo qui.

⚠️ **§6 di `progettazione-hud.md` e §3 di `guida-screen-hud-umg.md` restano divergenti** finché questo design
non atterra. Quando atterra, §3 va riscritto sulle otto zone, e la riga su `FastDecision` va corretta o
rimossa nello stesso passaggio.
