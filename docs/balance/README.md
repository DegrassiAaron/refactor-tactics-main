# Cataloghi di bilanciamento v0.1

> `CANONICAL` per i **numeri** · **Ultimo aggiornamento**: 2026-08-08
>
> I valori vigenti della v0.1, in Markdown versionato e diffabile: il bilanciamento si revisiona in PR, non
> riaprendo un PDF. Le **decisioni** restano nel canone
> ([`piano-canonico-mvp.md`](../product/piano-canonico-mvp.md)); lo **stato di avanzamento** nella
> [roadmap](../roadmap/roadmap-checkpoint.md) — questo README non lo duplica più.

## ⚠️ Chi tocca una stat rigenera i radar, nello stesso commit

I rating dei radar di personaggio **non sono scritti da nessuna parte**: si calcolano da questi cataloghi
a ogni generazione ([D-106](../decisions/RT_PDR_00_Decision_Log.md)). Gli SVG in
[`docs/characters/radar/`](../characters/radar/) sono **versionati con un gate**
([D-108](../decisions/RT_PDR_00_Decision_Log.md)), quindi cambiare `Salute`, `Movimento`, un danno o un
cooldown li rende **rossi** finché non vengono rigenerati.

```sh
node tools/radar/generate.ts           # riscrive gli otto SVG (Profile e Balance per quattro eroi)
node tools/radar/generate.ts --check   # verifica, exit 1 se divergono
```

E chi rigenera i radar riallinea anche il **testo alternativo** delle immagini sulla Wiki, che
ripete i valori disegnati sui raggi:

```sh
node tools/radar/wiki-alt.ts --wiki-root <clone> --check   # exit 1 se un alt e' rimasto indietro
node tools/radar/wiki-alt.ts --wiki-root <clone> --write   # lo riallinea
```

Senza questo secondo gate la catena si spezzava sull'ultimo anello: gli `alt` sono prosa scritta a
mano nel clone, quindi un rebalance aggiornava il grafico e lasciava indietro la descrizione — chi
vede il radar leggeva il numero nuovo, chi usa uno screen reader sentiva quello vecchio.

Serve **Node 22+** e nient'altro: nessun `npm install`, nessun build step. È il prezzo dichiarato di
D-108 — la toolchain Node è un prerequisito del **bilanciamento**, non solo della documentazione.

⚠️ **Gli SVG non si editano a mano.** Sono output: la correzione si fa qui o sulla rubrica
(`tools/radar/`).

## Quali file sono normativi

| File | Possiede | Autorevole? |
|---|---|---|
| [`RT_ActionCatalog_v0.1.md`](RT_ActionCatalog_v0.1.md) | ~35 azioni: ID stabile, macro-fase, priorità, range, costo, cooldown, fallback, interrompibilità | ✅ **sì** |
| [`RT_HeroCatalog_v0.1.md`](RT_HeroCatalog_v0.1.md) | Gadget · Phase · Riktor · Wraith: statistiche, abilità, varianti, loadout | ✅ **sì** |
| [`RT_TerrainCatalog_v0.1.md`](RT_TerrainCatalog_v0.1.md) | 8 terreni, stati ambientali, coperture e strutture | ✅ **sì** |
| [`RT_EquipmentCatalog_v0.1.md`](RT_EquipmentCatalog_v0.1.md) | Varianti arma, gadget, moduli di reazione | ✅ **sì** |
| [`RT_FootprintCatalog_v0.1.md`](RT_FootprintCatalog_v0.1.md) | I tre profili di posa: `Small` 2, `Medium` 3, `Large` 4 settori contigui | ✅ **sì** — ⚠️ **sorgente senza lettore**: nessuna unità dichiara ancora un footprint |
| [`RT_TestMatrix_v0.1.md`](RT_TestMatrix_v0.1.md) | Requisito → test → criterio di accettazione | ✅ come **mappa dei test**, non come stato |
| `RefactorTactics_Balance_Matrices_v0.1.xlsx` | Matrici di esplorazione | ❌ **no** — `RESEARCH`, vedi sotto |

## ⚠️ Il workbook non è una fonte

`RefactorTactics_Balance_Matrices_v0.1.xlsx` è **`RESEARCH`** ([D-023](../decisions/RT_PDR_00_Decision_Log.md)).
**Non si usa per risolvere un conflitto contro i cataloghi `.md`.** Quando i due divergono, vince il Markdown —
sempre, senza discutere quale sia più recente.

Contiene ancora, e sono tutte esplorazioni superate:

| Foglio | Cosa dice | Perché non vale |
|---|---|---|
| `02_Roster` | banca archetipi Paragon | il roster è Gadget · Phase · Riktor · Wraith |
| `06_Abilita_VS` | Steel · Aurora · Murdock · Kwang | personaggi mai adottati |
| `07_Fast_Reactions` | finestra **5–7 s** | la baseline è **3,0 s** ([ADR-0004](../decisions/adr-0004-finestre-di-reazione.md) §8) |
| `15_Azioni_Turni` | `FastReaction_sec = 6`; planning 35/35/40; `Max_Turni = 12` | `RoundLimit` è un parametro di formato ([D-010](../decisions/RT_PDR_00_Decision_Log.md)), non una costante |
| note percezione | «360° base, coni solo Overwatch/sensori» | incompatibile con [ADR-0005](../decisions/adr-0005-orientamento.md): il cono frontale è generale |

**Non correggerlo cella per cella.** Un workbook rattoppato diventerebbe una falsa fonte corrente, che è
peggio di uno dichiaratamente vecchio. Un eventuale `v0.2` andrà **derivato** dai cataloghi Markdown,
versionato e validato — idealmente generato, così che non possa più divergere da solo.

## Chi possiede un numero, e chi lo cita soltanto

I cataloghi hero/action **possiedono** i numeri competitivi. Wiki, pagine di sinergia, pagine di fazione e
scenari li **citano e li linkano**: non ne tengono una seconda copia normativa
([D-029](../decisions/RT_PDR_00_Decision_Log.md) ·
[ADR-0006](../decisions/adr-0006-ownership-abilita-sinergie.md) ·
[`../gameplay/spec-ownership-abilita-interazioni-sinergie.md`](../gameplay/spec-ownership-abilita-interazioni-sinergie.md)).

Corollari operativi:

- un'abilità appartiene a **un** owner: non esistono kit di coppia o di fazione, e una sinergia non è un
  `AbilityId`;
- una condizione sistemica (`+8 su bersaglio Wet`) vive nel catalogo dell'**abilità che la dichiara**, non nella
  pagina della sinergia che la usa come esempio;
- se una pagina di sinergia deve spiegare un numero, lo **linka** al catalogo owner. Un valore duplicato in una
  pagina divulgativa è lo stesso difetto della regola #1 qui sotto, solo più difficile da trovare.

## Come si cambia un valore

1. **Trova l'owner.** Un numero vive in **un** catalogo. Se lo trovi in due, è un difetto: apri una issue,
   non aggiornarne uno solo.
2. **Cambialo lì**, in PR, con il motivo nel messaggio di commit.
3. **Aggiorna il codice** se il valore è già consumato: i cataloghi sono la fonte, ma non si applicano da soli.
   Cerca lo Stable ID (`grep -rn "Action.Sprint" Source/`) prima di dichiarare fatto.
4. **Aggancialo a una verifica.** Un valore senza consumatore è il difetto ricorrente di questo progetto: se
   nessun test e nessun playtest lo tocca, è un numero che *sembra* deciso. Registra il test in
   [`RT_TestMatrix_v0.1.md`](RT_TestMatrix_v0.1.md), o la voce di taratura in
   [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md) sotto «Da playtestare».

Dove un numero **manca nella fonte**, il file lo dichiara *non specificato*. Non lo si inventa: un numero
plausibile scritto in un catalogo autorevole è indistinguibile da uno deciso.

## Divergenze dai PDF di origine

Elencate in fondo a ogni file, col motivo. La principale: le macro-fasi restano quelle di *Atlas Reactor*
(`Prep → Dash → Blast → Move`, **Move dopo il Blast**), mentre il catalogo PDR metteva il movimento prima
dell'attacco. I codici di fase del catalogo sopravvivono come attributo dell'azione
([ADR-0003 §3](../decisions/adr-0003-modello-azioni-v01.md)).

**Fonte**: il catalogo di bilanciamento v0.1 e il PDR-12, adottati con
[ADR-0003](../decisions/adr-0003-modello-azioni-v01.md). Erano due PDF; dal 2026-08-12 il loro testo è in
[`prd-personaggi-azioni-e-bilanciamento.md`](../research/prd/prd-personaggi-azioni-e-bilanciamento.md) e
[`RT_PDR_v0.1_consolidato.md`](../archive/pdr-v0.1/RT_PDR_v0.1_consolidato.md).
