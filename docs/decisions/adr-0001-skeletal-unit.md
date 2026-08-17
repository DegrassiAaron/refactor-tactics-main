# ADR-0001 — Unità da Static Mesh (cilindro) a Skeletal Mesh animata

> `CANONICAL` · **Stato**: **Accettato — prototipo di presentazione, rollout differito** · **Data**: 2026-08-03
> · **Decisore**: utente (dev singolo)
>
> **Stato al 2026-08-08.** La parte architetturale è **in codice**: `ARTUnit::VisualZOffset` esiste e
> `ARTGameMode::HeroUnitClasses` è una `TMap<FName, TSubclassOf<ARTUnit>>`, cioè lo spawn per eroe è
> configurabile come previsto dai punti 2 e 4. Ciò che **non** è deciso è il contenuto: **nessuna
> corrispondenza fra i personaggi Paragon e il roster canonico (Gadget · Phase · Riktor · Wraith) è stata
> scelta**. Questo ADR non va letto come se il roster avesse già una resa visiva definitiva; la mappatura è
> una voce aperta in [`../OPEN_DECISIONS.md`](../OPEN_DECISIONS.md).
> **Contesto sorgente**: `/sc:spec-panel` → [`spec-asset-pipeline.md`](../technical/spec-asset-pipeline.md)
> Primo ADR del progetto (non esisteva convenzione ADR: questo file la inaugura; gli ADR vivono in `docs/decisions/` dal 2026-08-07).

## Contesto

L'MVP rappresenta le unità come **cilindri** (`ARTUnit::Mesh = UStaticMeshComponent`, `RTUnit.cpp:15-25`).
Il canone (`piano-canonico-mvp.md §9`) prevede *«asset placeholder / Starter Content»* e rimanda la direzione
artistica al post-MVP. L'utente (2026-08-03) ha deciso di introdurre **personaggi skeletali animati** — passo
**north-star** che eccede lo scope MVP. Passare da `UStaticMeshComponent` a `USkeletalMeshComponent` + Animation
Blueprint è un **cambio architetturale della presentazione** e va documentato prima dell'implementazione
(CLAUDE.md: non sovrascrivere/estendere il canone in silenzio; ADR per cambi architetturali).

Vincoli: invariante #1 (le regole decidono, l'animazione riproduce), #4 (determinismo), split canonico
(**regole in C++, presentazione in Blueprint**), e la suite di **60 automation test** che non deve regredire.

## Decisione

Introdurre lo skeletal **in modo additivo e reversibile**, non sostitutivo:

1. **`BP_Unit : ARTUnit`** (Blueprint) aggiunge `USkeletalMeshComponent` + `ABP_Unit`. La logica resta in C++.
2. `ARTGameMode::SpawnUnit` (`RTGameMode.cpp:80`) spawna una **`TSubclassOf<ARTUnit>` configurabile per
   archetipo**, invece di `ARTUnit::StaticClass()` fisso.
3. Riferimenti asset **soft** (`TSoftObjectPtr`/`TSoftClassPtr`); se assenti, l'unità **resta il cilindro**
   (fallback), come già fa `UnitMaterial` (`RTUnit.cpp:65`).
4. Esporre `VisualZOffset` (`UPROPERTY`) per gestire il **pivot ai piedi** dei personaggi UE, sostituendo
   l'`UnitHalfHeight = 90` hardcoded in `WorldForCell` (`RTUnit.cpp:89`).
5. Animazioni **pilotate dai delegate** di playback (`RTTurnManager.h:71-84`), non da `GetVelocity`
   (il movimento è `SetActorLocation`, senza velocità reale).

## Alternative considerate

| Alternativa | Perché scartata |
|---|---|
| **A. Sostituire il root** `UStaticMeshComponent` → `USkeletalMeshComponent` in C++ | Più invasivo; rompe fallback e selezione; presentazione in C++ contro il canone |
| **B. Skeletal in C++ puro** su `ARTUnit` | Meno flessibile per il designer; presentazione dovrebbe stare in BP |
| **C. `BP_Unit` derivato + soft-ptr + fallback** ✅ | Additivo, reversibile, presentazione in BP, test invarianti — **scelta** |

## Conseguenze

**Positive**
- Il **playback non cambia**: muove l'Actor via `SetVisualLocation` → vale per cilindro e skeletal.
- I **60 test restano verdi**: gli asset non entrano nella logica; il fallback garantisce l'avvio senza asset.
- Percorso didattico completo (import, retarget, AnimBP) senza destabilizzare l'MVP.

**Negative / costi**
- Refactor mirato di `ARTUnit`/`ARTGameMode` (componente opzionale, `VisualZOffset`, spawn via `TSubclassOf`).
- **Identità di team** da ridefinire (il MID `"Color"` non si applica bene a un personaggio) → anello/decal o outline.
- **Coerenza visiva** a rischio con fonti miste (Paragon/Mixamo/MetaHuman).
- Peso repo (Git LFS) e gestione redirector/licenze (vedi `spec-asset-pipeline.md §8-§9`).

**Invarianti**: #1 e #4 **preservati**; split C++/Blueprint rispettato.

## Verifica (alla chiusura dell'implementazione)

- `git`/build: compila senza warning nuovi non spiegati.
- Suite verde: il numero **si misura** col comando in [`../README.md`](../README.md), non si cita — i «60 test»
  scritti qui il 2026-08-03 sono un numero storico, non un gate.
- 1 test C++ di fallback (spawn senza skeletal → unità valida).
- PIE: personaggio visibile e appoggiato a terra; corsa in Move; fallback cilindro se asset assente.
- Licenze registrate (`spec-asset-pipeline.md §8`).

## Revisione

Se l'implementazione mostra che il fallback complica troppo `ARTUnit`, riconsiderare l'alternativa **A** con un
flag di build. Rivedere questo ADR se si adotta il modello dati "ricco" north-star (`piano-canonico-mvp.md §8.1`).
