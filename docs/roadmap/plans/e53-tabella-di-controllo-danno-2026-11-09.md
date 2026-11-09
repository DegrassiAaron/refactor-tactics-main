# E53 · CP 53.1 — La tabella di controllo del danno: colpo × difesa → danno, sulla scala ×10

> `PROPOSED` · **Owner**: E53 ([#4013](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4013)), CP 53.1 · **Scritto**: 2026-11-09 su `main` = `5eac6115e`
> **Decisioni a monte**: [D-587](../../decisions/RT_PDR_00_Decision_Log.md) (bonus alle spalle sulla Base), [D-589](../../decisions/RT_PDR_00_Decision_Log.md) (formula, DEF/RES, ×10, Mod),
> [D-590](../../decisions/RT_PDR_00_Decision_Log.md) (3) (copertura piatta), [D-594](../../decisions/RT_PDR_00_Decision_Log.md) (il vettore AP con cui leggere i rimborsi di budget).
> ⛔ **Nessun numero qui è in vigore.** La tabella mostra che cosa produce la formula decisa sui numeri del catalogo di oggi moltiplicati per dieci; le
> tre scelte che la rendono eseguibile (arrotondamento, `Exposed`, valori iniziali di DEF e RES) sono dell'autore e, una volta prese, diventano la
> `D-nnn` che questa pagina cita in coda.

## 1. La formula, per esteso

```
Danno = max(0, floor_o_round( (Base + Spalle) × Mod × 100 / (100 + DEF) ) − Copertura − Guard)
```

- `Base`: il danno dichiarato dall'azione, ×10 rispetto al catalogo di oggi.
- `Spalle`: il bonus per personaggio dell'attaccante, solo dai tre lati posteriori (D-587); 0 di fronte. Valori: `MSP-2.n` (CP 53.4).
- `Mod`: prodotto dei moltiplicatori di stato (`Marked`, `Exposed`, Vulnerability del PRD); 1 se nessuno (D-589 (3)).
- `DEF`: per il danno diretto; `RES` dell'elemento al suo posto per il danno elementale; il danno ambientale salta `DEF` e passa da `RES` (PRD).
- `Copertura` e `Guard`: riduzioni piatte per colpo, ×10 (oggi 10 e 15 → 100 e 150), sottratte **dopo** la mitigazione (D-589 (3), D-590 (3)).

## 2. I numeri del catalogo di oggi, ×10

| Grandezza | Oggi | ×10 | Dove vive oggi |
|---|---|---|---|
| HP dei quattro | 90 · 95 · 120 · 90 | 900 · 950 · 1200 · 900 | `RTHeroCatalogLibrary.cpp` (`MaxHealth`) |
| `Action.Mortar` | 12 | 120 | `RTCatalogLibrary.cpp` |
| `Action.Counter` | 16 | 160 | idem |
| `Action.CircularAoE` | 18 | 180 | idem |
| `Action.Charge` · `Action.Electrify` | 20 | 200 | idem (Electrify propaga 12 → 120) |
| `Action.LineAttack` | 22 | 220 | idem |
| `Action.PrecisionAttack` | 24 | 240 | idem |
| `Action.HeavyAttack` | 35 | 350 | idem |
| `Hero.Ivrin.PulseShot` · `Hero.Aevik.LinearDischarge` | 21 · 24 | 210 · 240 | `RTHeroCatalogLibrary.cpp` |
| copertura bassa | 10 | 100 | ADR-0005 §4a |
| `GuardReduction` | 15 per personaggio | 150 | `URTCombatLibrary::GuardReductionDefault`, `RTHeroData.h` |
| `Exposed` | +5 al primo colpo | vedi §4 | `URTCombatLibrary::ExposedFirstHitBonus` |
| scudo base | 5 | 50 | `URTCombatLibrary::BaseShield` (D-224) |
| `Burning` | 8 per turno | 80 | `BurningCleanupDamage` |

## 3. La tabella: colpo × DEF, frontale, senza copertura né Guard, arrotondamento per difetto

Le colonne sono i valori di DEF che il GDD dà agli otto eroi (15, 20, 25, 35, 45, 60) più lo zero.

| Colpo (×10) | DEF 0 | DEF 15 | DEF 20 | DEF 25 | DEF 35 | DEF 45 | DEF 60 |
|---|---|---|---|---|---|---|---|
| Mortar 120 | 120 | 104 | 100 | 96 | 88 | 82 | 75 |
| Counter 160 | 160 | 139 | 133 | 128 | 118 | 110 | 100 |
| CircularAoE 180 | 180 | 156 | 150 | 144 | 133 | 124 | 112 |
| Charge · Electrify 200 | 200 | 173 | 166 | 160 | 148 | 137 | 125 |
| LineAttack 220 | 220 | 191 | 183 | 176 | 162 | 151 | 137 |
| PrecisionAttack 240 | 240 | 208 | 200 | 192 | 177 | 165 | 150 |
| HeavyAttack 350 | 350 | 304 | 291 | 280 | 259 | 241 | 218 |
| PulseShot 210 | 210 | 182 | 175 | 168 | 155 | 144 | 131 |

**Arrotondamento.** Per difetto e al più vicino divergono di un punto in 25 celle su 70 (per esempio Mortar contro DEF 35: 88 contro 89; Charge contro DEF 15: 173 contro 174). Il determinismo non distingue le due regole: entrambe sono funzioni intere della stessa coppia; distingue solo chi le scrive diversamente in due punti del codice. Per difetto è la regola che il resolver di oggi già usa per i Movement Point e non favorisce mai l'attaccante.

## 4. Gli esempi che la tabella rende scrivibili (Given / When / Then)

1. **Frontale, nudo.** *Given* un bersaglio con DEF 25 senza copertura né Guard; *when* riceve `PrecisionAttack` (240) di fronte; *then* perde 192 HP (`floor(240 × 100 / 125)`).
2. **Alle spalle.** *Given* lo stesso bersaglio; *when* lo stesso colpo arriva da `Rear`, `RearLeft` o `RearRight` da un attaccante con `Spalle` 60; *then* perde 240 HP (`floor((240 + 60) × 100 / 125)`): il bonus è mitigato come il resto del colpo (D-587 (3)).
3. **In copertura e in guardia, di fronte.** *Given* DEF 35, copertura bassa e `Guard` 150; *when* riceve `HeavyAttack` (350) di fronte; *then* perde 9 HP (`259 − 100 − 150`). ⚠️ È la conseguenza delle tre mitigazioni sommate: oggi, senza DEF, lo stesso colpo fa `35 − 10 − 15 = 10`; con una DEF che vale un quarto del colpo, il residuo scende quasi a zero. Non è un difetto della formula: è il punto in cui i **valori** di copertura e Guard vanno tarati con la DEF in mano (CP 53.3), e la tabella serve a vederlo prima di scriverli.
4. **Il retro toglie la copertura.** *Given* lo stesso bersaglio di 3; *when* lo stesso colpo arriva da dietro da un attaccante con `Spalle` 0; *then* perde 259 HP: senza copertura né Guard (ADR-0005 §4a), il bonus alle spalle non è nemmeno necessario perché il colpo valga.
5. **Mortaio contro il muro.** *Given* DEF 60, copertura e Guard; *when* `Mortar` (120) di fronte; *then* 0 HP (`75 − 250 < 0`): il `max(0, …)` lavora, e un colpo che oggi farebbe zero (`12 − 25`) continua a fare zero.
6. **Elementale.** *Given* RES `Electric` 20; *when* `Electrify` (200) colpisce; *then* 166 HP, e la propagazione (120) nelle celle d'acqua passa dalla stessa RES: 100.
7. **`Exposed`.** *Given* un bersaglio `Exposed` con DEF 25; *when* riceve `PrecisionAttack` di fronte; *then* il danno dipende dalla scelta di §5: con `Mod` 1,2 sono 230 HP (`floor(288 × 100 / 125)`), con +50 sulla Base sono 232 (`floor(290 × 100 / 125)`).

## 5. Le tre scelte che la rendono eseguibile

| Scelta | Che cosa decide | Alternative misurate |
|---|---|---|
| **Arrotondamento** | `floor` o al più vicino, applicato una volta sola dopo il prodotto | divergono di 1 in 25 celle su 70 |
| **`Exposed`** | oggi +5 piatto al primo colpo, cioè +42% su `Mortar`, +21% su `Precision`, +14% su `Heavy`: come moltiplicatore in `Mod` (D-589 (3) lo elenca lì) il valore che conserva il caso medio è **1,2**; +50 sulla Base lo rende piatto come il bonus alle spalle | ×1,2 · ×1,25 · +50 sulla Base |
| **Valori iniziali di DEF e RES** | gli otto del GDD §5.1 (Riktor 45/35 · Steel 60/30 · Kallari 15/20 · Feng Mao 35/25 · Murdock 20/15 · Gadget 15/35 · Muriel 25/40 · The Fey 20/30) come prima riga del catalogo, con la RES unica del GDD applicata a tutti i dodici elementi come **default** | solo DEF, RES a 0 finché un valore per elemento non esiste · nessun valore oggi |

Una quarta cosa la tabella **mostra senza deciderla**: copertura 100 e Guard 150 sommate a una DEF da 35 in su lasciano a un attacco pesante frontale una manciata di punti (esempio 3). È il primo numero che CP 53.3 tara, e va tarato **con** la DEF, non prima.

## 6. Stati di verifica

Compile · Tests · Determinism · Replay · Privacy · PIE · Packaged: `N/A`. La tabella è calcolata (Python, `floor` e `round` a confronto) sui valori letti da `RTCatalogLibrary.cpp`, `RTHeroCatalogLibrary.cpp` e `RTCombatLibrary.h` su `main` = `5eac6115e`; nessun codice cambia.

## 7. Esito (2026-11-09, [D-595](../../decisions/RT_PDR_00_Decision_Log.md))

L'autore ha scelto le quattro opzioni raccomandate: **per difetto**; **`Exposed` 1,2 in `Mod`**; **gli otto valori del GDD come prima riga di DEF e RES, con la RES unica come default per i dodici elementi**; **copertura 100 e Guard 150, da tarare in CP 53.3 con la DEF in mano**, con l'esempio 3 come primo caso di controllo. La formula eseguibile è quindi

```
Danno = max(0, floor((Base + Spalle) × Mod × 100 / (100 + DEF)) − Copertura − Guard)
```

con `Mod` = 1,2 se il bersaglio è `Exposed` al primo colpo diretto, RES dell'elemento al posto di DEF per il danno elementale e ambientale. Nessun numero è nel codice: CP 53.2 scrive gli esempi di §4 come test, CP 53.3 esegue la passata ×10.
