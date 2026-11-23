# Modello del danno: elemento, RES, scudo base, bonus nel Base, perforante, costo delle reazioni — design

> `RECORD` · **Approvato dall'autore il 2026-11-23** (issue [#4189](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4189),
> fetta 1 dell'epic [#4188](https://github.com/DegrassiAaron/refactor-tactics-main/issues/4188)).
> Le regole stanno in [`D-634`](../../decisions/RT_PDR_00_Decision_Log.md) e vincono su questo testo.

## 1. Intento

Portare in codice le regole del modello del danno che `D-634` ha scelto per il kit di Gadget e che valgono per ogni eroe,
prima delle voci del kit, che ci poggiano sopra. È la fetta 1 dell'epic, scelta dall'autore come primo passo
(«A. Modello del danno (Recommended)»).

## 2. Che cosa è deciso

- Da `D-634`:
  - (4): il danno di un'azione con un elemento è elementale, ridotto dalla RES del bersaglio con la stessa forma della DEF;
  - (5): lo scudo base ferma solo il `Direct`;
  - (6): la RES del catalogo è la RES di partenza per i nove elementi, accanto alla RES per elemento;
  - (35): marchio e +80 contro `Wet` entrano nel Base, prima della DEF;
  - (47): il perforante ignora la difesa e la copertura bassa, non la Guard;
  - (29): una reazione costa almeno la sua classe, 2 AP.
- Dall'autore, il 2026-11-23: in questa fetta solo il meccanismo; le azioni del kit di oggi restano senza elemento, quindi
  `Direct`. Il +80 di `LinearDischarge` va nel Base ma resta legato a lei fino alla fetta 2.

## 3. Obiettivi, non obiettivi

**Obiettivi**
- Un'azione può dichiarare un elemento; il colpo lo porta fino alla mitigazione.
- Un colpo elementale si riduce con la RES di quell'elemento; un colpo senza elemento con la DEF, come oggi.
- Lo scudo base assorbe solo la parte `Direct` del danno di un bersaglio.
- Marchio e +80 contro `Wet` si sommano al Base e passano dalla difesa.
- Un'azione perforante salta difesa e copertura bassa, non la Guard.
- Il validatore rifiuta una reazione che costi meno di 2.

**Non obiettivi**
- Elementi sulle azioni del kit di oggi.
- La riga Acqua–Fulmine come bonus per ogni colpo di Fulmine (fetta 2).
- Le cariche sulle celle (fetta 3).
- Un campo nuovo nel TurnLog.

## 4. Approcci

| Approccio | Cosa comporta | Esito |
|---|---|---|
| **Solo il meccanismo** | Regole nuove, provate con azioni di test; il kit di oggi resta `Direct` | ✅ **scelto dall'autore** |
| Meccanismo ed elementi al kit di oggi | ArcPulse, LinearDischarge e Overload diventano Fulmine; i numeri di Gadget cambiano subito | ❌ scartato |

## 5. Il disegno (scelte dell'assistente, ribaltabili)

1. **Elemento sull'azione.** Un campo `ElementId` (`FName`, vuoto per default) su `FRTActionDef`. `ValidateActions` lo
   rifiuta se non è fra `ElementIds()` (`RTHeroCatalogLibrary.cpp:184-196`). L'intento e il colpo del Blast lo portano
   (`FRTHexAttackIntent`, `FRTHexAttackHit`).
2. **RES nello snapshot.** `FRTHexCombatUnit` riceve `ResistanceDefault` ed `ElementResistance`, copiati da `ARTUnit`
   dove oggi si copia la `Defense` (`RTTurnManager_Blast.cpp:518-519`). `MitigatedHitPower` sceglie la difesa: la DEF
   per un colpo senza elemento, `ResistanceAgainst(ElementId)` per uno elementale. La formula resta `MitigateDirectDamage`,
   con un solo arrotondamento (D-595 (1)).
3. **Bonus nel Base.** Marchio e +80 contro `Wet` non si sommano più a `Hit.Power` già mitigato
   (`RTTurnManager.cpp:6822`, `:6875`). Il colpo si ricalcola dal suo `NominalPower` più il bonus, come fa già
   `ApplyExposedToFirstHit` per il moltiplicatore (`RTHexCombatLibrary.cpp:392-429`): `max(0, Mitigazione(Base + bonus,
   Spalle, Mod, difesa) − Copertura)`. Il bonus del marchio resta sul primo colpo idoneo; quello contro `Wet` resta di
   `LinearDischarge`.
4. **Scudo base.** Il danno di un bersaglio si somma separando la parte `Direct` da quella elementale. Lo scudo temporaneo
   assorbe prima, da qualsiasi fonte, come oggi. Lo scudo base assorbe solo ciò che resta della parte `Direct`. Sulla parte
   che lo scudo temporaneo consuma per prima: **prima l'elementale, poi il `Direct`**, così lo scudo base vale sul `Direct`
   in ogni caso. È una proposta: si può ribaltare, e cambia l'esito solo quando un bersaglio prende tutte e due le parti
   con uno scudo temporaneo attivo.
5. **Perforante.** Un booleano `bPiercing` su `FRTActionDef`: la mitigazione usa difesa 0 e la copertura bassa non si
   sottrae; la Guard e lo scudo restano.
6. **Costo delle reazioni.** `ValidateActions` rifiuta una riga con `Slot == Reaction` e costo sotto la classe reazione
   (`RTCatalogLibrary.h:116-124`). Oggi nessuna riga è sotto, quindi nessun numero cambia.
7. **TurnLog e hash.** Nessun campo nuovo: cambia solo l'`Amount` dove cambia il danno. Il corpus golden si rigenera
   solo dove compare il marchio con una difesa maggiore di 0; i file cambiati per altro restano fuori.

## 6. Verifica

- Test d'automazione nuovi per ogni punto del §5, con un'azione di test elementale, una perforante e una reazione a 1 AP.
- I test esistenti del marchio e del bonus `Wet` (`RTStatusTests.cpp`, `RTOffensiveActionTests.cpp`,
  `RTHexCombatIntegrationTests.cpp`, `RTHeroGadgetTests.cpp`) usano difesa 0: restano verdi, e vi si affianca un caso
  con difesa maggiore di 0, che fissa il punto del bonus.
- Suite `RefactorTactics` intera prima delle revisioni, perché il percorso del resolver è condiviso; corpus golden;
  gate dei documenti.
- PIE: `N/A` per il meccanismo; il solo effetto visibile in partita è il numero del marchio.

## 7. Lasciato a chi esegue

I nomi esatti dei campi, il punto in cui la somma per fonte entra nel resolver (`RTCombatResolver.cpp:33-46`), e quali
documenti di `docs/balance/` descrivono la formula da aggiornare.
