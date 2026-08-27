# RefactorTactics — set iconografico HUD (mock)

Generato il 2026-08-26 da `tools/hud-assets/generate_hud_assets.py`, deterministico.
Rigenerabile in qualunque momento: `python3 tools/hud-assets/generate_hud_assets.py`.

## Cosa c'è

```
Icons/    123 master SVG + 604 PNG RGBA (16/20/24/32/48 px, solo sopra la soglia leggibile)
Frames/     9 cornici SVG + PNG @1x/@2x — pannelli, slot, bottoni, portrait, timeline, chip
Review/     contact sheet a colori e in scala di grigi
manifest.json   scheda di consegna per ogni asset
```

**122 icone + `MissingIcon`.** Di queste, **61 sono le chiavi che
`URTIconLibrary::RequiredIconIds()` pretende**; le altre 61 sono le 20 ability del roster,
`Action.Dodge` e le cinque categorie che la v0.1 mostra ma il catalogo non dichiara ancora.

## Come sono fatte

- **master monocromatico e tintabile**: la tinta semantica la applica il widget, non l'export.
  Un glifo per tinta sarebbe una texture per ogni tema. Il `manifest.json` porta `SuggestedTint`
  per asset, ma è un suggerimento per il widget — non una decisione cotta nel PNG;
- **niente testo incorporato**: nessun keybind, costo, cooldown o percentuale dentro un glifo.
  I chip di keybind sono cornici vuote;
- **griglia 24**, tratto 1.8, cap e join tondi;
- **il colore è il secondo canale**: la tavola in scala di grigi in `Review/` è il test di
  accettazione, non un extra. Se un'informazione sparisce lì, manca il secondo canale.

## L'alfabeto cotto dentro

Ogni icona `Action.*` porta due strati che **non** sono decorazione:

- **il binario di fase** sul bordo alto — la tacca dice in quale `ResolutionPhase` l'azione
  risolve. Non è periodico di proposito: sei tacche a passo costante si leggerebbero come un
  tratteggio, cioè come `Certainty.Uncertain`. Le due stazioni ai lati del varco centrale sono
  `Control` e `Attack`, la coppia che decide il turno;
- **l'esagono di superficie** in basso a destra, sulle tre azioni che lasciano una superficie
  (`Ignite`, `CreateWater`, `Hero.Phase.MistVeil`). Senza, quelle tre leggono «non fa niente»,
  che è falso: il loro esito *è* la superficie.

Le 20 ability d'eroe portano anche la **marca di materia** in basso a sinistra (nodo, onda,
base larga, transito): serve perché molte abilità a distanza *sono* una linea con una punta, e
senza la marca cinque glifi collassavano l'uno sull'altro.

## ⚠️ Prima di importare: il prefisso è da decidere

I file qui si chiamano **`RT_UI_Icon_<Categoria>_<Nome>`**.

`docs/technical/tooling/brief-icone-v01.md` prescrive invece **`T_UI_Icon_<...>`**, e la fixture
di `RTIconCatalogTests.cpp` usa lo stesso. Ma `progettazione-hud.md` §43 porta fra i suoi esempi
`RT_UI_Icon_Overwatch`. **Due documenti owner si contraddicono**, e la scelta non è mia.

Raccomandazione: `T_UI_Icon_*` per le icone (il brief è più recente, più specifico, ed è quello
che l'issue #219 nomina come autorità) e `RT_UI_*` per le cornici, che sono chrome.

Il rename è una riga in `asset_name()` del generatore e una in `AssetNameForIcon` del
commandlet. **Decidere prima di importare**: il commandlet trova la texture di una chiave
derivandone il nome, quindi un prefisso diverso significa reimportare tutto.

## Import in Unreal

Non a mano: `docs/technical/runbooks/guida-catalogo-icone.md`. In sintesi —

```bash
UnrealEditor-Cmd RefactorTactics.uproject -run=RTBuildIconCatalog -DryRun   # verifica
UnrealEditor-Cmd RefactorTactics.uproject -run=RTBuildIconCatalog           # importa e costruisce
```

Imposta le texture come icone di HUD (`TEXTUREGROUP_UI`, `UserInterface2D`, niente mipmap, sRGB,
`NeverStream`) e costruisce `DA_IconCatalog` derivando ogni chiave da `RequiredIconIds()`.

⚠️ **Il commandlet non è mai stato compilato né eseguito**: richiede Unreal, che non c'era
nell'ambiente in cui è stato scritto.

## Cosa NON è verificato

Niente qui è stato guardato da un giocatore. Le misure sono geometriche e in scala di grigi.
La lettura a quattro classi del binario a 24 px è dichiarata come **scommessa non verificata**
nella specifica, che indica dove misurarla (calibration sheet, E21).
