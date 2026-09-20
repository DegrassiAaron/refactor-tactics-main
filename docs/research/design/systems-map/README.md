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
| `refactortactics-systems-map.team-knowledge-perceprion-system.png` | Team Knowledge e percezione — refuso `perceprion` nel nome. ⌫ **La ragione scritta qui fino al 2026-09-20 — *«conservato perché il file è citato così altrove»* — era falsa, e si autocitava**: `git grep -F perceprion main -- .` trova **solo questa cella**, e il clone della Wiki non ne ha nessuna. ⚠️ Il refuso sta però in **tre nomi di file**, non in uno — `git ls-tree -r --name-only main \| grep -i perceprion` dà il composite più `.uml.png` e `.infografic.png`, e i due pannelli sono rientrati con `a808419a` (vedi sotto). Rinominarli o lasciarli è una scelta aperta, non un vincolo — ma è una scelta su tre file |
| `refactortactics-systems-map.png` · `refactortactics-systems-uml.png` | Le due panoramiche d'insieme |

## ⚠️ Non estrarre di nuovo i pannelli

Fino al 2026-08-19 ogni tavola esisteva **tre volte**: il composite più i due pannelli come file
separati (`.uml.png`, `.infografic.png`). Non erano una versione a risoluzione maggiore né un ritaglio
diverso: **14 pannelli su 14 sono risultati pixel-identici alla metà corrispondente del composite** —
verificato ritagliando il composite e confrontando con `ImageChops.difference`, differenza `0` su
tutti. ~~La cartella pesava **32,8 MB**; oggi ne pesa **19,7**.~~ `de25da40` (PR #1214) li rimosse, e la
cartella scese da 24 file a 11 — da 32.817.384 a 19.688.048 byte.

### 🔴 La prescrizione qui sopra è stata violata cinque giorni dopo, e i pannelli sono tornati

`a808419a` (2026-08-24, PR #1306 — *«chore(cleanup): il valore di sei branch morti, prima di
cancellarli»*) **ha riaggiunto tutti e quattordici i pannelli**, e sono **gli stessi blob**. Il peso
dichiarato sopra descrive quindi un albero che non esiste da allora.

```sh
# i quattordici che de25da40 aveva cancellato, confrontati per blob con oggi
for f in $(git show --name-status --format='' de25da40 -- docs/research/design/systems-map/ \
           | awk '$1=="D"{print $2}'); do
  [ "$(git rev-parse de25da40^:"$f")" = "$(git rev-parse main:"$f")" ] && echo IDENTICO || echo DIVERSO
done | sort | uniq -c        # -> 14 IDENTICO

git ls-tree -lr main docs/research/design/systems-map/ | awk '{s+=$4;n++} END{print n" file, "s" byte"}'
```

La previsione scritta qui il 2026-08-19 — *«rimetterli riporterebbe 13,1 MB di byte identici»* — si è
avverata **alla cifra prevista**: i quattordici pesano 13.132.692 byte, e lo scarto rispetto alla
sottrazione 32,8 − 19,7 è il `README.md` che `de25da40` aggiungeva nello stesso commit.

🔑 **Perché è successo senza che nessuno se ne accorgesse.** Il commit che li ha riportati confrontava
i candidati **contro i composite già presenti in `main`** e concludeva *«nuovi davvero: nessun
omonimo»* — ed è vero sui nomi e falso sul contenuto, che è precisamente la distinzione che questo
README esisteva per fare. Il gate che avrebbe potuto vederlo, `docs_inventory.py`, era uscito dal
repository con **D-182** tre giorni prima: `git ls-files scripts | wc -l` → `0`.

⚠️ **Con loro è rientrato `refactortactics-guida-10pt.png`**, che è una risurrezione diversa e va
contata a parte: non era fra i quattordici, e va deciso separatamente. Non è elencato nella tabella
qui sopra.

⛔ **Se ritoglierli è una decisione d'autore, non una conseguenza di questa nota.** L'identità per
blob SHA è un fatto — e una prova più forte di quella del 2026-08-19, perché la fa `git` da solo e non
dipende da uno strumento. Ma `de25da40` era una scelta su *cosa `research/` conserva*, `a808419a` l'ha
ribaltata, e ribaltarla di nuovo resta una scelta di contenuto: la registra #1165, non questo file.

Se ti serve un pannello da solo **oggi esiste già come file**; finché ci sono, non vanno ri-estratti né
duplicati. Il composite resta largo esattamente la somma dei due, con
`larghezza(sinistra) = larghezza(infografica)`, quindi il ritaglio resta possibile il giorno in cui la
cartella tornasse ai soli composite. ⛔ E un ritaglio **non** ha lo stesso SHA-256 dell'originale: un
inventario per hash non riconoscerebbe il duplicato — che è la ragione per cui questa nota esiste, e
oggi non ha più un gate che la faccia rispettare.

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
