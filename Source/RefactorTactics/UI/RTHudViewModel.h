#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTTurnRules.h"
#include "RTHudViewModel.generated.h"

class ARTTurnManager;
class ARTUnit;

/**
 * Lo stato dell'intestazione di partita: round, fase, tempo.
 *
 * Esiste perche' il §4.1 di `progettazione-hud.md` vieta ai widget di ricalcolare, e senza una vista
 * dichiarata l'unica alternativa e' che `WBP_RT_TurnHeader` legga `ARTTurnManager` da solo — cioe' che la
 * regola resti una disciplina da ricordare invece di una proprieta' della firma.
 */
USTRUCT(BlueprintType)
struct FRTMatchHeaderView
{
	GENERATED_BODY()

	/** Round corrente, 1-based come lo mostra il gioco. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Round = 0;

	/**
	 * Limite di round del **formato in vigore**, mai una costante.
	 *
	 * `0` significa «nessun limite dichiarato» e NON va mostrato come «su 0»: una partita senza formato non
	 * e' una partita gia' scaduta. E' la stessa distinzione che `ARTHUD` fa oggi in Canvas.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 RoundLimit = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	ERTMatchPhase Phase = ERTMatchPhase::Planning;

	/**
	 * Secondi che restano al Planning. **Negativo** quando la domanda non si applica — fuori dal Planning,
	 * o senza timer. Un `0.f` direbbe «scaduto adesso», che e' un'altra cosa.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	float PlanningSecondsRemaining = -1.f;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bResolving = false;
};

/**
 * Una unita' come la vede il pannello: salute, scudo, energia, identita'.
 *
 * Non contiene intenti ne' piani: quelli hanno gia' `FRTIntentView` e la loro privacy e' verificata la'
 * (invariante #6). Duplicarli qui significherebbe due filtri da tenere allineati.
 */
USTRUCT(BlueprintType)
struct FRTUnitCardView
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FName HeroId;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Health = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 MaxHealth = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Shield = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 Energy = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	int32 MaxEnergy = 0;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bIsAlly = false;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bAlive = false;
};

/**
 * Le viste che alimentano lo Screen HUD (§4.1 di `progettazione-hud.md`, CP 11.7).
 *
 * Statiche e pure per la stessa ragione per cui lo e' `ARTHUD::ComputePlannedHitMarks`: l'indipendenza dallo
 * stato del widget diventa una proprieta' della **firma**, non una regola che qualcuno deve ricordare. Un
 * widget che chiama queste funzioni non puo' sbagliare filtro, perche' il filtro non e' suo.
 *
 * ⚠️ Non e' il layer §4.2. Path, AoE, fuoco amico e le barre ancorate alle unita' restano in `ARTHUD`, dove
 * la spec li vuole — «non devono essere realizzati come grandi widget HUD statici».
 */
UCLASS()
class REFACTORTACTICS_API URTHudViewModel : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * L'intestazione. `TurnManager` nullo da' una vista neutra (round 0, nessun limite, timer negativo):
	 * un widget che parte prima del manager mostra «—», non un «Turno 0/0» che sembra un dato.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	static FRTMatchHeaderView BuildMatchHeader(const ARTTurnManager* TurnManager);

	/** La carta di una singola unita', vista da `PlayerTeamId`. Unita' nulla da' una carta vuota e non viva. */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	static FRTUnitCardView BuildUnitCard(const ARTUnit* Unit, int32 PlayerTeamId);

	/**
	 * Il roster laterale: **solo le unita' di `PlayerTeamId`**, nell'ordine in cui arrivano.
	 *
	 * La squadra avversaria non entra, e non per privacy — gli HP nemici sono gia' pubblici sopra le teste in
	 * `ARTHUD` — ma perche' il roster risponde a «chi comando io». Un elenco che mescola le due squadre
	 * costringe a leggere un colore per sapere di chi e' una riga, e la spec vuole la relazione di squadra
	 * distinguibile per **forma** prima che per tinta.
	 *
	 * Le unita' morte restano, con `bAlive = false`: sparire dall'elenco e' peggio che comparire barrato —
	 * il giocatore perde il conto di quanti ne aveva.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|HUD")
	static TArray<FRTUnitCardView> BuildTeamRoster(const TArray<ARTUnit*>& Units, int32 PlayerTeamId);
};
