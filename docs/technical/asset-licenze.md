# Registro di provenienza degli asset

> **Owner del requisito**: [`docs/technical/architecture/spec-asset-pipeline.md`](architecture/spec-asset-pipeline.md) §8 — `FR-ASSET-LIC-01`
> **Regola d'ingresso**: [`docs/technical/tooling/asset-map.md`](tooling/asset-map.md) §6
> **Gate**: `node tools/asset-provenance/check.ts`
> **Nato il** 2026-09-05 con [#1767](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1767)

## 1. Che cos'è, e che cosa non è

Questo file dice, per ogni famiglia di asset che il progetto **usa o spedisce**, da dove viene e sotto
quale licenza. È l'oggetto che `FR-ASSET-LIC-01` prescriveva dal 2026-08-30 e che fino al 2026-09-05
**non esisteva** — quindi il verificatore scritto accanto al requisito non poteva fallire, e un
requisito che non può fallire non è un requisito.

⛔ **Un verde del gate significa «registrato», mai «consentito».** Nessun controllo automatico può
leggere un EULA. `check.ts` verifica che una **riga esista**; se quella riga dice il falso, resta verde.
Chi aggiunge una riga trascrive ciò che la pagina della fonte dichiara — non lo interpreta, e non lo
inventa.

⛔ **Nessuna authority di gameplay.** Una mesh non decide, un materiale non decide, e questo registro
tanto meno: descrive, non abilita.

## 2. Come si legge una riga

| Colonna | Contenuto |
|---|---|
| `Famiglia` | il nome leggibile del pack o del gruppo |
| `Copre` | il **prefisso di path** che la riga copre — una cartella con la barra finale, o un file singolo |
| `Fonte` | chi lo pubblica, con l'URL ufficiale quando lo si conosce |
| `Licenza` | il titolo sotto cui il progetto lo usa |
| `Versione` | la versione o l'identificativo del listing |
| `Acquisito` | quando è entrato nel progetto |
| `UE` | la versione di Unreal Engine al momento dell'acquisizione |
| `Attribuzione` | l'obbligo, non una nota di cortesia |
| `Consumer` | chi lo usa; `nessuno — presente, non cotto` per ciò che sta sul disco senza essere referenziato |

Tre valori hanno un significato fissato:

- **`NON VERIFICATA`** — la licenza **non** è accertata. È un esito **registrato**, non un'omissione:
  l'asset esiste, il progetto lo sa, e il riuso diretto si ferma finché qualcuno non accerta il titolo.
  Il gate resta verde: la riga c'è, ed è ciò che il gate misura.
- **`listing n/d`** — l'identificativo del listing Fab non è stato registrato all'acquisizione, e non
  viene indovinato. La cella `Fonte` di quelle righe **non porta un link**: un link inventato sarebbe
  peggio di un link assente.
- **`nessuno — presente, non cotto`** — il pack sta sul disco ma nessun package versionato lo cita,
  quindi non entra nel cook. È lo stato di 24 dei 28 pack Paragon.

🔴 **Da dove vengono le date.** Per `ParagonProps` e `KiteDemo` la data è quella dichiarata dal report
di acquisizione (2026-09-04). Per i 26 pack personaggio la data d'acquisizione **non era stata
registrata da nessuna parte**: il valore è il `mtime` della cartella root, misurato il 2026-09-05. È una
misura del filesystem, non un atto — e i 26 valori identici al 2026-08-06 sono coerenti con l'unica
importazione in blocco che le fonti descrivono, ma non ne sono la prova. Per il contenuto versionato la
data è quella del primo commit che lo introduce.

🔴 **La colonna `UE` dice `5.8`**, che è ciò che `RefactorTactics.uproject` dichiara come
`EngineAssociation` **da sempre** — dal 2026-08-01, mai cambiato. `CLAUDE.md` pinna la patch `5.8.1`;
la differenza è fra la versione del progetto e la versione dell'installazione, e nessuna delle
acquisizioni qui elencate ha attraversato un cambio di major.

## 3. Il registro

<!-- registro:inizio -->

| Famiglia | Copre | Fonte | Licenza | Versione | Acquisito | UE | Attribuzione | Consumer |
|---|---|---|---|---|---|---|---|---|
| RefactorTactics — contenuto proprietario | `Content/RT/` | prodotto dal progetto | Proprietaria — RefactorTactics | n/a | 2026-08-05 | 5.8 | n/a | il gioco |
| Set iconografico HUD | `Content/Icons/` | [tools/hud-assets/generate_hud_assets.py](../../tools/hud-assets/generate_hud_assets.py) | Proprietaria — RefactorTactics, generato | rigenerabile | 2026-08-27 | 5.8 | n/a | `URTIconLibrary` |
| UI kit estratto da uno screenshot di concept | `Content/RT_UI_AssetPack_FromHUD/` | screenshot di concept, non tracciato | NON VERIFICATA | n/d | 2026-08-08 | 5.8 | ignota | nessuno — prototipo, non cotto |
| Icone abilità Paragon — archivio di fan | `tools/icons-downloader/Paragon_Skill_Icons/` | [Paragon Archive](https://paragon-archive.fandom.com/wiki/Category:Abilities) e [ParagoneAPI](https://github.com/alex-taxiera/ParagoneAPI) | NON VERIFICATA | n/d | 2026-08-16 | 5.8 | ignota | nessuno — materiale di studio |
| Downloader icone Paragon | `tools/Paragon_Skill_Icons_Downloader.zip` | prodotto dal progetto | Proprietaria — RefactorTactics | n/a | 2026-08-16 | 5.8 | n/a | strumento |
| Paragon: Agora e Monolith — Kite Demo | `/Game/FabAsset/Paragon/KiteDemo/` | [Fab · Epic Games](https://www.fab.com/listings/6f401fb5-88b5-41b4-bf1b-62321414e1f0) | Fab Standard License | listing `6f401fb5` | 2026-09-04 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Boris | `/Game/FabAsset/Paragon/ParagonBoris/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Dekker | `/Game/FabAsset/Paragon/ParagonDekker/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Drongo | `/Game/FabAsset/Paragon/ParagonDrongo/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Fey | `/Game/FabAsset/Paragon/ParagonFey/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Gadget | `/Game/FabAsset/Paragon/ParagonGadget/` | [Fab · Epic Games](https://www.fab.com/listings/52a621fd-28bb-4898-a2e7-93229a40e3f4) | Fab Standard License | listing `52a621fd` | 2026-08-11 | 5.8 | nessuna — ⛔ NoAI §5 | `BP_Unit_Gadget` |
| Paragon: Gideon | `/Game/FabAsset/Paragon/ParagonGideon/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Greystone | `/Game/FabAsset/Paragon/ParagonGreystone/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Grux | `/Game/FabAsset/Paragon/ParagonGrux/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Khaimera | `/Game/FabAsset/Paragon/ParagonKhaimera/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Morigesh | `/Game/FabAsset/Paragon/ParagonMorigesh/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Murdock | `/Game/FabAsset/Paragon/ParagonMurdock/` | [Fab · Epic Games](https://www.fab.com/listings/89ae5876-3350-432e-882b-7d617e8c7e6f) | Fab Standard License | listing `89ae5876` | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Muriel | `/Game/FabAsset/Paragon/ParagonMuriel/` | [Fab · Epic Games](https://www.fab.com/listings/c16f2277-675d-4366-a7e6-cf3b5085f936) | Fab Standard License | listing `c16f2277` | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Narbash | `/Game/FabAsset/Paragon/ParagonNarbash/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Phase | `/Game/FabAsset/Paragon/ParagonPhase/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | `BP_Unit_Phase` |
| Paragon: Agora e Monolith — Props | `/Game/FabAsset/Paragon/ParagonProps/` | [Fab · Epic Games](https://www.fab.com/listings/6f401fb5-88b5-41b4-bf1b-62321414e1f0) | Fab Standard License | listing `6f401fb5` | 2026-09-04 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Revenant | `/Game/FabAsset/Paragon/ParagonRevenant/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Riktor | `/Game/FabAsset/Paragon/ParagonRiktor/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | `BP_Unit_Riktor` |
| Paragon: Serath | `/Game/FabAsset/Paragon/ParagonSerath/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Sevarog | `/Game/FabAsset/Paragon/ParagonSevarog/` | [Fab · Epic Games](https://www.fab.com/listings/a4882b5e-cfad-4830-a3dd-46a6c31a79b2) | Fab Standard License | listing `a4882b5e` | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Sparrow | `/Game/FabAsset/Paragon/ParagonSparrow/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Steel | `/Game/FabAsset/Paragon/ParagonSteel/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: SunWukong | `/Game/FabAsset/Paragon/ParagonSunWukong/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Terra | `/Game/FabAsset/Paragon/ParagonTerra/` | [Fab · Epic Games](https://www.fab.com/listings/5ea6bcb6-e43e-4bbe-813f-c19d8c907565) | Fab Standard License | listing `5ea6bcb6` | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Twinblast | `/Game/FabAsset/Paragon/ParagonTwinblast/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Wraith | `/Game/FabAsset/Paragon/ParagonWraith/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | `BP_Unit_Wraith` |
| Paragon: Yin | `/Game/FabAsset/Paragon/ParagonYin/` | Fab · Epic Games — listing n/d | Fab Standard License | listing n/d | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |
| Paragon: Zinx | `/Game/FabAsset/Paragon/ParagonZinx/` | [Fab · Epic Games](https://www.fab.com/listings/15e6593c-3696-4f87-99ef-fcec9e3138fd) | Fab Standard License | listing `15e6593c` | 2026-08-06 | 5.8 | nessuna — ⛔ NoAI §5 | nessuno — presente, non cotto |

<!-- registro:fine -->

I due marcatori HTML **delimitano ciò che il gate legge**. Una tabella scritta fuori da lì è prosa;
una riga scritta dentro è un impegno. Non spostarli, e non aggiungerne una seconda coppia.

## 4. Come si aggiunge una riga

**La riga entra nello stesso commit dell'asset.** Non il commit dopo, non «quando avremo tempo»: fra
i due momenti l'asset è materiale di terze parti senza titolo dichiarato, e in quella finestra non c'è
niente che lo dica.

È la quarta riga della procedura di [`docs/technical/tooling/asset-map.md`](tooling/asset-map.md) §6,
accanto a `.gitignore`, `editor-sessions.yaml` e la mappa stessa.

Se la licenza non è accertata, la riga si scrive lo stesso con `NON VERIFICATA`. **Omettere la riga è
sempre peggio che scrivere che non si sa**: la prima forma è invisibile, la seconda è cercabile.

Prima di committare:

```powershell
node tools/asset-provenance/check.ts
```

## 5. ⛔ La clausola NoAI dei listing Paragon

I listing Fab dei pack Paragon dichiarano `allows_usage_with_ai: false`. Non è una cautela
redazionale: è un **obbligo derivato dalla licenza**, e vale per ogni sessione assistita che tocchi
questo progetto.

Un assistente **può** ricevere metadati:

`asset_name` · `object_path` · `package_path` · `asset_class` · `dependency_metadata` ·
`human_approved_object_paths`

Un assistente **non deve** ricevere il contenuto:

pixel di texture · thumbnail · screenshot che contengono asset Paragon · geometria delle mesh ·
preview render · immagini di preview dei materiali · dati audio · forme d'onda · sorgenti esportate

🔴 **La clausola ha già un effetto operativo misurato**, non è teorica: la selezione dei candidati
Paragon si è fermata su **211 candidati su 211** in stato «da approvare da una persona», zero
approvati, perché scegliere richiede di **guardare** gli asset — che è esattamente ciò che la clausola
impedisce a un assistente. L'approvazione visiva resta un atto umano.

Questa riga vive qui, e non solo in un JSON fuori dal versionamento, perché un obbligo che sparisce
insieme a una cartella non è un obbligo.

## 6. Quello che questo registro non sa

- **Se una licenza è rispettata.** Sa che è stata dichiarata. Il resto è un atto umano.
- **Che cosa c'è dentro un pack.** Le righe descrivono famiglie, non file. Sotto
  `/Game/FabAsset/Paragon/` ci sono 37.482 `.uasset`, e il registro ne conosce i 28 root.
- **Che cosa il cook porta con sé.** Il cook segue le dipendenze, non le cartelle: 12 riferimenti dai
  quattro `BP_Unit_*` hanno portato **756 asset Paragon** dentro un pacchetto
  ([#1663](https://github.com/DegrassiAaron/refactor-tactics-main/issues/1663)). Sapere **quali** è il
  lavoro di [#803](https://github.com/DegrassiAaron/refactor-tactics-main/issues/803) e
  [#815](https://github.com/DegrassiAaron/refactor-tactics-main/issues/815); questo registro è metà di
  quell'elenco, non l'elenco.
- **Gli asset che nessuno ha ancora scaricato.** Copre ciò che è presente o referenziato, non ciò che
  è stato valutato.
- **Le `n/d`.** Sono buchi veri, scritti come tali. Il modo di chiuderle è ritrovare il listing, non
  dedurlo dal nome del pack.
