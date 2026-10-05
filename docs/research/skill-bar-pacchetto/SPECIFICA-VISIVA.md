# Skill bar — specifica visiva (PROPOSTA)

> **Statuto**: sorgente di design, **non canone**. Il layout non è prescritto da D-407. I valori numerici sono in `dati/tokens.json`, i comandi in `dati/comandi.json`.
> **Regola sovraordinata** (02-color-system §1, progettazione-hud §47-bis.1): il colore è il **secondo** canale. Ogni stato deve restare distinguibile in scala di grigi (vedi la riga inferiore di `immagini/08-stati-slot.png`).

## 1. Le tavole

| Immagine | Che cosa mostra |
|---|---|
| `01-default-arc-pulse-move.png` | Stato base: attacco base selezionato, profilo Move, Sprint bloccato |
| `02-guardia-reattiva-riserva-withdraw.png` | Overwatch: lo slot movimento è riservato a Withdraw, gli altri profili sono bloccati |
| `03-irrigidimento-root.png` | Brace: `Root`, nessun profilo |
| `04-interagisci-sprint-reazione-armata.png` | Interact + Sprint + reazione armata: i tre slot occupati insieme |
| `05-skill-kit-compatibilita-da-dichiarare.png` | Skill del kit senza `MinStability` dichiarato: profili in stato *da verificare* |
| `06-attesa.png` | Wait: nessuno spostamento volontario |
| `07-solo-movimento.png` | Nessuna principale, solo Sneak |
| `08-stati-slot.png` | Gli 8 stati di §7, a colori e in scala di grigi |

## 2. Anatomia dello slot (layer di §7.1, dall'alto al basso dello z-order)

1. **Frame**: fondo + bordo (cambia per stato).
2. **Striscia di fase**: 4 px in alto, colore D-233.
3. **Etichetta di fase**: testo, in alto a destra (`PREP`, `BLAST`, `CLEANUP`, `REAZ.`, `—`). È il secondo canale della striscia e **non si omette**.
4. **Badge tasto**: in alto a sinistra, da `HotkeyLabel`. Mai testo cotto nel PNG.
5. **Icona**: 28 px, da `DA_IconCatalog` via `IconId`. ⚠️ Le icone del mockup sono **segnaposto**: non si importano.
6. **Nome**: 11 px, al massimo 2 righe.
7. **Overlay di stato**: cooldown, marker, badge.

## 3. Ricette degli stati

| Stato | Frame | Secondo canale (forma/pattern) | Icona/testo |
|---|---|---|---|
| Available | `BG_Raised`, bordo 1 px `Frame_Mid` | — | pieno |
| Hover | fondo più chiaro, bordo 1 px `Cyan` | luminosità | pieno |
| Selected | fondo `#2B2918`, bordo **2 px** `Amber`, alone | **barra 40×4 sotto lo slot** | icona `Amber` |
| Planned | bordo **2 px** `Amber` | **angolo pieno 18 px** in alto a destra | pieno |
| Cooldown | `BG_Panel`, bordo spento, striscia al 30% | **numero turni 26 px** sopra l'icona | spenti |
| Unavailable | **tratteggio diagonale** 135° | pattern | spenti |
| Invalid | fondo `#2A1719`, bordo 2 px `Red` | **✕** in alto a destra | pieno |
| Warning | bordo **2 px tratteggiato** `Amber` | **triangolo !** in alto a destra | pieno |
| Reazione armata | fondo `#221E3A`, bordo **2 px tratteggiato** `Violet` | barra sotto lo slot **tratteggiata** | icona viola |

⚠️ Selected e Warning condividono l'ambra (§32 la assegna a entrambi). Li separa la **forma**: bordo continuo + barra contro bordo tratteggiato + triangolo.

## 4. Composizione della dock

```
[COMUNI: G B C X Z] | [BASE: 1, Difesa caratt.] [KIT: 2 3 4 5] | [PROFILO: Withdraw Sneak Move Sprint]
                                                                   (FUTURE — #1410/#653)
```

- I gruppi sono un **aiuto alla lettura** (§6.7), non quattro economie d'azione.
- L'ordine di `GetActions()` è **identità** (D-397): il raggruppamento legge un campo, non riordina la lista.
- Sopra la dock, a destra: tre chip **PRINCIPALE · MOVIMENTO · REAZIONE**, gli slot occupati dell'unità selezionata. Il valore è vuoto (`—`) per un nemico ispezionato.

## 5. Ciò che le immagini mostrano ma NON si implementa ora

- Il **selettore di profilo** (FUTURE, D-397).
- Lo **slot Difesa caratteristica** (FUTURE: `ERTActionSlot::SignatureDefense` non esiste).
- Il cooldown di Overload è uno **stato dimostrativo**, non un dato.
- Le **icone** sono segnaposto.
