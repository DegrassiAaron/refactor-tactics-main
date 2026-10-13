# Il glossario di stati, superfici e ripari — piano di implementazione

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** ogni stato registrato, ogni superficie e ogni tipo di riparo ha un nome leggibile e un effetto in una riga, in una tabella C++ chiavata per id stabile, con un documento gemello che l'autore rilegge e due gate che li tengono onesti.

**Architecture:** una tabella sola in `UI/RTGlossary.{h,cpp}`, nella forma di `Ability/RTActionDescriptions`: due elenchi `{ id, testo }` — nomi ed effetti — con le stesse chiavi, più tre funzioni che danno la chiave di uno stato, di una superficie, di un riparo. Le chiavi sono gli id che il gioco scrive già (`Status.Burning`, `Terrain.Fire`). Un gate Automation confronta la tabella con ciò che esiste nel gioco, nei due versi; un radar confronta la tabella col documento, riusando il parser delle frasi d'azione.

**Tech Stack:** Unreal Engine 5.8.1, C++ (modulo `RefactorTactics`), Automation Test framework; TypeScript eseguito da `node` 22 senza build (`tools/radar/`), `node --test`.

**Spec:** [`docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`](../specs/2026-10-13-ispettore-del-mondo-design.md), fetta `F1` di §9.1 e paragrafo «Il glossario» di §4. Le decisioni `S1`–`S7`, i `Ruling` e la revisione `V1`–`V7` sono chiusi: non si riaprono qui. Issue di lavoro: [#3744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3744), fetta di [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705).

**Stato misurato in pianificazione:** checkout `D:/Repositories/rt-wt-ispettore`, branch `issue/3744-glossario-stati-superfici-ripari`, da `origin/main` = `4e6d198cd`. Ogni regola citata nelle frasi del Task 1 è stata letta sul codice di quel commit; la tabella «Da dove viene ogni frase» dice dove.

## Global Constraints

- Engine: **UE 5.8.1** in `D:/EpicGames/UE_5.8`, *installed build*. Nessun aggiornamento di Engine, plugin o dipendenze.
- ⛔ **Nessun `.uasset`, nessun Blueprint, nessuna UI.** La fetta è una tabella, un documento e due gate.
- ⛔ **Nessun lettore in partita, ed è dichiarato.** Fino alla fetta `F2` della spec `RTGlossary` è una sorgente senza lettore: lo dicono l'header, il documento e `docs/balance/README.md`. Non si aggiunge un lettore «per non lasciarla inerte».
- ⛔ **Il testo del registro non cambia.** `URTHudViewModel::ComposePlayerEventText` non si tocca.
- ⛔ **Nessuna `UFUNCTION` e nessuna `UPROPERTY` nuova**: `RTGlossary` è un namespace C++, come `RTActionDescriptions`.
- **Le chiavi sono id che esistono già**: per uno stato il nome del tag (`Status.Burning`); per una superficie `URTTurnLogLibrary::TerrainCausePrefix()` più il nome del valore di `ERTHexSurface` (`Terrain.Fire`), la stessa composizione che `ARTTurnManager` scrive nel TurnLog; per un riparo `Structure.LowCover` e `Structure.HighCover`, gli id di `docs/balance/RT_TerrainCatalog_v0.1.md` §3.
- **Regole dei testi**, verificate dal gate:
  - solo ASCII stampabile: l'accento si scrive con l'apostrofo (`piu'`, `e'`), come nelle frasi d'azione;
  - nessuna cifra: un numero invecchia al primo ribilanciamento;
  - nessun nome di tag (`Status.`, `Terrain.`, `Structure.`);
  - un **nome** non finisce col punto; un **effetto** sì.
- ⛔ **Nessun totale volatile** in commenti, documenti, issue, PR (`AGENTS.md` §14): niente «N stati». Dove serve un elenco ci sono i nomi; dove serve una misura c'è il comando.
- Stile: commenti in **italiano**, nella forma dei file accanto. Test nella famiglia esistente `RefactorTactics.UI.*`, con prefisso `Glossary`.
- **Unity build**: i nomi dentro un namespace anonimo sono distinti per file. Qui portano il prefisso `Glossary`.
- Commit: `<type>(3744): <descrizione>`, chiuso da `Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>`. Un commit per task. Si aggiungono al commit i soli file del task, per percorso: mai `git add -A`.
- `<checkout>` = `D:/Repositories/rt-wt-ispettore`. `<scratchpad>` = la cartella scratchpad della sessione. ⛔ **Ogni comando git porta `-C <checkout>`** o gira con la cwd lì: il clone principale `D:/Repositories/refactor-tactics-main` è di un'altra sessione e non si tocca.
- **Motore uno per macchina** (`CLAUDE.md` §10, `AGENTS.md` §11). Prima di ogni build o run:
  ```powershell
  Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine
  ```
  Si aspetta se qualcosa gira su `<checkout>`, o se un Editor interattivo è aperto in qualunque clone. Non si aspetta per una build in un altro clone. ➕ rev2. Per una suite headless altrui la domanda non è che cosa misuro io, è che cosa misura **lei**: si legge la sua `CommandLine`, e se è una serie ripetuta o una misura che vive nei tempi si aspetta, oppure si dichiara `NOT RUN` col nome del clone.
- Build, con l'Editor chiuso su `<checkout>`:
  ```powershell
  & "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="D:/Repositories/rt-wt-ispettore/RefactorTactics.uproject" -WaitMutex
  ```
  ⚠️ Build e test **in due comandi separati**: se la build fallisce, i test girerebbero sui binari vecchi. L'esito della build si legge dalla riga `Result:` dell'output, non dall'assenza di errori.
- Test, dalla cwd `<checkout>`:
  ```powershell
  python tools/suite/esegui.py RefactorTactics.UI.Glossary --log "<scratchpad>/<nome-parlante>.log" -v
  ```
  Lo strumento attende sul log e stampa il referto. Un esito si legge dai conteggi del referto, e i nomi dei test trovati si controllano: un filtro sbagliato non seleziona nulla e non lo dice.
- Scritture su GitHub: `export GH_TOKEN="$(gh auth token --user DegrassiAaron)"` prima di ogni `gh` che scrive, poi `unset GH_TOKEN`. Il corpo passa da `--body-file`, mai inline.

## Review Focus

Che cosa la spec implica e che può mordere chi userà questa tabella, dal più probabile:

1. **Nasce uno stato nuovo e nessuno gli scrive il testo.** Atteso: il gate diventa rosso il giorno in cui il tag viene registrato. Lo pinna `GlossaryCoversEveryStatusSurfaceAndCover` (Task 1), e il Task 2 prova che sa fallire.
2. **L'autore corregge una frase nel documento e non nella tabella.** Atteso: il radar esce `1` e stampa i due valori. Lo pinnano `glossary.test.ts` e il controllo positivo del Task 3.
3. **Qualcuno chiede una chiave che non c'è**: un tag non valido, `ERTHexCoverType::None`, un `FName` qualunque. Atteso: testo vuoto, mai un crash. Lo pinna `GlossaryKeysAreTheIdsTheGameWrites` (Task 1).
4. **Il parser del radar smette di trovare la tabella** perché una funzione è stata rinominata. Atteso: errore, non due insiemi vuoti che concordano. Lo pinna `glossary.test.ts` (Task 3).
5. **Un commento nomina `RTGlossary::Effects()` prima della sua definizione**, e il radar legge la tabella sbagliata. Atteso: l'àncora del radar corrisponde solo alla definizione. Lo pinna `glossary.test.ts` (Task 3).

---

### Task 0: Issue, branch, spec e piano nel repository

Già fatto in pianificazione, e qui resta la traccia:

- la issue [#3744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3744) è aperta, firmata `DegrassiAaron`, con le etichette `enhancement` e `P2`, senza milestone: lo assegna l'autore;
- il branch `issue/3744-glossario-stati-superfici-ripari` porta la spec (`8daa9fe8f`) e la sua revisione (`f03f0638f`).

- [ ] **Step 1: Committa questo piano**

```bash
git -C D:/Repositories/rt-wt-ispettore add -- docs/superpowers/plans/2026-10-13-glossario-stati-superfici-ripari.md
git -C D:/Repositories/rt-wt-ispettore commit -F - <<'EOF'
docs(3744): piano di implementazione del glossario di stati, superfici e ripari

Fetta F1 della spec dell'ispettore del mondo.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 1: La tabella, le chiavi e il gate

**Files:**
- Create: `Source/RefactorTactics/UI/RTGlossary.h`
- Create: `Source/RefactorTactics/UI/RTGlossary.cpp`
- Test: `Source/RefactorTactics/Tests/RTGlossaryTests.cpp`

**Interfaces:**
- Consumes: `URTTurnLogLibrary::TerrainCausePrefix()` (`Turn/RTTurnLogLibrary.h`); `ERTHexSurface`, `ERTHexCoverType` (`Map/RTHexCellData.h`); i tag `TAG_Status_*` (`Core/RTGameplayTags.h`).
- Produces, per le fette `F2` e `F3` e per il radar del Task 3:
  - `FText RTGlossary::NameFor(const FName& Key)`
  - `FText RTGlossary::EffectFor(const FName& Key)`
  - `FName RTGlossary::KeyForStatus(const FGameplayTag& Tag)`
  - `FName RTGlossary::KeyForSurface(ERTHexSurface Surface)`
  - `FName RTGlossary::KeyForCover(ERTHexCoverType Type)`
  - `const TArray<TPair<FName, FString>>& RTGlossary::Names()`
  - `const TArray<TPair<FName, FString>>& RTGlossary::Effects()`

**Da dove viene ogni frase.** Le frasi le scrive chi implementa, dalle regole nel codice, e restano da accettare dall'autore. Chi le cambia rilegge la regola, non la frase.

| Voce | Regola letta in |
|---|---|
| `Status.Root` | `URTCombatLibrary::EffectiveMoveRange`; `ARTUnit::GetEffectiveDashRange` |
| `Status.Slow` | `ARTTurnManager`, `SimUnit.MoveCostModifier`; il commento di `GetEffectiveDashRange`: lo scatto non ne risente |
| `Status.Reveal` | `URTCombatLibrary::IsIntentVisibleTo`, chiamata da `URTIntentPrivacyLibrary` |
| `Status.Exposed` | `ARTTurnManager`, `FirstHitDelta` con `ExposedFirstHitBonus` |
| `Status.Guarded` | `ARTTurnManager`, `GuardReductionByTarget` sull'arco frontale; `GuardResistedPushDistance` |
| `Status.Marked` | `ARTTurnManager`: il bonus vale solo se `TeamId` di chi colpisce è `GetMarkedByTeam()`, poi `RemoveStatus` |
| `Status.Wet` | `ARTUnit::ApplyStatus` toglie `Status.Burning`; `AevikWetDischargeBonus`, che il suo commento dichiara di **una** abilità e non una regola universale |
| `Status.Braced` | `ARTTurnManager`, `EveryHitDelta` con `BraceDamageReduction`; il ramo `Braced` dello spostamento forzato |
| `Status.Burning` | `BurningCleanupDamage`; `docs/balance/RT_TerrainCatalog_v0.1.md` §2 |
| `Status.Obscured` | `FRTTerrainDef::MaxTargetingRangeThrough` del fumo |
| `Status.Electrified` | `ARTTurnManager`, `MakeStatusInstantEntry` sui colpi di `CollectElectricPropagation`, compreso il passo zero: il bersaglio diretto |
| `Status.Unbalanced` | `ARTUnit::PlannedDashDeniedByStatus`: nega la corsa, non lo scatto lineare; `UnbalancedExtraDisplacement`, per spinta e trazione; `CadeSeSbilanciato`; i rami `Guarded` e `Braced` dello spostamento forzato, che non valgono per chi è sbilanciato |
| `Status.Prone` | `StandUpMovePointCost`; `ReactionBlockedThisTurn` |
| `Status.Stunned` | `ARTTurnManager::RefuseMainActionIfStunned`; `ReactionBlockedThisTurn` |
| superfici | `URTTerrainLibrary::GetTerrainCatalog` e i commenti dei campi di `FRTTerrainDef`: `SlideCells` vale per chi **termina** il movimento, `bBlocksDashCharge` per chi **attraversa**, `bFlammable` solo per pavimento e accidentato. Per il ghiaccio anche `URTHexSimLibrary::ApplyIceSliding`, con la soglia di budget. `Void` non è a catalogo: `ARTTurnManager` e `URTHexMapAsset` lo escludono solo come cella d'atterraggio |
| ripari | il commento di `ERTHexCoverType` e di `FRTHexCover` (un colpo fuori dall'arco frontale annulla la riduzione); `URTHexCoverLibrary::BlocksTraversal`; il commento sulla copertura in `URTCombatLibrary`: non riduce le aree né il danno ambientale |

- [ ] **Step 1: Controlla il motore**

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine
```

Atteso: nessun processo il cui `CommandLine` nomini `rt-wt-ispettore`, e nessun `UnrealEditor.exe` interattivo. Se c'è, si aspetta.

- [ ] **Step 2: Scrivi il test, che non compila ancora**

`Source/RefactorTactics/Tests/RTGlossaryTests.cpp`:

```cpp
// Il glossario di stati, superfici e ripari (`#3744`): ogni cosa che il gioco puo' nominare ha un nome e un effetto,
// e ogni voce nomina qualcosa che esiste.

#include "Misc/AutomationTest.h"
#include "Core/RTGameplayTags.h"
#include "GameplayTagsManager.h"
#include "Map/RTHexCellData.h"
#include "UI/RTGlossary.h"
#include "UObject/Class.h" // UEnum: il file deve compilare anche fuori dal blob unity

#if WITH_DEV_AUTOMATION_TESTS

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */

	/** Le chiavi degli stati: ogni tag registrato sotto `Status.`, come li enumera `URTIconLibrary`. */
	TArray<FName> GlossaryStatusKeys()
	{
		TArray<FName> Out;
		const FGameplayTag StatusRoot = FGameplayTag::RequestGameplayTag(FName(TEXT("Status")), /*ErrorIfNotFound*/ false);
		if (StatusRoot.IsValid())
		{
			TArray<FGameplayTag> Tags;
			UGameplayTagsManager::Get().RequestGameplayTagChildren(StatusRoot).GetGameplayTagArray(Tags);
			for (const FGameplayTag& Tag : Tags)
			{
				Out.Add(RTGlossary::KeyForStatus(Tag));
			}
		}
		return Out;
	}

	/** Le chiavi delle superfici: ogni valore di `ERTHexSurface`. */
	TArray<FName> GlossarySurfaceKeys()
	{
		TArray<FName> Out;
		const UEnum* Enum = StaticEnum<ERTHexSurface>();
		// `NumEnums() - 1`: l'ultimo e' il `_MAX` sintetico che UHT aggiunge.
		for (int32 I = 0; I < Enum->NumEnums() - 1; ++I)
		{
			Out.Add(RTGlossary::KeyForSurface(static_cast<ERTHexSurface>(Enum->GetValueByIndex(I))));
		}
		return Out;
	}

	/** Le chiavi dei ripari: ogni valore di `ERTHexCoverType` tranne `None`, che non e' un riparo. */
	TArray<FName> GlossaryCoverKeys()
	{
		TArray<FName> Out;
		const UEnum* Enum = StaticEnum<ERTHexCoverType>();
		for (int32 I = 0; I < Enum->NumEnums() - 1; ++I)
		{
			const ERTHexCoverType Type = static_cast<ERTHexCoverType>(Enum->GetValueByIndex(I));
			if (Type != ERTHexCoverType::None)
			{
				Out.Add(RTGlossary::KeyForCover(Type));
			}
		}
		return Out;
	}

	/** Le regole di un testo: non vuoto, solo ASCII stampabile, nessuna cifra, nessun nome di tag. */
	void GlossaryCheckText(FAutomationTestBase& Test, const FString& Key, const TCHAR* Quale, const FString& Testo)
	{
		Test.TestFalse(*FString::Printf(TEXT("%s (%s): non e' vuoto"), *Key, Quale), Testo.TrimStartAndEnd().IsEmpty());
		bool bSoloAscii = true;
		bool bSenzaCifre = true;
		// Per indice e non con un `for` a intervallo: qui conta ogni carattere della stringa e nessun terminatore.
		for (int32 I = 0; I < Testo.Len(); ++I)
		{
			const TCHAR C = Testo[I];
			bSoloAscii = bSoloAscii && C >= 0x20 && C <= 0x7E;
			bSenzaCifre = bSenzaCifre && !FChar::IsDigit(C);
		}
		Test.TestTrue(*FString::Printf(TEXT("%s (%s): solo ASCII, con l'apostrofo al posto dell'accento"), *Key, Quale), bSoloAscii);
		Test.TestTrue(*FString::Printf(TEXT("%s (%s): non porta cifre"), *Key, Quale), bSenzaCifre);
		for (const TCHAR* Prefisso : { TEXT("Status."), TEXT("Terrain."), TEXT("Structure.") })
		{
			Test.TestFalse(*FString::Printf(TEXT("%s (%s): non nomina un tag (%s)"), *Key, Quale, Prefisso),
				Testo.Contains(Prefisso, ESearchCase::IgnoreCase));
		}
	}
}

/**
 * OGNI STATO, OGNI SUPERFICIE E OGNI RIPARO HA UNA VOCE - `#3744`.
 *
 * 🔑 **Le tre sorgenti sono quelle del gioco, non un elenco scritto qui**: i tag registrati sotto `Status.` e i
 * valori dei due enum. Uno stato nuovo senza testo fa cadere questo test il giorno in cui viene registrato.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlossaryCoversEverythingTest,
	"RefactorTactics.UI.GlossaryCoversEveryStatusSurfaceAndCover",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlossaryCoversEverythingTest::RunTest(const FString&)
{
	const TArray<FName> Stati = GlossaryStatusKeys();
	const TArray<FName> Superfici = GlossarySurfaceKeys();
	const TArray<FName> Ripari = GlossaryCoverKeys();

	// ⚠️ Le premesse: un insieme vuoto renderebbe i cicli qui sotto un verde vuoto. Tre variabili e non un
	// `&&`: si valutano tutte, e il log le nomina tutte.
	const bool bStati = TestTrue(TEXT("premessa: ci sono stati registrati sotto `Status.`"), Stati.Num() > 0);
	const bool bSuperfici = TestTrue(TEXT("premessa: `ERTHexSurface` ha dei valori"), Superfici.Num() > 0);
	const bool bRipari = TestTrue(TEXT("premessa: c'e' almeno un tipo di riparo diverso da `None`"), Ripari.Num() > 0);
	if (!bStati || !bSuperfici || !bRipari)
	{
		return false;
	}
	// Controllo positivo: l'enumerazione vede uno stato che il gioco applica di sicuro.
	TestTrue(TEXT("premessa: fra gli stati c'e' quello del fuoco"),
		Stati.Contains(TAG_Status_Burning.GetTag().GetTagName()));

	TArray<FName> Tutte;
	Tutte.Append(Stati);
	Tutte.Append(Superfici);
	Tutte.Append(Ripari);
	for (const FName& Key : Tutte)
	{
		TestFalse(*FString::Printf(TEXT("%s ha un nome"), *Key.ToString()),
			RTGlossary::NameFor(Key).IsEmptyOrWhitespace());
		TestFalse(*FString::Printf(TEXT("%s ha un effetto"), *Key.ToString()),
			RTGlossary::EffectFor(Key).IsEmptyOrWhitespace());
	}
	return true;
}

/**
 * OGNI VOCE NOMINA QUALCOSA CHE ESISTE, UNA VOLTA SOLA, E RISPETTA LE REGOLE DEI TESTI - `#3744`.
 *
 * 🔑 **E' il verso opposto del test qui sopra.** Senza di lui una voce resta nella tabella dopo che il suo stato
 * e' stato rinominato, e il documento la ripete come se valesse.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlossaryEntriesExistTest,
	"RefactorTactics.UI.GlossaryEntriesNameThingsThatExist",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlossaryEntriesExistTest::RunTest(const FString&)
{
	const TArray<TPair<FName, FString>>& Nomi = RTGlossary::Names();
	const TArray<TPair<FName, FString>>& Effetti = RTGlossary::Effects();
	if (!TestTrue(TEXT("premessa: il glossario ha delle voci"), Nomi.Num() > 0))
	{
		return false;
	}
	TestEqual(TEXT("nomi ed effetti hanno lo stesso numero di voci"), Nomi.Num(), Effetti.Num());

	TSet<FName> Attese;
	Attese.Append(GlossaryStatusKeys());
	Attese.Append(GlossarySurfaceKeys());
	Attese.Append(GlossaryCoverKeys());

	TSet<FName> Viste;
	for (int32 I = 0; I < Nomi.Num(); ++I)
	{
		const FName Key = Nomi[I].Key;
		const FString K = Key.ToString();
		TestTrue(*FString::Printf(TEXT("%s nomina uno stato, una superficie o un riparo che esiste"), *K),
			Attese.Contains(Key));
		TestFalse(*FString::Printf(TEXT("%s compare una volta sola"), *K), Viste.Contains(Key));
		Viste.Add(Key);

		GlossaryCheckText(*this, K, TEXT("nome"), Nomi[I].Value);
		TestFalse(*FString::Printf(TEXT("%s (nome): non finisce col punto"), *K), Nomi[I].Value.EndsWith(TEXT(".")));

		if (!Effetti.IsValidIndex(I))
		{
			continue;
		}
		// Le due tabelle hanno le stesse chiavi nello stesso ordine: e' cio' che le rende rileggibili affiancate.
		TestEqual(*FString::Printf(TEXT("la voce %d ha la stessa chiave nelle due tabelle"), I), Effetti[I].Key, Key);
		GlossaryCheckText(*this, K, TEXT("effetto"), Effetti[I].Value);
		TestTrue(*FString::Printf(TEXT("%s (effetto): finisce col punto"), *K), Effetti[I].Value.EndsWith(TEXT(".")));
	}
	return true;
}

/**
 * LE CHIAVI SONO GLI ID CHE IL GIOCO SCRIVE GIA', E UNA CHIAVE CHE NON C'E' DA' UN TESTO VUOTO - `#3744`.
 *
 * 🔑 **Valori letterali, e non una seconda composizione**: se `KeyForSurface` cambiasse forma, il test qui sopra
 * continuerebbe a concordare con lei. `Status.Burning` e `Terrain.Fire` sono cio' che il TurnLog scrive come causa
 * di un danno; gli altri sono gli id del catalogo dei terreni, dove una superficie che non fa danno non arriva
 * mai al TurnLog. Il catalogo non elenca `Terrain.Void`: quella chiave segue solo lo schema.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGlossaryKeysTest,
	"RefactorTactics.UI.GlossaryKeysAreTheIdsTheGameWrites",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGlossaryKeysTest::RunTest(const FString&)
{
	TestEqual(TEXT("uno stato ha per chiave il nome del suo tag"),
		RTGlossary::KeyForStatus(TAG_Status_Burning), FName(TEXT("Status.Burning")));
	TestEqual(TEXT("una superficie ha per chiave la causa che il TurnLog scrive"),
		RTGlossary::KeyForSurface(ERTHexSurface::Fire), FName(TEXT("Terrain.Fire")));
	TestEqual(TEXT("il nome del valore entra intero nella chiave"),
		RTGlossary::KeyForSurface(ERTHexSurface::ShallowWater), FName(TEXT("Terrain.ShallowWater")));
	TestEqual(TEXT("il riparo basso ha l'id del catalogo dei terreni"),
		RTGlossary::KeyForCover(ERTHexCoverType::Low), FName(TEXT("Structure.LowCover")));
	TestEqual(TEXT("il riparo alto ha l'id del catalogo dei terreni"),
		RTGlossary::KeyForCover(ERTHexCoverType::High), FName(TEXT("Structure.HighCover")));

	// Cio' che non ha una voce: nessuna chiave, e un testo vuoto invece di un crash.
	TestTrue(TEXT("`None` non e' un riparo: nessuna chiave"), RTGlossary::KeyForCover(ERTHexCoverType::None).IsNone());
	TestTrue(TEXT("un tag non valido non ha chiave"), RTGlossary::KeyForStatus(FGameplayTag()).IsNone());
	TestTrue(TEXT("una chiave vuota non ha nome"), RTGlossary::NameFor(NAME_None).IsEmpty());
	TestTrue(TEXT("una chiave sconosciuta non ha nome"), RTGlossary::NameFor(TEXT("Status.Inesistente")).IsEmpty());
	TestTrue(TEXT("una chiave sconosciuta non ha effetto"), RTGlossary::EffectFor(TEXT("Status.Inesistente")).IsEmpty());
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
```

- [ ] **Step 3: Scrivi l'header e uno scheletro che compila e non risponde**

`Source/RefactorTactics/UI/RTGlossary.h`:

```cpp
#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "Map/RTHexCellData.h" // ERTHexSurface, ERTHexCoverType

/**
 * IL GLOSSARIO: il nome e l'effetto in una riga di uno stato, di una superficie, di un riparo (`#3744`).
 *
 * 🔑 **Una tabella sola, chiavata per id stabile**, nella forma di `RTActionDescriptions`. Le chiavi sono id che
 * esistono gia': il nome del tag per uno stato (`Status.Burning`); per una superficie la causa che il TurnLog
 * scrive quando fa danno (`Terrain.Fire`), che per le altre e' l'id del catalogo dei terreni — tranne
 * `Terrain.Void`, che il catalogo non elenca; l'id dello stesso catalogo per un riparo (`Structure.LowCover`),
 * che il gioco non scrive da nessuna parte.
 *
 * ⛔ **Un testo non porta numeri ne' nomi di tag.** Durata, danno e costo li compone il gioco dai dati: un numero
 * scritto qui invecchierebbe al primo ribilanciamento, e nessun gate se ne accorgerebbe.
 *
 * ⚠️ **Oggi nessuno la legge in partita, ed e' dichiarato**: i lettori sono lo sguardo e la scheda dell'ispettore
 * del mondo (`docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md`, fette F2 e F3). Chi la trova
 * inerte non ha trovato un difetto.
 *
 * 📄 Le stesse voci stanno in `docs/balance/RT_Glossary_v0.1.md`, che e' cio' che l'autore rilegge.
 * `tools/radar/glossary.ts --check` confronta le due copie, e non dice quale correggere. Che ogni stato, ogni
 * superficie e ogni riparo abbia una voce lo verificano i test `RefactorTactics.UI.Glossary*`.
 */
namespace RTGlossary
{
	/** Il nome da leggere di `Key`, oppure vuoto: una chiave senza voce non ha un nome da mostrare. */
	FText NameFor(const FName& Key);

	/** L'effetto in una riga di `Key`, oppure vuoto. */
	FText EffectFor(const FName& Key);

	/** La chiave di uno stato: il nome del suo tag. `None` per un tag non valido. */
	FName KeyForStatus(const FGameplayTag& Tag);

	/** La chiave di una superficie: il prefisso delle cause di terreno del TurnLog piu' il nome del valore. */
	FName KeyForSurface(ERTHexSurface Surface);

	/** La chiave di un tipo di riparo. `None` per `ERTHexCoverType::None`, che non e' un riparo. */
	FName KeyForCover(ERTHexCoverType Type);

	/** I nomi: li leggono il gate e il radar. */
	const TArray<TPair<FName, FString>>& Names();

	/** Gli effetti, con le stesse chiavi nello stesso ordine dei nomi: lo verifica il gate. */
	const TArray<TPair<FName, FString>>& Effects();
}
```

`Source/RefactorTactics/UI/RTGlossary.cpp`, scheletro:

```cpp
#include "UI/RTGlossary.h"

const TArray<TPair<FName, FString>>& RTGlossary::Names()
{
	static const TArray<TPair<FName, FString>> Voci;
	return Voci;
}

const TArray<TPair<FName, FString>>& RTGlossary::Effects()
{
	static const TArray<TPair<FName, FString>> Voci;
	return Voci;
}

FText RTGlossary::NameFor(const FName& Key) { return FText::GetEmpty(); }
FText RTGlossary::EffectFor(const FName& Key) { return FText::GetEmpty(); }
FName RTGlossary::KeyForStatus(const FGameplayTag& Tag) { return NAME_None; }
FName RTGlossary::KeyForSurface(ERTHexSurface Surface) { return NAME_None; }
FName RTGlossary::KeyForCover(ERTHexCoverType Type) { return NAME_None; }
```

- [ ] **Step 4: Compila**

```powershell
& "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="D:/Repositories/rt-wt-ispettore/RefactorTactics.uproject" -WaitMutex
```

Atteso: `Result: Succeeded`. È la prima build di questo checkout: compila tutto il modulo, e dura.

- [ ] **Step 5: Lancia i test e guardali fallire**

```powershell
python tools/suite/esegui.py RefactorTactics.UI.Glossary --log "<scratchpad>/glossario-rosso.log" -v
```

Atteso: tre test trovati, tre rossi. Fra i messaggi di ciascuno:
- `GlossaryCoversEveryStatusSurfaceAndCover`: «premessa: fra gli stati c'e' quello del fuoco», perché lo scheletro dà `None` come chiave a tutto;
- `GlossaryEntriesNameThingsThatExist`: «premessa: il glossario ha delle voci»;
- `GlossaryKeysAreTheIdsTheGameWrites`: «uno stato ha per chiave il nome del suo tag».

Se i test trovati non sono tre, il filtro o la registrazione sono sbagliati: non si prosegue.

- [ ] **Step 6: Scrivi la tabella e le chiavi**

Sostituisci per intero `Source/RefactorTactics/UI/RTGlossary.cpp`:

```cpp
#include "UI/RTGlossary.h"
#include "Turn/RTTurnLogLibrary.h" // il prefisso delle cause di terreno: lo stesso che il TurnLog scrive
#include "UObject/Class.h"         // UEnum: il file deve compilare anche fuori dal blob unity

// Le voci della v0.1 (`#3744`), scritte da chi implementa a partire dalle regole nel codice e da accettare
// dall'autore. L'ordine e' quello di `docs/balance/RT_Glossary_v0.1.md`: stati, superfici, ripari. Il radar
// confronta voce per voce, non l'ordine; due copie nello stesso ordine si rileggono affiancate.
//
// ⛔ Niente cifre, niente nomi di tag, solo ASCII: lo verifica `RefactorTactics.UI.GlossaryEntriesNameThingsThatExist`.
// ⚠️ Una frase segue la REGOLA, non il catalogo dei terreni: dove i due divergono vale il codice che la applica.
// Due casi: il ghiaccio fa scivolare chi vi TERMINA il movimento, non chi vi entra; la copertura bassa non
// ferma il passo, che il catalogo le attribuisce.
// ⚠️ Il radar trova le due tabelle dalla riga della loro definizione: nessun commento sopra di esse deve
// ripeterne la firma.
const TArray<TPair<FName, FString>>& RTGlossary::Names()
{
	static const TArray<TPair<FName, FString>> Voci = {
		// Stati
		{ TEXT("Status.Root"),        TEXT("Immobilizzato") },
		{ TEXT("Status.Slow"),        TEXT("Rallentato") },
		{ TEXT("Status.Reveal"),      TEXT("Rivelato") },
		{ TEXT("Status.Exposed"),     TEXT("Esposto") },
		{ TEXT("Status.Guarded"),     TEXT("In guardia") },
		{ TEXT("Status.Marked"),      TEXT("Marchiato") },
		{ TEXT("Status.Wet"),         TEXT("Bagnato") },
		{ TEXT("Status.Braced"),      TEXT("Irrigidito") },
		{ TEXT("Status.Burning"),     TEXT("In fiamme") },
		{ TEXT("Status.Obscured"),    TEXT("Nel fumo") },
		{ TEXT("Status.Electrified"), TEXT("Folgorato") },
		{ TEXT("Status.Unbalanced"),  TEXT("Sbilanciato") },
		{ TEXT("Status.Prone"),       TEXT("A terra") },
		{ TEXT("Status.Stunned"),     TEXT("Stordito") },

		// Superfici
		{ TEXT("Terrain.Floor"),        TEXT("Pavimento") },
		{ TEXT("Terrain.ShallowWater"), TEXT("Acqua bassa") },
		{ TEXT("Terrain.Rough"),        TEXT("Accidentato") },
		{ TEXT("Terrain.Fire"),         TEXT("Fuoco") },
		{ TEXT("Terrain.Conductive"),   TEXT("Superficie metallica") },
		{ TEXT("Terrain.Ice"),          TEXT("Ghiaccio") },
		{ TEXT("Terrain.Void"),         TEXT("Vuoto") },
		{ TEXT("Terrain.Smoke"),        TEXT("Fumo") },
		{ TEXT("Terrain.HighGround"),   TEXT("Quota elevata") },

		// Ripari
		{ TEXT("Structure.LowCover"),  TEXT("Copertura bassa") },
		{ TEXT("Structure.HighCover"), TEXT("Copertura alta") },
	};
	return Voci;
}

const TArray<TPair<FName, FString>>& RTGlossary::Effects()
{
	static const TArray<TPair<FName, FString>> Voci = {
		// Stati
		{ TEXT("Status.Root"),        TEXT("Non puoi muoverti ne' scattare.") },
		{ TEXT("Status.Slow"),        TEXT("Ogni cella ti costa di piu': a piedi arrivi meno lontano.") },
		{ TEXT("Status.Reveal"),      TEXT("Gli avversari vedono il tuo piano del turno, ma non la reazione che hai armato.") },
		{ TEXT("Status.Exposed"),     TEXT("Finche' dura, il primo colpo che ricevi in un turno fa piu' danno.") },
		{ TEXT("Status.Guarded"),     TEXT("Ogni colpo che arriva di fronte perde una parte del danno. Una spinta debole non ti sposta, a meno che tu sia sbilanciato.") },
		{ TEXT("Status.Marked"),      TEXT("Il prossimo colpo della squadra che ti ha marchiato fa piu' danno, poi il marchio si consuma.") },
		{ TEXT("Status.Wet"),         TEXT("La scarica lineare di Aevik ti fa piu' danno. Bagnarti spegne le fiamme.") },
		{ TEXT("Status.Braced"),      TEXT("Ogni colpo, da qualunque direzione, perde un po' di danno, e resisti alle spinte finche' non sei sbilanciato.") },
		{ TEXT("Status.Burning"),     TEXT("Subisci danno alla fine di ogni turno, finche' le fiamme durano o l'acqua le spegne.") },
		{ TEXT("Status.Obscured"),    TEXT("Sei nel fumo: attraverso il fumo si mira solo da vicino, in entrambi i versi.") },
		{ TEXT("Status.Electrified"), TEXT("L'elettricita' corsa nel terreno ti ha colpito: sulla cella mirata, oppure lungo acqua e metallo.") },
		{ TEXT("Status.Unbalanced"),  TEXT("Non puoi correre, e guardia e irrigidimento non ti tengono fermo; spinte, trazioni e scivolate ti portano piu' lontano, e una spinta o una trazione che ti sposta ti butta a terra.") },
		{ TEXT("Status.Prone"),       TEXT("Cadendo perdi cio' che avevi armato, e le tue reazioni non scattano. Rialzarti ti costa una parte del movimento.") },
		{ TEXT("Status.Stunned"),     TEXT("Non puoi usare l'azione principale, le tue reazioni non scattano, e perdi cio' che avevi gia' armato.") },

		// Superfici
		{ TEXT("Terrain.Floor"),        TEXT("Nessun effetto proprio. Puo' prendere fuoco.") },
		{ TEXT("Terrain.ShallowWater"), TEXT("Costa piu' movimento e conduce l'elettricita'. Chi ci sta dentro e' bagnato finche' ci resta.") },
		{ TEXT("Terrain.Rough"),        TEXT("Costa piu' movimento, e non lo si attraversa ne' di scatto ne' in carica. Puo' prendere fuoco.") },
		{ TEXT("Terrain.Fire"),         TEXT("Costa piu' movimento. Chi ci entra, o ci si trova quando divampa, subisce danno e prende fuoco.") },
		{ TEXT("Terrain.Conductive"),   TEXT("Conduce l'elettricita'.") },
		{ TEXT("Terrain.Ice"),          TEXT("Chi finisce qui il movimento con piu' di un passo ancora da spendere scivola oltre, nella direzione in cui andava, e resta sbilanciato.") },
		{ TEXT("Terrain.Void"),         TEXT("Non e' un punto d'atterraggio: chi cade non si ferma qui.") },
		{ TEXT("Terrain.Smoke"),        TEXT("Chi ci sta dentro e' nel fumo: attraverso il fumo si mira solo da vicino.") },
		{ TEXT("Terrain.HighGround"),   TEXT("Nessun effetto proprio: indica una posizione rialzata, e il vantaggio viene dal piano della cella.") },

		// Ripari
		{ TEXT("Structure.LowCover"),  TEXT("Riduce il danno dei colpi mirati che ti arrivano di fronte attraverso quel lato; non ripara dalle esplosioni. Non ferma la vista ne' il passo.") },
		{ TEXT("Structure.HighCover"), TEXT("Chiude quel lato: non passano la vista, il passo e i colpi.") },
	};
	return Voci;
}

namespace
{
	/** Il testo di `Key` in `Voci`, oppure vuoto. Lineare come `RTActionDescriptions::For`: le voci sono poche. */
	FText GlossaryLookup(const TArray<TPair<FName, FString>>& Voci, const FName& Key)
	{
		for (const TPair<FName, FString>& Voce : Voci)
		{
			if (Voce.Key == Key)
			{
				return FText::FromString(Voce.Value);
			}
		}
		return FText::GetEmpty();
	}
}

FText RTGlossary::NameFor(const FName& Key)
{
	return GlossaryLookup(Names(), Key);
}

FText RTGlossary::EffectFor(const FName& Key)
{
	return GlossaryLookup(Effects(), Key);
}

FName RTGlossary::KeyForStatus(const FGameplayTag& Tag)
{
	return Tag.IsValid() ? Tag.GetTagName() : NAME_None;
}

FName RTGlossary::KeyForSurface(ERTHexSurface Surface)
{
	// 🔑 La stessa composizione con cui `ARTTurnManager` scrive la causa di un danno da terreno: il prefisso
	// arriva dalla libreria del TurnLog e non da un letterale, cosi' le due non divergono. Il TurnLog la scrive
	// solo per una superficie che fa danno; per le altre e' lo stesso schema, e l'id del catalogo dei terreni —
	// tranne `Terrain.Void`, che il catalogo non elenca.
	return FName(*FString::Printf(TEXT("%s%s"), URTTurnLogLibrary::TerrainCausePrefix(),
		*StaticEnum<ERTHexSurface>()->GetNameStringByValue(static_cast<int64>(Surface))));
}

FName RTGlossary::KeyForCover(ERTHexCoverType Type)
{
	switch (Type)
	{
	case ERTHexCoverType::Low:  return FName(TEXT("Structure.LowCover"));
	case ERTHexCoverType::High: return FName(TEXT("Structure.HighCover"));
	default:                    return NAME_None;
	}
}
```

- [ ] **Step 7: Compila**

```powershell
& "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="D:/Repositories/rt-wt-ispettore/RefactorTactics.uproject" -WaitMutex
```

Atteso: `Result: Succeeded`.

- [ ] **Step 8: Lancia i test e guardali passare**

```powershell
python tools/suite/esegui.py RefactorTactics.UI.Glossary --log "<scratchpad>/glossario-verde.log" -v
```

Atteso: tre test trovati, tre verdi, nessun rosso.

Se `GlossaryCoversEveryStatusSurfaceAndCover` è rosso su una chiave che la tabella non ha — uno stato registrato dopo `4e6d198cd`, un valore di enum nuovo — la voce si scrive leggendo la regola nel codice, si aggiunge alle due tabelle nello stesso punto, e si ripete da Step 7. Non si toglie la chiave dal test.

- [ ] **Step 9: Committa**

```bash
git -C D:/Repositories/rt-wt-ispettore add -- Source/RefactorTactics/UI/RTGlossary.h Source/RefactorTactics/UI/RTGlossary.cpp Source/RefactorTactics/Tests/RTGlossaryTests.cpp
git -C D:/Repositories/rt-wt-ispettore commit -F - <<'EOF'
feat(3744): il glossario di stati, superfici e ripari, col gate di completezza

Una tabella chiavata per gli id che il gioco scrive gia': il nome del tag per
uno stato, la causa del TurnLog per una superficie, l'id del catalogo per un
riparo. Tre test nei due versi: ogni cosa ha una voce, ogni voce nomina
qualcosa che esiste, le chiavi sono quelle attese.

Nessun lettore in partita finche' non arriva lo sguardo dell'ispettore: e'
dichiarato nell'header.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 2: Il gate sa fallire, nei due versi

Un gate nasce verde, e senza una mutazione il suo verde non distingue «ho guardato» da «non ho guardato». Questo task non lascia mutazioni nell'albero: lascia due misure, che vanno nel corpo della PR.

➕ rev2. ⌫ *La prima stesura toglieva una sola riga da `Effects()`*: provava il verso «ogni cosa ha una voce» e lasciava mai visti rossi il verso opposto, i doppioni e le regole dei testi. Qui le mutazioni sono due, nella stessa build, su voci diverse: i messaggi dicono quale ha colpito.

**Files:**
- Modify, e poi ripristina: `Source/RefactorTactics/UI/RTGlossary.cpp`

- [ ] **Step 1: Applica le due mutazioni**

Mutazione A, una voce che manca. In `RTGlossary::Effects()` cancella l'ultima riga della tabella, quella di `Structure.HighCover`. La gemella in `Names()` resta. È l'ultima apposta: nessuna voce dopo di lei scala di posto.

Mutazione B, una voce che non nomina niente e viola le regole dei testi. Nelle **due** tabelle rinomina la chiave `Status.Burning` in `Status.Bruciante`, e in `Effects()` scrivi come suo testo `Brucia per 2 turni`: una cifra, e nessun punto finale.

- [ ] **Step 2: Compila la mutazione**

```powershell
& "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="D:/Repositories/rt-wt-ispettore/RefactorTactics.uproject" -WaitMutex
```

Atteso: `Result: Succeeded`, e l'output nomina `RTGlossary.cpp` fra i file compilati: un file modificato rispetto a `HEAD` esce dal blob unity e si compila da solo. Se non lo nomina, la build non ha visto la modifica e la misura che segue non vale.

- [ ] **Step 3: Lancia i test: due devono cadere, coi messaggi di entrambe le mutazioni**

```powershell
python tools/suite/esegui.py RefactorTactics.UI.Glossary --log "<scratchpad>/glossario-mutazione.log" -v
```

Atteso: tre test trovati, **due rossi e uno verde**.
- `GlossaryCoversEveryStatusSurfaceAndCover` rosso, con: «Structure.HighCover ha un effetto» (A); «Status.Burning ha un nome» e «Status.Burning ha un effetto» (B).
- `GlossaryEntriesNameThingsThatExist` rosso, con: «nomi ed effetti hanno lo stesso numero di voci» (A); «Status.Bruciante nomina uno stato, una superficie o un riparo che esiste», «Status.Bruciante (effetto): non porta cifre» e «Status.Bruciante (effetto): finisce col punto» (B).
- `GlossaryKeysAreTheIdsTheGameWrites` verde: le funzioni delle chiavi non sono cambiate.

I messaggi si leggono dal log, uno per uno. Se ne manca uno, il ramo che lo produce non guarda ciò che dichiara: ci si ferma e lo si corregge prima di tutto il resto.

- [ ] **Step 4: Ripristina**

```bash
git -C D:/Repositories/rt-wt-ispettore checkout -- Source/RefactorTactics/UI/RTGlossary.cpp
git -C D:/Repositories/rt-wt-ispettore status --short -- Source/
```

Atteso: nessuna riga per `RTGlossary.cpp`. ⚠️ `git checkout` riscrive il file con la data di adesso, quindi la build che segue lo ricompila. Un ripristino che conservasse la data vecchia lascerebbe il binario mutato.

- [ ] **Step 5: Ricompila e rimisura il verde**

```powershell
& "D:/EpicGames/UE_5.8/Engine/Build/BatchFiles/Build.bat" RefactorTacticsEditor Win64 Development -Project="D:/Repositories/rt-wt-ispettore/RefactorTactics.uproject" -WaitMutex
```

```powershell
python tools/suite/esegui.py RefactorTactics.UI.Glossary --log "<scratchpad>/glossario-dopo-mutazione.log" -v
```

Atteso: `Result: Succeeded`, poi tre test trovati e tre verdi. Le due misure — rosso con le mutazioni, verde dopo il ripristino — si riportano nel corpo della PR coi nomi dei due log.

---

### Task 3: Il documento, il radar e il README

**Files:**
- Create: `docs/balance/RT_Glossary_v0.1.md`
- Create: `tools/radar/glossary.ts`
- Test: `tools/radar/glossary.test.ts`
- Modify: `docs/balance/README.md` (una riga nella tabella dei documenti, dopo `RT_ActionDescriptions_v0.1.md`)

**Interfaces:**
- Consumes: `cppPairs`, `docRows`, `compare`, `Divergenza` da `tools/radar/action-descriptions.ts`; le due tabelle del Task 1, lette come testo.
- Produces: `confronta(doc: string, cpp: string): Esito`, e il comando `node tools/radar/glossary.ts --check`.

- [ ] **Step 1: Scrivi il test del radar, che fallisce**

`tools/radar/glossary.test.ts`:

```ts
import { test } from 'node:test';
import assert from 'node:assert/strict';
import { confronta } from './glossary.ts';

/** Le forme sono copiate dai due file veri — la tabella C++ e il documento — non inventate. */
const CPP = [
  '#include "UI/RTGlossary.h"',
  '// Le voci della v0.1.',
  'const TArray<TPair<FName, FString>>& RTGlossary::Names()',
  '{',
  '\tstatic const TArray<TPair<FName, FString>> Voci = {',
  '\t\t// Stati',
  '\t\t{ TEXT("Status.Burning"),     TEXT("In fiamme") },',
  '\t\t{ TEXT("Terrain.Ice"),          TEXT("Ghiaccio") },',
  '\t};',
  '\treturn Voci;',
  '}',
  '',
  'const TArray<TPair<FName, FString>>& RTGlossary::Effects()',
  '{',
  '\tstatic const TArray<TPair<FName, FString>> Voci = {',
  '\t\t{ TEXT("Status.Burning"),     TEXT("Subisci danno alla fine di ogni turno, finche\' le fiamme durano o l\'acqua le spegne.") },',
  '\t\t{ TEXT("Terrain.Ice"),          TEXT("Chi finisce qui il movimento con piu\' di un passo ancora da spendere scivola oltre, nella direzione in cui andava, e resta sbilanciato.") },',
  '\t};',
  '\treturn Voci;',
  '}',
  '',
  'FText RTGlossary::NameFor(const FName& Key)',
  '{',
  '\treturn GlossaryLookup(Names(), Key);',
  '}',
].join('\n');

const DOC = [
  '## Stati',
  '',
  '| Voce | Nome | Effetto | Da rivedere |',
  '|---|---|---|---|',
  "| `Status.Burning` | In fiamme | Subisci danno alla fine di ogni turno, finche' le fiamme durano o l'acqua le spegne. |  |",
  '',
  '## Superfici',
  '',
  '| Voce | Nome | Effetto | Da rivedere |',
  '|---|---|---|---|',
  "| `Terrain.Ice` | Ghiaccio | Chi finisce qui il movimento con piu' di un passo ancora da spendere scivola oltre, nella direzione in cui andava, e resta sbilanciato. |  |",
].join('\r\n');

test('due copie uguali concordano, e la copertura si conta su entrambi i lati', () => {
  const e = confronta(DOC, CPP);
  assert.deepEqual(e.divergenze, []);
  assert.deepEqual([e.righe, e.nomi, e.effetti], [2, 2, 2]);
});

test('un effetto cambiato nel solo documento e una divergenza, con entrambi i valori', () => {
  const e = confronta(DOC.replace('scivola oltre', 'scivola'), CPP);
  assert.deepEqual(e.divergenze, [
    {
      id: 'Terrain.Ice',
      campo: 'frase',
      doc: "Chi finisce qui il movimento con piu' di un passo ancora da spendere scivola, nella direzione in cui andava, e resta sbilanciato.",
      cpp: "Chi finisce qui il movimento con piu' di un passo ancora da spendere scivola oltre, nella direzione in cui andava, e resta sbilanciato.",
    },
  ]);
});

test('un nome cambiato nel solo documento e una divergenza', () => {
  const e = confronta(DOC.replace('| Ghiaccio |', '| Lastra di ghiaccio |'), CPP);
  assert.deepEqual(e.divergenze.map((d) => [d.id, d.campo]), [['Terrain.Ice', 'nome']]);
});

test('una voce che manca dal documento e una divergenza di presenza', () => {
  const senzaGhiaccio = DOC.split('\r\n').slice(0, 5).join('\r\n');
  const e = confronta(senzaGhiaccio, CPP);
  assert.deepEqual(e.divergenze.map((d) => [d.id, d.campo]), [['Terrain.Ice', 'presenza']]);
});

test('una funzione rinominata da zero voci da quel lato: chi chiama lo tratta come errore', () => {
  const e = confronta(DOC, CPP.replace('RTGlossary::Effects()', 'RTGlossary::Effetti()'));
  assert.equal(e.effetti, 0);
  assert.equal(e.nomi, 2);
});

test('un commento che nomina la funzione prima della definizione non sposta la lettura', () => {
  const conCommento = CPP.replace(
    '// Le voci della v0.1.',
    '// Le voci della v0.1: vedi RTGlossary::Effects() e RTGlossary::Names() qui sotto.',
  );
  const e = confronta(DOC, conCommento);
  assert.deepEqual(e.divergenze, []);
  assert.deepEqual([e.nomi, e.effetti], [2, 2]);
});
```

- [ ] **Step 2: Lancialo e guardalo fallire**

```bash
cd D:/Repositories/rt-wt-ispettore/tools/radar && node --test glossary.test.ts
```

Atteso: FAIL, con `Cannot find module` su `./glossary.ts`.

- [ ] **Step 3: Scrivi il radar**

`tools/radar/glossary.ts`:

```ts
/** Il confronto **glossario: documento ↔ C++** (`#3744`).
 *
 *  Il nome e l'effetto di ogni stato, superficie e riparo stanno in due posti, ed e' una scelta dichiarata: il
 *  gioco li legge dalle due tabelle di `RTGlossary.cpp`, l'autore li rilegge in
 *  `docs/balance/RT_Glossary_v0.1.md`. Due copie senza un gate divergono in silenzio. Questo e' il gate.
 *
 *  🔑 **Il parser e il confronto sono quelli delle frasi d'azione** (`action-descriptions.ts`): stessa forma di
 *  tabella C++, stessa forma di riga nel documento. Qui cambiano solo le sorgenti e le due ancore.
 *
 *  Confronta, per chiave: l'**effetto** (documento ↔ `Effects`), il **nome** (documento ↔ `Names`), la
 *  **presenza** (una voce senza riga, una riga senza voce, una riga doppia).
 *
 *  ⚠️ **Non prescrive quale lato correggere**: stampa i due valori e si ferma.
 *
 *  ⚠️ **Non dice se una voce NOMINA qualcosa che esiste**, ne' se manca uno stato: quello lo verificano i test
 *  `RefactorTactics.UI.Glossary*`, che vedono i tag registrati e gli enum. Un verde qui significa «le due copie
 *  concordano», non «il glossario e' completo».
 *
 *  🔴 **Il guasto temuto e' il falso verde.** Se un'ancora smettesse di trovare la sua tabella, due insiemi
 *  vuoti concorderebbero. Percio' zero voci da un lato e' un errore, e la copertura si stampa sempre.
 *
 *  Uso:  node tools/radar/glossary.ts [--check]
 */
import { fileURLToPath, pathToFileURL } from 'node:url';
import { compare, cppPairs, docRows } from './action-descriptions.ts';
import type { Divergenza } from './action-descriptions.ts';

/** 🔑 L'ancora comprende la fine del tipo di ritorno: corrisponde alla DEFINIZIONE, non a un commento che la nomina. */
const ANCORA_NOMI = '>& RTGlossary::Names()';
const ANCORA_EFFETTI = '>& RTGlossary::Effects()';

export interface Esito {
  righe: number;
  nomi: number;
  effetti: number;
  divergenze: Divergenza[];
}

/** Legge le due copie e le confronta. `doc` e' il documento, `cpp` il sorgente di `RTGlossary.cpp`. */
export function confronta(doc: string, cpp: string): Esito {
  const righe = docRows(doc);
  const nomi = cppPairs(cpp, ANCORA_NOMI);
  const effetti = cppPairs(cpp, ANCORA_EFFETTI);
  return { righe: righe.length, nomi: nomi.length, effetti: effetti.length, divergenze: compare(righe, effetti, nomi) };
}

if (import.meta.url === pathToFileURL(process.argv[1] ?? '').href) {
  const { readFileSync } = await import('node:fs');
  const dallaRadice = (p: string) => readFileSync(fileURLToPath(new URL(`../../${p}`, import.meta.url)), 'utf8');

  const esito = confronta(
    dallaRadice('docs/balance/RT_Glossary_v0.1.md'),
    dallaRadice('Source/RefactorTactics/UI/RTGlossary.cpp'),
  );
  console.error(`copertura: ${esito.righe} righe nel documento · ${esito.nomi} nomi e ${esito.effetti} effetti in C++`);

  if (esito.righe === 0 || esito.nomi === 0 || esito.effetti === 0) {
    console.error(
      "errore: un lato non ha prodotto voci. Il parser non trova piu' la sua sorgente, e un confronto vuoto " +
        'sarebbe un falso verde.',
    );
    process.exit(1);
  }

  if (esito.divergenze.length > 0) {
    console.error(
      `errore: ${esito.divergenze.length} divergenze fra documento e C++. Nessuno dei due lati e' presunto giusto:\n` +
        esito.divergenze.map((d) => `  ${d.id}  [${d.campo}]\n    doc: ${d.doc}\n    cpp: ${d.cpp}`).join('\n'),
    );
    process.exit(1);
  }
  console.error('documento e C++ concordano su nomi ed effetti');
}
```

⚠️ `compare` chiama `frase` il campo che qui è l'effetto: il nome viene dal modulo riusato, e il test del passo 1 lo usa così.

- [ ] **Step 4: Lancia il test e guardalo passare**

```bash
cd D:/Repositories/rt-wt-ispettore/tools/radar && node --test glossary.test.ts
```

Atteso: `pass 6`, `fail 0`.

- [ ] **Step 5: Scrivi il documento**

`docs/balance/RT_Glossary_v0.1.md`. Le righe delle tabelle portano il testo **identico** a quello di `RTGlossary.cpp`, carattere per carattere:

````markdown
# RT — Glossario di stati, superfici e ripari v0.1

> **Owner**: [#3744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3744), fetta di
> [#705](https://github.com/DegrassiAaron/refactor-tactics-main/issues/705) · **Stato**: **bozza, da accettare
> dall'autore**. Le voci le ha scritte chi ha implementato, a partire dalle regole nel codice.
>
> Il **nome** da leggere e l'**effetto in una riga** di ciò che il giocatore incontra sulla mappa: uno stato
> addosso a un'unità, la superficie di una cella, un riparo su un lato.
>
> - **Un testo non porta numeri.** Durata, danno e costo li compone il gioco dai dati: un numero scritto qui
>   invecchierebbe al primo ribilanciamento.
> - **Il testo è quello che il gioco mostrerà**, carattere per carattere, compreso l'apostrofo al posto
>   dell'accento (`piu'`, `e'`), che è la convenzione del testo a schermo del progetto.
> - **Le copie sono due**, questa e le tabelle in
>   [`RTGlossary.cpp`](../../Source/RefactorTactics/UI/RTGlossary.cpp). Il radar le confronta, e non dice quale
>   delle due correggere:
>
>   ```sh
>   node tools/radar/glossary.ts --check   # exit 1 se un nome o un effetto divergono
>   ```
>
> - **Niente resta senza voce**: i test `RefactorTactics.UI.Glossary*` falliscono se uno stato registrato, una
>   superficie o un tipo di riparo non ha una voce, o se una voce non nomina più niente.
>
> ⚠️ **Sorgente senza lettore.** Oggi nessuno mostra queste voci in partita. I lettori sono lo sguardo e la
> scheda dell'[ispettore del mondo](../superpowers/specs/2026-10-13-ispettore-del-mondo-design.md), fette `F2` e
> `F3`.
>
> **Come rivederle**: correggi le colonne *Nome* ed *Effetto*. Il cambio va riportato nelle tabelle C++ nella
> stessa PR; il radar lo pretende. La colonna *Voce* è la chiave della tabella, e non si cambia qui.

## Stati

La chiave è il nome del tag. Le regole stanno in
[`spec-stati-temporanei-cp82.md`](../gameplay/spec-stati-temporanei-cp82.md) e nel resolver.

| Voce | Nome | Effetto | Da rivedere |
|---|---|---|---|
| `Status.Root` | Immobilizzato | Non puoi muoverti ne' scattare. |  |
| `Status.Slow` | Rallentato | Ogni cella ti costa di piu': a piedi arrivi meno lontano. |  |
| `Status.Reveal` | Rivelato | Gli avversari vedono il tuo piano del turno, ma non la reazione che hai armato. | oggi nessuna azione lo applica |
| `Status.Exposed` | Esposto | Finche' dura, il primo colpo che ricevi in un turno fa piu' danno. |  |
| `Status.Guarded` | In guardia | Ogni colpo che arriva di fronte perde una parte del danno. Una spinta debole non ti sposta, a meno che tu sia sbilanciato. |  |
| `Status.Marked` | Marchiato | Il prossimo colpo della squadra che ti ha marchiato fa piu' danno, poi il marchio si consuma. |  |
| `Status.Wet` | Bagnato | La scarica lineare di Aevik ti fa piu' danno. Bagnarti spegne le fiamme. | il danno in piu' vale per una sola abilita' |
| `Status.Braced` | Irrigidito | Ogni colpo, da qualunque direzione, perde un po' di danno, e resisti alle spinte finche' non sei sbilanciato. |  |
| `Status.Burning` | In fiamme | Subisci danno alla fine di ogni turno, finche' le fiamme durano o l'acqua le spegne. |  |
| `Status.Obscured` | Nel fumo | Sei nel fumo: attraverso il fumo si mira solo da vicino, in entrambi i versi. |  |
| `Status.Electrified` | Folgorato | L'elettricita' corsa nel terreno ti ha colpito: sulla cella mirata, oppure lungo acqua e metallo. | non resta addosso: e' l'istante di una scarica |
| `Status.Unbalanced` | Sbilanciato | Non puoi correre, e guardia e irrigidimento non ti tengono fermo; spinte, trazioni e scivolate ti portano piu' lontano, e una spinta o una trazione che ti sposta ti butta a terra. |  |
| `Status.Prone` | A terra | Cadendo perdi cio' che avevi armato, e le tue reazioni non scattano. Rialzarti ti costa una parte del movimento. |  |
| `Status.Stunned` | Stordito | Non puoi usare l'azione principale, le tue reazioni non scattano, e perdi cio' che avevi gia' armato. | oggi nessuna azione lo applica |

## Superfici

La chiave è `Terrain.` più il nome del valore di `ERTHexSurface`: la stessa causa che il TurnLog scrive per un
danno da terreno. Le regole stanno in [`RT_TerrainCatalog_v0.1.md`](RT_TerrainCatalog_v0.1.md) §1.

| Voce | Nome | Effetto | Da rivedere |
|---|---|---|---|
| `Terrain.Floor` | Pavimento | Nessun effetto proprio. Puo' prendere fuoco. |  |
| `Terrain.ShallowWater` | Acqua bassa | Costa piu' movimento e conduce l'elettricita'. Chi ci sta dentro e' bagnato finche' ci resta. |  |
| `Terrain.Rough` | Accidentato | Costa piu' movimento, e non lo si attraversa ne' di scatto ne' in carica. Puo' prendere fuoco. |  |
| `Terrain.Fire` | Fuoco | Costa piu' movimento. Chi ci entra, o ci si trova quando divampa, subisce danno e prende fuoco. |  |
| `Terrain.Conductive` | Superficie metallica | Conduce l'elettricita'. |  |
| `Terrain.Ice` | Ghiaccio | Chi finisce qui il movimento con piu' di un passo ancora da spendere scivola oltre, nella direzione in cui andava, e resta sbilanciato. | il catalogo dei terreni dice «all'ingresso»: il codice fa scivolare chi vi TERMINA il movimento, se gli resta piu' di un passo |
| `Terrain.Void` | Vuoto | Non e' un punto d'atterraggio: chi cade non si ferma qui. | non ha una riga nel catalogo dei terreni; il passo lo blocca la cella, non la superficie |
| `Terrain.Smoke` | Fumo | Chi ci sta dentro e' nel fumo: attraverso il fumo si mira solo da vicino. |  |
| `Terrain.HighGround` | Quota elevata | Nessun effetto proprio: indica una posizione rialzata, e il vantaggio viene dal piano della cella. | la mappa dimostrativa la mette anche su celle del piano terra: li' non indica niente |

## Ripari

La chiave è l'id di [`RT_TerrainCatalog_v0.1.md`](RT_TerrainCatalog_v0.1.md) §3. Un riparo sta su un **lato** della
cella, e vale per ciò che attraversa quel lato.

| Voce | Nome | Effetto | Da rivedere |
|---|---|---|---|
| `Structure.LowCover` | Copertura bassa | Riduce il danno dei colpi mirati che ti arrivano di fronte attraverso quel lato; non ripara dalle esplosioni. Non ferma la vista ne' il passo. | il catalogo dei terreni dice «blocca l'arco»: nel codice solo la copertura alta ferma il passo |
| `Structure.HighCover` | Copertura alta | Chiude quel lato: non passano la vista, il passo e i colpi. |  |
````

- [ ] **Step 6: Lancia il radar sui file veri**

```bash
cd D:/Repositories/rt-wt-ispettore && node tools/radar/glossary.ts --check; echo "exit: $?"
```

Atteso: la riga `copertura:` con lo stesso numero sui tre lati, poi `documento e C++ concordano su nomi ed effetti`, `exit: 0`.

- [ ] **Step 7: Controllo positivo: il radar sa fallire sui file veri**

➕ rev2. ⌫ *La prima stesura metteva copia, mutazione e radar in una sola catena di `&&`*, e leggeva `$?` in coda: se la copia falliva, la catena si fermava e l'eco stampava `1`, cioè proprio il valore atteso. Qui la mutazione si prova da sé, e il radar ha un comando suo.

```bash
cd D:/Repositories/rt-wt-ispettore
cp docs/balance/RT_Glossary_v0.1.md "<scratchpad>/RT_Glossary.bak"
sed -i 's/| Ghiaccio |/| Lastra |/' docs/balance/RT_Glossary_v0.1.md
grep -c '| Lastra |' docs/balance/RT_Glossary_v0.1.md
```

Atteso: `1`. È la prova che la mutazione c'è.

```bash
cd D:/Repositories/rt-wt-ispettore && node tools/radar/glossary.ts --check; rc=$?; echo "radar con un nome cambiato: $rc"
```

Atteso: la divergenza `Terrain.Ice  [nome]` stampata, e `radar con un nome cambiato: 1`.

```bash
cd D:/Repositories/rt-wt-ispettore
cp "<scratchpad>/RT_Glossary.bak" docs/balance/RT_Glossary_v0.1.md
git diff --stat -- docs/balance/RT_Glossary_v0.1.md
node tools/radar/glossary.ts --check; rc=$?; echo "radar dopo il ripristino: $rc"
```

Atteso: `radar dopo il ripristino: 0`.

- [ ] **Step 8: Aggiungi la riga al README**

In `docs/balance/README.md`, subito dopo la riga di `RT_ActionDescriptions_v0.1.md`:

```markdown
| [`RT_Glossary_v0.1.md`](RT_Glossary_v0.1.md) | Nome ed effetto in una riga di stati, superfici e ripari ([#3744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3744)): **testo**, non numeri | ✅ come **testo**: copia delle tabelle C++, confrontata da `node tools/radar/glossary.ts --check` — ⚠️ **sorgente senza lettore**: nessuno mostra ancora queste voci in partita |
```

- [ ] **Step 9: Lancia tutti i radar e i loro test**

```bash
cd D:/Repositories/rt-wt-ispettore && for f in tools/radar/*.ts; do n=$(basename "$f" .ts); case "$n" in *.test|cli|files|git) continue;; esac; \
  if [ "$n" = "wiki-alt" ]; then node "$f" --check --wiki-root "D:/Repositories/refactor-tactics-main.wiki" >/dev/null 2>&1; else node "$f" --check >/dev/null 2>&1; fi; echo "$n: $?"; done
cd tools/radar && node --test 2>&1 | tail -8
```

Atteso: `glossary: 0`, `doc-links: 0`, `doc-tables: 0`, `doc-coherence: 0`. `pie-verdict-age` e `wiki-alt` erano `1` su `origin/main` `4e6d198cd` prima di questo lavoro: restano com'erano, e ogni altro `1` è di questo branch. `node --test`: `fail 0`.

- [ ] **Step 10: Committa**

```bash
git -C D:/Repositories/rt-wt-ispettore add -- docs/balance/RT_Glossary_v0.1.md docs/balance/README.md tools/radar/glossary.ts tools/radar/glossary.test.ts
git -C D:/Repositories/rt-wt-ispettore commit -F - <<'EOF'
docs(3744): il documento del glossario e il radar che lo confronta con la tabella

Le voci che l'autore rilegge stanno in docs/balance, copia delle due tabelle
C++. Il radar riusa parser e confronto delle frasi d'azione: cambiano le
sorgenti e le ancore, che corrispondono alla definizione e non a un commento.

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

---

### Task 4: Chiusura — suite, spec, PR, revisione, merge, issue

**Files:**
- Modify: `docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md` (lo statuto in testa, e la riga `F1` di §9.1)
- Modify: questo piano, con le note `➕ esecuzione`

- [ ] **Step 1: Allinea a `origin/main`**

```bash
git -C D:/Repositories/rt-wt-ispettore fetch --prune origin
git -C D:/Repositories/rt-wt-ispettore log --oneline HEAD..origin/main
```

Se `origin/main` è avanzato: `git -C D:/Repositories/rt-wt-ispettore merge origin/main`, poi si rifanno la build e i tre test del glossario prima di proseguire. Se fra i commit entrati c'è un tag `Status.` nuovo o un valore di `ERTHexSurface` nuovo, il gate diventa rosso ed è corretto: si scrive la voce.

- [ ] **Step 2: Suite intera sul commit della PR**

```powershell
Get-CimInstance Win32_Process -Filter "Name LIKE 'UnrealEditor%'" | Select ProcessId, Name, CommandLine
python tools/suite/esegui.py RefactorTactics --log "<scratchpad>/suite-3744.log" -v
```

⚠️ Il binario deve venire dal codice di `HEAD`: se un file sotto `Source/` è cambiato dopo l'ultima build — per esempio per il merge dello Step 1 — si ricompila prima, in un comando a sé.

Atteso: il referto coi conteggi. Questo branch aggiunge file e non ne modifica di esistenti nel modulo. ➕ rev2. Un rosso fuori da `RefactorTactics.UI.Glossary*` è quindi improbabile, non escluso: i test che leggono ogni file di `Source/` — per esempio `RTLegacyIdentityRatchetTests` — hanno i file nuovi fra i loro ingressi. Ogni rosso si elenca per nome nel corpo della PR, e per ciascuno si **dimostra** la causa prima di attribuirlo altrove: legge un asset che il worktree non ha (`git ls-files <asset>` vuoto), oppure è rosso anche su `origin/main`. Non si dichiara «suite verde» se un rosso c'è.

- [ ] **Step 3: Aggiorna la spec**

In testa alla spec, dopo il paragrafo `➕ rev.` dello statuto, aggiungi:

```markdown
>
> ✅ **Fetta `F1` implementata** in [#3744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3744), col
> piano [`2026-10-13-glossario-stati-superfici-ripari.md`](../plans/2026-10-13-glossario-stati-superfici-ripari.md):
> la tabella `RTGlossary`, il documento [`RT_Glossary_v0.1.md`](../../balance/RT_Glossary_v0.1.md), il radar e i
> test `RefactorTactics.UI.Glossary*`. I testi restano da accettare dall'autore. Le fette `F2`–`F6` non hanno
> ancora una issue.
```

E nella tabella di §9.1 la riga `F1` diventa:

```markdown
| **F1** | **Glossario**: testi d'autore per stati, superfici e ripari, col gate di completezza. Nessuna UI — ✅ [#3744](https://github.com/DegrassiAaron/refactor-tactics-main/issues/3744) | — |
```

- [ ] **Step 4: Radar documentali, e commit**

```bash
cd D:/Repositories/rt-wt-ispettore && for r in doc-links doc-tables doc-coherence glossary; do node tools/radar/$r.ts --check >/dev/null 2>&1; echo "$r: $?"; done
git -C D:/Repositories/rt-wt-ispettore add -- docs/superpowers/specs/2026-10-13-ispettore-del-mondo-design.md docs/superpowers/plans/2026-10-13-glossario-stati-superfici-ripari.md
git -C D:/Repositories/rt-wt-ispettore commit -F - <<'EOF'
docs(3744): la spec dell'ispettore registra la fetta F1, e il piano le note di esecuzione

Co-Authored-By: Claude Opus 5.5 <noreply@anthropic.com>
EOF
```

Atteso: quattro `0`.

- [ ] **Step 5: Push e PR verso `main`**

Il branch nasce da `origin/main`: la base della PR è `main`.

```bash
cd D:/Repositories/rt-wt-ispettore && GIT_TERMINAL_PROMPT=0 git -c credential.helper= \
  -c credential.https://github.com.helper='!f() { echo username=x-access-token; echo "password=$(gh auth token --user DegrassiAaron)"; }; f' \
  push -u origin issue/3744-glossario-stati-superfici-ripari
```

Il corpo della PR va in un file dello scratchpad e comincia con `Closes #3744`. Porta, in quest'ordine: che cosa entra; che cosa non entra (nessuna UI, nessun lettore); i gate col loro stato in parole — `Compile`, `Tests` coi nomi dei tre test, la mutazione del Task 2 coi nomi dei log, `Determinism`, `Replay` e `Privacy` `N/A` con la ragione, `PIE` `N/A`, `Packaged` `NOT RUN`; i rossi della suite intera, per nome; i testi da accettare, col link al documento. Chiude con `🤖 Generated with [Claude Code](https://claude.com/claude-code)`.

```bash
export GH_TOKEN="$(gh auth token --user DegrassiAaron)"
gh pr create -R DegrassiAaron/refactor-tactics-main --base main --head issue/3744-glossario-stati-superfici-ripari \
  --title "feat(3744): il glossario di stati, superfici e ripari, col gate di completezza" --body-file "<scratchpad>/pr-3744-body.md"
unset GH_TOKEN
```

- [ ] **Step 6: Revisione del codice**

Si lancia la skill `code-review` sulla PR. Ogni rilievo si verifica sul codice prima di applicarlo; ciò che si corregge si ricompila e si rimisura coi tre test, e va in un commit `fix(3744): dopo la revisione — …`. Ciò che non si applica si dichiara nella PR, col motivo.

- [ ] **Step 7: Prima del merge, rimisura**

```bash
git -C D:/Repositories/rt-wt-ispettore fetch --prune origin
git -C D:/Repositories/rt-wt-ispettore log --oneline HEAD..origin/main
```

Se `origin/main` è avanzato dopo la suite dello Step 2, si mergia `origin/main` nel branch e si rifanno build e test del glossario. I gate valgono per il commit che si mergia.

- [ ] **Step 8: Merge, e verifica dei due esiti**

```bash
export GH_TOKEN="$(gh auth token --user DegrassiAaron)"
gh pr merge <n> -R DegrassiAaron/refactor-tactics-main --merge --delete-branch
gh pr view <n> -R DegrassiAaron/refactor-tactics-main --json state,mergeCommit
unset GH_TOKEN
git -C D:/Repositories/rt-wt-ispettore ls-remote --heads origin issue/3744-glossario-stati-superfici-ripari
```

⚠️ `gh pr merge` può stampare un errore dopo aver già mergiato: l'esito si legge da `state`, non dall'output. Se il ref remoto c'è ancora, si cancella con `git push origin --delete`, dopo aver provato che il lavoro è in `main` (`git merge-base --is-ancestor <sha> origin/main`).

- [ ] **Step 9: Issue e pulizia**

- Nel corpo di #3744 si spuntano le voci della DoD che hanno un'evidenza, ciascuna con la sua: il nome del test, il nome del log, il commit. La voce «I testi sono presentati all'autore» si spunta solo dopo averlo fatto.
- Si controlla che la issue sia chiusa (`gh issue view 3744 --json state`); se `Closes` non l'ha chiusa, si chiude a mano con un commento che cita la PR.
- Il worktree si rimuove solo dopo il merge verificato: `git -C D:/Repositories/refactor-tactics-main worktree remove D:/Repositories/rt-wt-ispettore`, poi `git -C D:/Repositories/refactor-tactics-main branch -D issue/3744-glossario-stati-superfici-ripari`. Sono i soli due comandi che toccano il clone principale, e toccano solo i suoi metadati.

---

## Self-review del piano (eseguito in scrittura)

**1. Copertura della spec.** La fetta `F1` chiede testi per stati, superfici e ripari, col gate di completezza, e nessuna UI: Task 1 (tabella e gate), Task 3 (documento e radar). Il paragrafo «Il glossario» di §4 chiede una tabella, un documento che l'autore rilegge e un gate che diventa rosso quando una voce manca: i tre ci sono, e il Task 2 prova il rosso. Le regole di privacy di §5 non si applicano: il glossario è testo pubblico e non legge nessuno stato di partita.

**2. Segnaposto.** Ogni passo di codice porta il codice. I soli `<…>` sono `<scratchpad>`, definito in «Global Constraints», e `<n>`, il numero della PR che nasce nello Step 5 del Task 4.

**3. Coerenza dei tipi.** `NameFor`, `EffectFor`, `KeyForStatus`, `KeyForSurface`, `KeyForCover`, `Names`, `Effects` hanno la stessa firma nell'header, nello scheletro, nell'implementazione, nei test e nel blocco «Interfaces». Il radar legge `Names()` ed `Effects()` con le ancore `>& RTGlossary::Names()` e `>& RTGlossary::Effects()`, che sono il testo della definizione.

**4. Review Focus.** Le cinque voci hanno ciascuna il proprio test, nominato accanto.

## Revisione del piano

Fatta il 2026-10-13, su richiesta dell'autore, da chi l'ha scritto: non è una revisione indipendente. Il metodo è stato confrontare ogni passo con una seconda fonte — gli header del motore, gli strumenti del repository, il codice su `4e6d198cd`.

| Che cosa si è confrontato | Con che cosa | Esito |
|---|---|---|
| `TestEqual` fra due `FName` | `Engine/Source/Runtime/Core/Public/Misc/AutomationTest.h` | l'overload esiste |
| Il progetto che `tools/suite/esegui.py` lancia | la sua costante `RADICE`, derivata dal percorso del file | è il checkout da cui lo si esegue, non il clone principale |
| La composizione di `Terrain.<Superficie>` | `ARTTurnManager`, dove nasce la causa di un danno da terreno | stesso prefisso, stessa funzione sull'enum |
| L'enumerazione dei tag di stato | `URTIconLibrary`, che pretende un'icona per ogni tag | stessa chiamata |
| «Sbilanciato: non puoi scattare» | `ARTUnit::PlannedDashDeniedByStatus` | ⌫ **falso**: nega la corsa e lascia lo scatto lineare. La frase dice «correre», come il motivo di rifiuto che il gioco già scrive |
| «Marchiato: il primo colpo che ricevi» | `ARTTurnManager`, il confronto con `GetMarkedByTeam()` | ⌫ **impreciso**: vale solo per la squadra di chi ha marchiato |
| Il passo di mutazione | la build adattiva di UBT | un file modificato si compila da solo ed è nominato; dopo il ripristino torna nel blob |
| Il ciclo sui caratteri di una stringa | — | riscritto per indice, per non dipendere da come `FString` espone il terminatore |
| Un comando di build del Task 2 | `grep` di un carattere a larghezza zero | ⌫ **un carattere invisibile nel percorso del motore**: il comando non avrebbe trovato `Build.bat`. Tolto |

### ➕ rev2. Revisione avversaria

Chiesta dall'autore il 2026-10-13: un agente con contesto fresco, in sola lettura, con un tetto di trenta chiamate, sul piano e sui file già scritti. Ogni rilievo è stato riletto sul codice da chi scrive prima di entrare nel lavoro.

| | Gravità | Rilievo | Seconda fonte | Esito |
|---|---|---|---|---|
| 1 | critico | `Terrain.Ice` diceva «chi ci entra scivola» | `FRTTerrainDef::SlideCells`: chi **termina** il movimento, con budget residuo | frase corretta |
| 2 | critico | `Terrain.Void` diceva «non ci si cammina» | nessun lettore nel movimento: è escluso solo come cella d'atterraggio | frase corretta |
| 3 | importante | `Structure.LowCover` taceva aree e arco frontale | il commento sulla copertura in `URTCombatLibrary`; `FRTHexCover` | frase corretta |
| 4 | importante | `Status.Electrified` taceva il bersaglio diretto | `CollectElectricPropagation`, passo zero | frase corretta |
| 5 | importante | `Status.Unbalanced` taceva che annulla guardia e irrigidimento, e le trazioni | `ARTTurnManager`, spostamento forzato | tre frasi corrette |
| 6 | importante | `Status.Wet` generalizzava il bonus di una sola abilità | il commento di `AevikWetDischargeBonus` | frase corretta |
| 7 | importante | `Terrain.HighGround` prometteva un vantaggio che la superficie non dà | la riga di catalogo è quella del pavimento | frase corretta |
| 8 | importante | la mutazione provava un solo verso del gate | la lettura dei tre test | Task 2 riscritto, due mutazioni |
| 9 | importante | la regola d'attesa guardava solo la propria misura | `CLAUDE.md` §10 | vincolo corretto |
| 10 | minore | «un rosso altrui non può nascere da qui» | i test che leggono ogni file di `Source/` | Task 4 Step 2 corretto |
| 11 | minore | `$?` letto dietro una catena di `&&` | la semantica della shell | Task 3 Step 7 riscritto |
| 12 | minore | `Status.Stunned` e `Status.Reveal` non hanno un produttore | `ARTTurnManager`; i lettori di `TAG_Status_Reveal` | nota nel documento |
| 13 | minore | `Terrain.Rough`: la regola è l'attraversamento | `FRTTerrainDef::bBlocksDashCharge` | frase corretta |
| 14 | minore | pavimento e accidentato possono prendere fuoco | `FRTTerrainDef::bFlammable` | due frasi corrette |
| 15 | minore | «chiavi che il gioco scrive già» vale per una superficie | `ARTTurnManager` scrive la causa solo per ciò che fa danno | commenti corretti |
| 16 | minore | l'ordine «uguale al documento» non lo verifica nessuno | `compare` indicizza per chiave | commento corretto: il radar non confronta l'ordine |

Il revisore non ha trovato difetti di compilazione né un test che non possa fallire. Ha lasciato **non verificato**, fra l'altro: la frase di `Status.Marked`, la direzione dello scivolamento, «in entrambi i versi» del fumo. Chi scrive le ha rilette dopo: `GetMarkedByTeam` in `ARTTurnManager`, `URTHexSimLibrary::ApplyIceSliding`, `URTTerrainLibrary::EffectiveTargetingRange`.

### ➕ rev3. Revisione avversaria finale, sul branch finito

Un secondo agente con contesto fresco, in sola lettura, sui file committati. Nessun rilievo critico. Ogni rilievo è stato riletto sul codice prima di entrare, ed è entrato nel commit `fix(3744): dopo la revisione avversaria finale`.

| | Gravità | Rilievo | Seconda fonte | Esito |
|---|---|---|---|---|
| 1 | importante | `Status.Guarded`: «finché non sei sbilanciato» si leggeva anche sul danno | `ARTTurnManager`, spostamento forzato: l'inerzia di chi è sbilanciato si ferma allo spostamento | frase divisa in due |
| 2 | importante | `Terrain.Ice` taceva che chi scivola resta sbilanciato, e la soglia è più di un passo | `ARTTurnManager`, fase Move; `URTHexSimLibrary::ApplyIceSliding` | frase corretta |
| 3 | importante | `Status.Stunned` e `Status.Prone` tacevano che si perde ciò che era armato | `ARTTurnManager::DisarmPreparedReactions` | due frasi corrette |
| 4 | importante | `Structure.LowCover` contraddice il catalogo dei terreni, e la nota era vuota | `RT_TerrainCatalog_v0.1.md` §3; `URTHexCoverLibrary::BlocksTraversal` | nota nel documento; il catalogo resta com'è |
| 5 | importante | la modifica ai messaggi dei test non era committata, e due messaggi avevano ancora la forma vecchia | `git status` | committata; suite rifatta su quel commit |
| 6 | minore | `Terrain.Fire` e `Terrain.ShallowWater`: «chi ci entra» non copre chi c'è quando la superficie nasce | `ARTTurnManager`, `BornSurfaceCells`; `ARTUnit::PersistentWhileOnCell` | due frasi corrette |
| 7 | minore | `Status.Reveal` sembrava comprendere la reazione | il commento di `FRTPlannedIntent::bRevealed` | frase corretta |
| 8 | minore | `Status.Electrified` si confondeva con la scarica lineare | il produttore è la sola propagazione nel Cleanup | frase corretta |
| 9 | minore | tre commenti dicevano il falso su `Terrain.Void` e sulle chiavi dei ripari | `RT_TerrainCatalog_v0.1.md`: `Void` non c'è | commenti e nota del documento corretti |
| 10 | minore | `Terrain.HighGround` senza nota | la mappa dimostrativa la mette sul piano terra | nota nel documento |
| 11 | minore | «effetto in una riga» non ha un gate di lunghezza | misura delle voci: la più lunga è `Status.Unbalanced` | **non corretto qui**: il tetto si decide in F2, quando esiste la targa che lo mostra. Scritto nella PR |
| 12 | minore | spec, P3: `Status.Slow` e il divieto di corsa non passano da `GetEffectiveMoveRange` | `ARTUnit::GetEffectiveMoveRange`; `ARTTurnManager::MakeSimUnit` | spec corretta |

Il revisore ha lasciato **non verificato**, fra l'altro: se «attraversare» l'accidentato copre anche terminarvi lo scatto; il ramo conteso della spinta; se la propagazione elettrica colpisce chi sta sulla cella mirata quando quella non conduce. Chi scrive ha riletto dopo la prima e la terza: `IsTraversableByStep` in `Turn/RTMovementActionLibrary.cpp` vale per ogni cella del passo, quella d'arrivo compresa, e il salto ne è escluso; `CollectElectricPropagation` colpisce al passo zero anche su una cella che non conduce. Le due frasi reggono. Il ramo conteso della spinta resta **non verificato**: tocca un caso limite della frase di `Status.Unbalanced`.

**Ciò che il piano non copre, e lo dice:**
- la resa a schermo di una voce: non esiste ancora chi la mostri;
- la correttezza di una frase rispetto alla sua regola: nessun test la può verificare. La tabella «Da dove viene ogni frase» dice dove rileggerla, e l'accettazione è dell'autore;
- la build `Packaged`: `NOT RUN`. Il glossario è codice del modulo di gioco senza asset, e la prima fetta che lo mostra a schermo è quella che deve provarlo cotto.
