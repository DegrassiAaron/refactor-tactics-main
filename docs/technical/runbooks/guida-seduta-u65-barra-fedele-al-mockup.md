# Seduta `U65` — la barra dei comandi fedele al mockup

> **Tipo**: foglio di conduzione · **Scritto**: 2026-10-05, dalla PR del C++ di [#3498] · **Owner del criterio**:
> la voce `U65` di [`editor-sessions.yaml`](../../roadmap/editor-sessions.yaml). Questo foglio **non** ridefinisce
> il criterio: dove divergono, vale la voce.
>
> 🔑 **La fonte delle misure è una sola**:
> [`sorgente-mockup/Main.dc.html`](../../research/design/hud/skill-bar-2026-10/sorgente-mockup/Main.dc.html), lo
> stesso sorgente da cui escono le immagini del mockup. I valori qui sotto sono letti da lì. Il giudizio di
> somiglianza è dell'autore, sulla cattura accanto al mockup.

---

## 0. Preflight

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%' OR Name LIKE 'LiveCodingConsole%'" |
  Select-Object ProcessId, Name, CommandLine
```

Deve essere **vuoto**. La seduta crea e salva asset, quindi si fa nel **clone principale**, che può essere in uso
da un'altra sessione: chiedi prima di cambiargli branch ([`AGENTS.md`](../../../AGENTS.md) §11).

**Precondizione [D-403]**: un commit di `main` che contenga il C++ di [#3498], con l'Editor **ricompilato** da
quel commit. Una DLL di prima non ha le porte di §2, e il `RoundedBox` resterebbe del colore del Designer.

## 1. I font

### 1.1 Da dove vengono

Si importano **i file statici pubblicati dagli autori, non modificati**:

| File | Fonte (commit) | Versione | sha256 |
|---|---|---|---|
| `Orbitron Medium.ttf` | [`theleagueof/orbitron`] `13e6a5222aa6` | 1.000 | `826f71504dfc50c8…` |
| `Orbitron Bold.ttf` | [`theleagueof/orbitron`] `13e6a5222aa6` | 1.000 | `ed4acd72c6ef9ff4…` |
| `fonts/ttf/Exo2-Regular.ttf` | [`googlefonts/Exo-2.0`] `f83ea8a02d3e` | 2.010 | `cb876aebe2b89a6c…` |
| `fonts/ttf/Exo2-SemiBold.ttf` | [`googlefonts/Exo-2.0`] `f83ea8a02d3e` | 2.010 | `65234b9df11abdc7…` |
| `fonts/ttf/Exo2-Bold.ttf` | [`googlefonts/Exo-2.0`] `f83ea8a02d3e` | 2.010 | `9c3646f933ba0f62…` |

```bash
O=13e6a5222aa6818d81c9acd27edd701a2d744152; E=f83ea8a02d3e1d6963ab6e910038521f27e283a2
curl -sSfLO "https://raw.githubusercontent.com/theleagueof/orbitron/$O/Orbitron%20Medium.ttf"   # e Bold
curl -sSfLO "https://raw.githubusercontent.com/googlefonts/Exo-2.0/$E/fonts/ttf/Exo2-SemiBold.ttf"  # e Regular, Bold
sha256sum *.ttf   # confronta con la tabella prima di importare
```

⛔ **Non le istanze del variabile di Google Fonts.** Google Fonts distribuisce Orbitron ed Exo 2 a peso
variabile (`Orbitron[wght].ttf`, `Exo2[wght].ttf`). Ricavarne un peso statico toglie le tabelle di variazione,
e per l'OFL è una *Modified Version*. Orbitron dichiara un *Reserved Font Name*, quindi una sua versione
modificata non potrebbe chiamarsi «Orbitron».

⚠️ **La versione di Orbitron non è quella che il mockup rende.** Il mockup carica Google Fonts, cioè la 2.001;
gli statici degli autori sono la 1.000, del 2011. La differenza va guardata sulla cattura. Se si vede, la via
è un'istanza del variabile **rinominata**, e la sceglie l'autore.

### 1.2 L'import

In `/Game/RT/UI/Fonts/`:

- un **Font Face** per file, con *Loading Policy* `Inline`: `FF_RT_Orbitron_Medium`, `FF_RT_Orbitron_Bold`,
  `FF_RT_Exo2_Regular`, `FF_RT_Exo2_SemiBold`, `FF_RT_Exo2_Bold`;
- due **Font** compositi: `F_RT_Orbitron`, coi typeface `Medium` e `Bold`; `F_RT_Exo2`, coi typeface
  `Regular`, `SemiBold` e `Bold`.

🔑 **Quale peso per quale testo.** Il mockup carica Orbitron solo a 500 e 700
(`fonts.googleapis.com/css2?…Orbitron:wght@500;700`). Un testo Orbitron senza `font-weight` il browser lo rende
quindi col 500: tasto, etichetta di fase, intestazioni di gruppo sono **`Medium`**. Il nome è Exo 2 `600`,
cioè **`SemiBold`**.

### 1.3 Licenza e registro

- Versiona i due testi della licenza accanto ai font: `Content/RT/UI/Fonts/OFL-Orbitron.txt`, da
  `Open Font License.markdown` di Orbitron, e `Content/RT/UI/Fonts/OFL-Exo2.txt`, da `OFL.txt` di Exo 2.
- 🔴 **`.gitignore` li ignora**: la regola `Content/**/*.txt` li esclude, e `git add -A` li salterebbe senza
  errore mentre i `.uasset` dei font entrano. Nello stesso commit aggiungi la negazione accanto alle altre di
  `Content/`, e controlla:

  ```bash
  # in .gitignore:  !Content/RT/UI/Fonts/OFL-*.txt
  git check-ignore -v Content/RT/UI/Fonts/OFL-Orbitron.txt   # deve NON stampare niente
  ```
- Aggiungi in [`asset-licenze.md`](../asset-licenze.md) la riga di `Content/RT/UI/Fonts/`: SIL Open Font
  License 1.1, le due fonti con i commit, l'attribuzione *«il testo OFL accompagna il font»*.

⛔ **Il gate non vede l'assenza di questa riga.** `Content/RT/` è già coperto dalla riga del contenuto
proprietario, e in [`registry.ts`](../../../tools/asset-provenance/registry.ts) vince il prefisso più lungo
**se c'è**. Senza la riga dei font, `check.ts` resta verde e li registra come proprietari. Controlla a mano:

```bash
grep -n "Content/RT/UI/Fonts/" docs/technical/asset-licenze.md   # deve trovare la riga
node tools/asset-provenance/check.ts
```

⚠️ **Il pacchetto deve portare i due testi**, e oggi niente ve li mette. È un follow-up del packaging, non di
questa seduta.

## 2. Lo slot — `WBP_RT_ActionSlot`

L'albero d'arrivo. I nomi segnati con **◆** li legge qualcuno: il C++ come porta ([#3489], [#3498]), un gate
(`ArmedBorder`, `CooldownText`) o il grafo (`IconImage`). Vanno dichiarati con **quel nome esatto**, perché un nome
sbagliato non dà errore e lascia la porta spenta. Gli altri sono liberi.

```text
SlotRoot                 Overlay — nuova radice, SelfHitTestInvisible
├─ SlotColumn            VerticalBox
│  ├─ ◆ GroupHeaderText  Orbitron Medium 10, Letter Spacing 160; padding sotto 10
│  ├─ SlotSize           SizeBox 78×96
│  │  └─ ClickSurface    Button — padding 0, brush senza disegno (vedi sotto)
│  │     └─ ◆ ArmedBorder  Overlay
│  │        ├─ ◆ SelectedGlow    Border RoundedBox, raggio 9, contorno 3 px #FFD456 α 0.16, fondo α 0; slot −3
│  │        ├─ ◆ StateFrame      Border RoundedBox, raggio 6
│  │        ├─ ◆ PhaseStrip      Border, 4 px in alto, raggi 6 6 0 0
│  │        ├─ ◆ HotkeyBadge     Border RoundedBox, raggio 3, min 18×18, fondo #080F14, contorno 1 px #4A5568
│  │        │  └─ ◆ HotkeyText   Orbitron Medium 10, #E6EBF2, centrato
│  │        ├─ ◆ PhaseLabelText  Orbitron Medium 8, Letter Spacing 80
│  │        ├─ VerticalBox       in basso, padding sotto 9
│  │        │  ├─ ◆ IconImage    28×28
│  │        │  └─ ◆ ActionNameText  Exo 2 SemiBold 11, centrato, Auto Wrap a 70, padding sopra 6
│  │        └─ ◆ CooldownText · ◆ PlannedCorner · ◆ UnavailableHatch · ◆ InvalidMark · ◆ WarningMark   come oggi
│  └─ SelectedBarRow     SizeBox, altezza 9
│     └─ ◆ SelectedBar   Border RoundedBox 40×4, raggio 2; padding sopra 5, centrata
└─ ◆ GroupDivider        Border 1 px, #203542; sinistra · riempi, Render Translation X −23
```

Le posizioni assolute del mockup, tradotte in padding dello slot dell'`Overlay`:

| Widget | Allineamento | Padding |
|---|---|---|
| `HotkeyBadge` | alto · sinistra | sinistra 6, alto 9 |
| `PhaseLabelText` | alto · destra | destra 6, alto 12 |
| `PhaseStrip` | alto · riempi | — |
| `SelectedGlow` | riempi · riempi | −3 su ogni lato |

🔑 **Che cosa scrive il C++, e che cosa il Designer.** `RefreshLook` scrive i **colori**, il Designer **forma e
misure**. Per stato il C++ scrive:

- di `StateFrame`, se il brush è un `RoundedBox`: il fondo (`FillColors`, sul `BrushColor`), il contorno
  (`FrameColors`) e lo spessore (`FrameWidths`);
- la tinta dell'icona (`IconTints`) e del nome (`NameTints`);
- la striscia: il colore della fase, al 30% in ricarica, grigia se indisponibile;
- il colore dell'etichetta di fase: viola chiaro su una reazione;
- il colore di `SelectedBar`: ambra, o viola su una reazione armata;
- la visibilità di `SelectedGlow`, `HotkeyBadge`, `GroupHeaderText` e `GroupDivider`, e i tre testi.

Un colore messo nel Designer su questi widget viene **sovrascritto**: lascia bianco il *Tint* dei loro brush.

- **`StateFrame`**: *Use Brush Transparency* spento. Il contorno di un `RoundedBox` non si moltiplica per il
  `BrushColor`: lo shader lo prende come colore secondario dal brush
  (`SlateCore/Private/Rendering/DrawElementTypes.cpp`, `SetOutline`). Fondo e contorno stanno quindi su un
  widget solo.
- **`ClickSurface`**: gli stili `Normal`, `Hovered` e `Pressed` col brush su *Draw As* `NoDrawType`. Il fondo lo dà
  `StateFrame`, e un brush del pulsante lo coprirebbe. Il padding a 0 è quello della §7 della guida U61.
- **`SelectedGlow`** sta **prima** di `StateFrame` nell'`Overlay`, cioè dietro. Il padding −3 lo fa uscire di
  3 px dallo slot: è l'alone `0 0 0 3px` del mockup. Il C++ lo accende solo su un'azione armata che non è
  una reazione.
- **`GroupHeaderText`** fa parte dello slot, non della dock. Il C++ lo rende `Hidden` sulle voci che non aprono
  un gruppo, **non** `Collapsed`: tutti gli slot della fila restano alla stessa altezza.
- **`SelectedBarRow`** tiene i 9 px anche quando `SelectedBar` è `Collapsed`, per la stessa ragione.
- **`ActionNameText`**: al massimo due righe, come «Guardia reattiva» nel mockup. 70 px è la larghezza dello slot
  meno il padding di 4 per lato.
- **`GroupDivider`** sta **fuori** dallo slot, nel `GroupGap` di 45 px che il C++ scrive sul padding sinistro:
  22 px, il divisore, altri 22. La traslazione −23 lo mette a 22 px dallo slot che apre il gruppo. Il C++ lo
  accende solo dove un gruppo ne segue un altro. ⛔ Non va in `SlotBox` della dock: sposterebbe ogni
  `GetChildAt(i)`.

⛔ **`ArmedBorder` non si rinomina**: lo esige `ScreenHud.ActionSlotHasIconSurface`.

⚠️ **`NomeText` e il suo binding `GetActionLine` si tolgono**, sostituiti da `ActionNameText` e `HotkeyText`.
La riga ripeteva tasto e nome, e portava il prefisso dell'armata e il motivo della ricarica. Lo stato armato
ora lo dicono forma e bordo (`SelectedBar`, `SelectedGlow`, `StateFrame`); la ricarica la dice `CooldownText`;
il motivo testuale passa al tooltip di [#3499]. ⚠️ La cella `PIE-V01-SCREENHUD` registra la riga di
`GetActionLine` come il rimedio al suo ➖ sugli stati: dopo questa seduta quel ➖ va **rigiudicato** a schermo.

⛔ **Restano esclusi**:
- i tratteggi: il contorno della reazione armata, di Warning e dello slot vuoto, e la barra della reazione. Un
  `RoundedBox` non li disegna;
- il nome dell'eroe nell'intestazione della Base: il mockup scrive «AEVIK · BASE», ma la vista non porta l'eroe;
- il selettore di profilo ([D-425]);
- la riga «PIANIFICAZIONE» e i chip ([D-456] punto 6).

## 3. Il pannello — `WBP_RT_ActionDock`

Un `Border` `BarPanel` attorno a `BarRow`: `RoundedBox` con raggio 10, fondo `#151A23`, contorno 1 px
`#203542`, padding sopra 20, ai lati 24, sotto 22. Gli spazi fra gruppi (45, col divisore) e fra slot (8) li
scrive il C++ sul padding sinistro di ogni slot (`GroupGap`, `ItemGap`): nel Designer non si toccano.

Il pannello non ha una porta: il C++ non lo legge.

## 4. Il gate, nello stesso commit dell'asset

`ScreenHud.ActionBarDeclaresTheNamedPorts` riceve i sei nomi nuovi dello slot: `HotkeyBadge`, `SelectedGlow` e
`GroupDivider` (`UWidget`), `HotkeyText`, `ActionNameText` e `GroupHeaderText` (`UTextBlock`). Prima
dell'asset sarebbe rosso, quindi entra nella stessa PR. Va provato nei due versi, col blob di `main` e col
nuovo.

## 5. Rileggere dal disco, e la cattura

Chiudi l'Editor, poi, **in un processo fresco**:

```bash
python tools/uasset/names.py Content/RT/UI/Match/WBP_RT_ActionSlot.uasset --unici
# atteso, fra gli altri: HotkeyBadge, HotkeyText, ActionNameText, GroupHeaderText, SelectedGlow, GroupDivider, ArmedBorder
python tools/suite/esegui.py RefactorTactics.ScreenHud
python tools/suite/esegui.py RefactorTactics.Editor
```

La cattura: PIE a 1920×1080 su `L_DevSandbox`, finestra flottante letta con `PrintWindow` (§7 della guida
U61), con un'unità comandata e uno slot armato. Mettila accanto al mockup in
`docs/technical/evidence/hud/u65-barra-accanto-al-mockup.png`.

⛔ **Chi cabla non firma la somiglianza.** Il verdetto è percettivo ed è dell'autore.

[#3489]: https://github.com/DegrassiAaron/refactor-tactics-main/issues/3489
[#3498]: https://github.com/DegrassiAaron/refactor-tactics-main/issues/3498
[#3499]: https://github.com/DegrassiAaron/refactor-tactics-main/issues/3499
[D-403]: ../../decisions/RT_PDR_00_Decision_Log.md
[D-425]: ../../decisions/RT_PDR_00_Decision_Log.md
[D-456]: ../../decisions/RT_PDR_00_Decision_Log.md
[`theleagueof/orbitron`]: https://github.com/theleagueof/orbitron
[`googlefonts/Exo-2.0`]: https://github.com/googlefonts/Exo-2.0
