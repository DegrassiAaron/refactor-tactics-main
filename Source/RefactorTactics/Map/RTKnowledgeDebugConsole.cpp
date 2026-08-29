#include "CoreMinimal.h"

#if !UE_BUILD_SHIPPING

#include "HAL/IConsoleManager.h"
#include "Map/RTHexMapActor.h"
#include "Map/RTHexMapAsset.h"
#include "Perception/RTTeamKnowledge.h"
#include "Kismet/GameplayStatics.h"
#include "Turn/RTTurnManager.h"

/**
 * `rt.Debug.Knowledge [team]` — accende i VOLUMI DI CONOSCENZA: ogni cella porta un prisma la cui ALTEZZA
 * dice cosa la squadra sa di lei.
 *
 * ```text
 * 3/3 H   osservata ORA        250 uu
 * 2/3 H   ricordo               167 uu
 * 1/3 H   mai vista              83 uu   <- l'unica cosa che il gioco NON disegna
 * ```
 *
 * **Perche' esiste.** `ApplyKnowledgeVeil` dice gia' i tre stati, ma li dice con mezzi che in due casi non
 * si leggono: una cella mai vista **non viene disegnata** ([D-225]), quindi la sua assenza e' indistinguibile
 * da una cella che non esiste; e su `Relief`, `Blockers` ed `EdgeFeatures` il velo *«NASCONDE e basta, non
 * attenua»* — quelle famiglie non portano custom data — quindi **ricordo e osservazione coincidono**. Il
 * canale ALTEZZA non passa dal materiale e li separa dove il colore non arriva.
 *
 * ⛔ **Non contraddice [D-225], e la ragione e' l'ATTORE.** Quella decisione vieta la «mappa nera» al
 * GIOCATORE, in partita. Questo comando serve a chi sviluppa, e vedere cio' che il giocatore non vede e'
 * esattamente il suo mestiere. Il confine e' l'intero file dentro `#if !UE_BUILD_SHIPPING`.
 *
 * ⚠️ **Che la guardia tenga lo dice la BUILD, non un test**: una suite gira in Development, dove il comando
 * esiste per definizione, quindi nessuna asserzione puo' parlare di Shipping. L'oracolo e' il gate `G1` del
 * DoD v0.1.
 *
 * ✅ **Misurato il 2026-08-29**, e non solo compilando: cercate le stringhe nei due binari,
 * `RefactorTactics-Win64-Shipping.exe` **non contiene** ne' `rt.Debug.Knowledge` ne' `RT_KnowledgeVolume`,
 * mentre `UnrealEditor-RefactorTactics.dll` porta entrambe. ⚠️ E il controllo di sanita' e' la meta' che
 * conta: `rt.Debug.DrawCells` e' **presente** nello stesso binario Shipping — quindi l'assenza qui e' il
 * confinamento e non un artefatto della ricerca. *(Che `DrawCells` non sia confinato e' una differenza
 * deliberata: mostra superficie e blocchi, cioe' la mappa, non cio' che una squadra sa.)*
 *
 * ⚠️ **La quarta frazione non ha uno stato.** Il vocabolario ne porta quattro — `1/3 · 1/2 · 2/3 · 3/3` — e
 * gli stati sono tre: `1/2` esiste perche' e' stata chiesta, e resta **non assegnata**. Darle un significato
 * vorrebbe dire inventare un quarto stato che `FRTTeamKnowledge` non ha (porta `VisibleCells` ed
 * `ExploredCells`, e il terzo stato e' l'assenza da entrambe) — il difetto che [D-146] registra per una
 * tassonomia che non corrispondeva al modello reale.
 *
 * Sola lettura: non tocca lo stato di gioco. Namespace `rt.Debug.*` di CP 11.4 (#80), come
 * `rt.Debug.DrawCells` accanto a cui vive.
 */
static void RTDebugKnowledgeCommand(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
{
	if (!World)
	{
		Ar.Log(TEXT("[RT] Nessun mondo attivo."));
		return;
	}

	ARTHexMapActor* HexMap = ARTHexMapActor::FindInWorld(World);
	if (!HexMap)
	{
		Ar.Log(TEXT("[RT] Nessuna mappa esagonale nel livello."));
		return;
	}

	// Nessun argomento = interruttore sul team 0. Con argomento, il team di cui mostrare la conoscenza —
	// perche' «cosa si sa» non e' una proprieta' della mappa ma di CHI guarda, ed e' la stessa domanda che
	// `ApplyKnowledgeVeil` lascia deliberatamente al chiamante.
	const bool bEnable = !HexMap->IsKnowledgeDebugEnabled() || Args.Num() > 0;
	int32 TeamId = 0;
	if (Args.Num() > 0)
	{
		TeamId = FCString::Atoi(*Args[0]);
	}

	FRTTeamKnowledge Knowledge;
	if (bEnable)
	{
		// La conoscenza si CHIEDE al TurnManager, che ne e' l'autorita': ricostruirla qui creerebbe la
		// seconda verita' su cosa una squadra sa, e le due divergerebbero al primo cambio di regola.
		ARTTurnManager* Turn = Cast<ARTTurnManager>(
			UGameplayStatics::GetActorOfClass(World, ARTTurnManager::StaticClass()));
		if (!Turn)
		{
			Ar.Log(TEXT("[RT] Nessun TurnManager: senza autorita' sulla conoscenza non c'e' niente da mostrare."));
			return;
		}
		Knowledge = Turn->KnowledgeForTeamPublic(TeamId);
	}

	HexMap->SetKnowledgeDebugEnabled(bEnable, Knowledge);

	if (!bEnable)
	{
		Ar.Log(TEXT("[RT] Volumi di conoscenza: spenti."));
		return;
	}

	int32 Hidden = 0;
	int32 Remembered = 0;
	int32 Lit = 0;
	HexMap->GetKnowledgeDebugCounts(Hidden, Remembered, Lit);
	Ar.Logf(TEXT("[RT] Volumi di conoscenza (team %d): %d osservate (3/3) · %d ricordate (2/3) · %d mai viste (1/3)."),
		TeamId, Lit, Remembered, Hidden);
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTDebugKnowledge(
	TEXT("rt.Debug.Knowledge"),
	TEXT("Volumi di conoscenza: 3/3 osservata, 2/3 ricordo, 1/3 mai vista. Argomento opzionale: il team."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTDebugKnowledgeCommand));

#endif // !UE_BUILD_SHIPPING
