# Evidenza EDITOR — reflection e referenze asset

Wave `counter-attack-record/1` · write-set `0eeb5c10` · misurato su `HEAD = 12342a91`.

Risponde alle tre domande del work order §4: `FRTCounterAttack` non riflesso, i sei simboli
rimossi non referenziati da alcun asset, nessun Blueprint che tocchi `FRTReactionPassResult`.

---

## 0. Perché questa misura è possibile senza aprire l'Editor

Il work order dichiarava la ricerca sulla name table **scartata in calibrazione** — «cinque simboli
certamente usati hanno risposto zero, quindi lo strumento non discrimina».

La premessa è falsa, e la ragione è che quella calibrazione non era **omologa al bersaglio**: un
nome di package o di classe vive nella *import table*, un nome di funzione invocata vive come
stringa prodotta dal compilatore Blueprint. Sono due popolazioni diverse. Calibrando sulla specie
giusta lo strumento discrimina, e il controllo negativo torna interpretabile.

⚠️ Senza controllo positivo omologo, uno zero non vale «nessuno lo usa»: vale `NOT MEASURED`.

## 1. Corpus

```bash
for f in $(git ls-files '*.uasset' '*.umap'); do
  tr -c '[:print:]' '\n' < "$f" | grep -oE '^[A-Za-z_][A-Za-z0-9_]{4,}$'
done | sort -u > all_assets.txt
```

```text
asset tracciati:   123
stringhe uniche:   2186
```

Perimetro completo: gli unici 3 `.uasset` su disco non tracciati sono autosave dell'Editor
(`Saved/Autosaves/…`), non contenuto di progetto. LFS è disattivato — `.gitattributes` — quindi
nessun file è un pointer che simuli un binario vuoto.

## 2. Controllo POSITIVO (calibrazione)

```text
UFUNCTION del progetto presenti in almeno un asset:   67 / 455
occorrenze della firma di chiamata 'CallFunc_*':      111
```

Esempi rilevati: `GetPhaseLabel`, `EvaluateMapState`, `CreateScenarioDraft`, `ApplyFixtureFacing`,
`GetBannerVisibility`, `CallFunc_GetResolvedIcon_ReturnValue`.

Lo strumento **vede** i nomi delle funzioni invocate dai Blueprint. Il controllo negativo è quindi
interpretabile.

## 3. Controllo NEGATIVO

```text
CounterAttackSrc          0        FRTCounterAttack          0
CounterActionId           0        FRTReactionPassResult     0
CounterBaseActionId       0        DeflectDelta              0
CounterPriority           0        HazardFlees               0
CounterAttackActors       0        HazardFleeDistance        0
CounterAttacks            0        CancelledDisplacements    0
                                   CancelledControls         0
```

Zero su match esatto e su sottostringa.

### L'unico non-zero, e perché non è nostro

`ActionId` (nome nudo del membro di `FRTCounterAttack`) compare in 4 stringhe:

```text
Action_ActionId · ReturnValue_Main_ActionId · ReturnValue_Movement_ActionId · ReturnValue_Reaction_ActionId
```

Sono split-pin di `FRTPlannedSlotView`, il ViewModel HUD con i tre slot
`Movement`/`Main`/`Reaction` (`Source/RefactorTactics/UI/RTHudViewModel.h:234-240`) — tipo
**riflesso** e distinto. La sorgente riflessa di `ActionId` è `FRTActionDef`
(`RTActionDef.h:423-424`, `UPROPERTY(EditAnywhere, BlueprintReadOnly)`), non il record del
contrattacco.

## 4. Prova strutturale, indipendente dalla misura

`Source/RefactorTactics/Turn/RTReactionPassResult.h` non contiene alcun `USTRUCT`,
`GENERATED_BODY` né `UPROPERTY`. `FRTCounterAttack` (riga 42) e `FRTReactionPassResult` (riga 69)
sono struct C++ semplici, e l'header lo dichiara a riga 38: «*Non è una `USTRUCT`, come il resto di
questo header*».

Un membro non riflesso non esiste per la reflection: nessun Blueprint **può** referenziarlo,
indipendentemente da cosa mostri il grep. La misura empirica sopra è una conferma indipendente,
non l'unico appoggio.

## 5. Comandi per rieseguire

```bash
git rev-parse --short HEAD                      # atteso: l'albero su cui si rimisura
git show 0eeb5c10:Source/RefactorTactics/Turn/RTReactionPassResult.h | grep -c 'GENERATED_BODY'   # atteso 0
# poi §1, §2, §3 sopra
```
