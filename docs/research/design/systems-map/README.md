# Systems Map — otto viste concettuali dei sistemi

> `RESEARCH` · **Aggiornato**: 2026-08-19 · **Non è una fonte**: non risolve conflitti e non descrive
> il codice di oggi. Owner delle regole che queste tavole illustrano: [`../../../gameplay/`](../../../gameplay/)
> e [`../../../technical/`](../../../technical/).

Otto tavole `1536×1024` (una `1491×1055`), una per sistema, più due panoramiche. Ognuna affianca due
pannelli: a sinistra il **concetto** — principi, campi di un Data Asset, tipi di targeting — a destra
il **diagramma UML** dei componenti e delle loro relazioni.

| Tavola | Sistema |
|---|---|
| `refactortactics-systems-map.ability-effect-system.png` | Abilità ed effetti componibili |
| `refactortactics-systems-map.character-state-configuration.png` | Stati del personaggio e configurazione |
| `refactortactics-systems-map.element-env-reaction-system.png` | Elementi, ambiente e reazioni |
| `refactortactics-systems-map.interactive-map.png` | La mappa interattiva |
| `refactortactics-systems-map.movement-facing-system.png` | Movimento e facing |
| `refactortactics-systems-map.reaction-tactic-system.png` | Reazioni tattiche — **solo il composite** |
| `refactortactics-systems-map.status-control-system.png` | Stati e controllo |
| `refactortactics-systems-map.team-knowledge-perceprion-system.png` | Team Knowledge e percezione — refuso `perceprion` nel nome, conservato perché il file è citato così altrove |
| `refactortactics-systems-map.png` · `refactortactics-systems-uml.png` | Le due panoramiche d'insieme |

## ⚠️ Non estrarre di nuovo i pannelli

Fino al 2026-08-19 ogni tavola esisteva **tre volte**: il composite più i due pannelli come file
separati (`.uml.png`, `.infografic.png`). Non erano una versione a risoluzione maggiore né un ritaglio
diverso: **14 pannelli su 14 sono risultati pixel-identici alla metà corrispondente del composite** —
verificato ritagliando il composite e confrontando con `ImageChops.difference`, differenza `0` su
tutti. La cartella pesava **32,8 MB**; oggi ne pesa **19,7**.

Se ti serve un pannello da solo, ritaglialo: il composite è largo esattamente la somma dei due, e
`larghezza(sinistra) = larghezza(infografica)`. Rimetterli come file separati riporterebbe 13,1 MB di
byte identici, e il gate `python scripts/docs_inventory.py --check` non li vedrebbe, perché due
**ritagli** non hanno lo stesso SHA-256 dell'originale.

## ⚠️ Descrivono un'architettura con GAS, che la v0.1 non ha

Il pannello concettuale di `ability-effect-system` si apre con *«GAS gestisce l'intenzione, i costi, i
cooldown»*, e il suo UML ha un blocco `Ability System (GAS Mirror)`. Il canone dice il contrario:
**no GAS nella v0.1**, azioni e personaggi sono data-driven con `URTActionData` / `URTHeroData` /
`URTEquipmentData` — vedi [`AGENTS.md`](../../../../AGENTS.md) §*Decisioni tecniche correnti*.

Non è un difetto da correggere: sono tavole di **visione**, e questa cartella non è normativa. È il
motivo per cui restano in `research/` e non vengono promosse altrove — e per cui, se una di queste
immagini finisse dentro un documento `CURRENT`, quello sarebbe il difetto.

Zero documenti le referenziano, misurato: è normale per materiale non ancora consumato, ed è la
ragione per cui `docs/research/` è un'area dove un'immagine orfana è ammessa.
