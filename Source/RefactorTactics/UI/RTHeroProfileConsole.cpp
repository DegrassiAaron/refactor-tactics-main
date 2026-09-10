// `rt.Debug.HeroProfile` — mette il Profilo Tattico a schermo in PIE, con fixture di prova.
//
// 🔴 **Esiste perche' la verifica visiva non era eseguibile, non per comodita'.** I sette criteri
// osservabili di `#2652` — radar leggibile, due sagome a confronto, `0` al centro, 3/5/8 assi, sezioni
// assenti, UI scale, leggibilita' senza colore — richiedono che il widget sia **a schermo**. Senza un
// modo per mostrarlo, l'unica strada era costruire una scena a mano ogni volta, cioe' una verifica che
// nessuno rifa' e cha nessuno puo' confrontare con la precedente.
//
// ⚠️ **I valori sono INVENTATI, e devono restare tali.** Non sono il profilo di nessun eroe del catalogo:
// servono solo a produrre sagome distinguibili. Un giorno in cui questi numeri diventassero «i valori di
// Aevik» il comando avrebbe smesso di essere uno strumento e sarebbe diventato una seconda fonte —
// esattamente cio' che tutto il componente evita. Il catalogo lo possiede `tools/radar`.
//
// ⛔ **Sola presentazione**: non tocca `ARTTurnManager`, non muove unita', non legge stato di partita.
// Costruisce una `FRTHeroProfileView` da costanti e la passa al widget. Uno strumento d'ispezione che
// muove cio' che ispeziona produce sedute che non si possono confrontare fra loro (`RTDebugConsole.cpp`).
//
// Il comando vive qui e non in `Debug/` per il precedente del repository: `rt.Debug.Los` sta in
// `Map/RTHexLosConsole.cpp`, dov'e' il dominio che ispeziona. E per la stessa ragione **non** entra fra
// gli otto di `Debug.NamespaceDeclaresAllCommands`: quel DoD elenca cio' che deve esserci, non tutto cio'
// che c'e'.

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/GameplayStatics.h"
#include "UI/RTHeroProfileView.h"
#include "UI/RTHeroProfileWidget.h"

namespace RTHeroProfileConsole
{
	// ⚠️ Namespace NOMINATO: la unity build concatena questo file con gli altri di `UI/`.

	/** Il widget mostrato dall'ultima invocazione, per poterlo togliere senza ricordarselo. */
	static TWeakObjectPtr<URTHeroProfileWidget> GShownProfile;

	const TCHAR* const ProfileWidgetPath = TEXT("/Game/RT/UI/Profile/WBP_RT_HeroProfile.WBP_RT_HeroProfile_C");

	/** I sei assi normativi con i valori dati. ⚠️ Etichette di prova, non traduzioni. */
	TArray<FRTProfileRadarAxisView> MakeSixAxes(const TArray<int32>& Values)
	{
		static const TCHAR* const Ids[] = {
			TEXT("Radar.Profile.Offense"), TEXT("Radar.Profile.Durability"), TEXT("Radar.Profile.Mobility"),
			TEXT("Radar.Profile.Control"), TEXT("Radar.Profile.Support"), TEXT("Radar.Profile.Information") };
		static const TCHAR* const Labels[] = {
			TEXT("Offesa"), TEXT("Durabilita"), TEXT("Mobilita"),
			TEXT("Controllo"), TEXT("Supporto"), TEXT("Informazione") };

		TArray<FRTProfileRadarAxisView> Axes;
		for (int32 Index = 0; Index < Values.Num() && Index < 6; ++Index)
		{
			FRTProfileRadarAxisView Axis;
			Axis.AxisId = FName(Ids[Index]);
			Axis.Label = FText::FromString(Labels[Index]);
			Axis.Value = Values[Index];
			Axis.MaxValue = 10;
			Axes.Add(Axis);
		}
		return Axes;
	}

	/** `Count` assi generici: serve a provare che il widget non e' cablato a sei. */
	TArray<FRTProfileRadarAxisView> MakeNAxes(int32 Count)
	{
		TArray<FRTProfileRadarAxisView> Axes;
		for (int32 Index = 0; Index < Count; ++Index)
		{
			FRTProfileRadarAxisView Axis;
			Axis.AxisId = FName(*FString::Printf(TEXT("Radar.Probe.%d"), Index));
			Axis.Label = FText::FromString(FString::Printf(TEXT("Asse %d"), Index + 1));
			// Valori che cambiano per asse, cosi' la figura non e' un poligono regolare.
			Axis.Value = (Index * 3) % 11;
			Axis.MaxValue = 10;
			Axes.Add(Axis);
		}
		return Axes;
	}

	/** Profilo A: aggressivo e mobile. Valori di prova. */
	FRTHeroProfileView MakeProfileA()
	{
		FRTHeroProfileView View;
		View.HeroId = FName(TEXT("Hero.ProbeAlfa"));
		View.DisplayName = FText::FromString(TEXT("Sagoma Alfa"));
		View.PrimaryRoleId = FName(TEXT("Role.Probe.Primary"));
		View.PrimaryRoleLabel = FText::FromString(TEXT("Ruolo di prova"));
		View.StyleTags = { FText::FromString(TEXT("TagUno")), FText::FromString(TEXT("TagDue")) };
		View.RadarAxes = MakeSixAxes({ 9, 3, 8, 2, 4, 6 });
		View.AffinityId = FName(TEXT("Affinity.Probe"));
		View.AffinityLabel = FText::FromString(TEXT("Affinita di prova"));
		View.RangeLabel = FText::FromString(TEXT("Media"));
		View.LearningDifficulty = 2;
		View.MasteryDifficulty = 4;
		View.CombatIdentity = FText::FromString(TEXT("Apre lo scontro e detta il ritmo."));
		View.Strengths = { FText::FromString(TEXT("Pressione continua")), FText::FromString(TEXT("Riposizionamento")) };
		View.Tradeoffs = { FText::FromString(TEXT("Fragile se accerchiato")) };
		return View;
	}

	/**
	 * Profilo B: difensivo e di supporto.
	 *
	 * 🔵 **Deliberatamente diverso da A su ogni asse**, perche' il criterio da verificare e' «due sagome
	 * chiaramente diverse»: due profili simili renderebbero il confronto inconcludente proprio dove serve.
	 * E lascia fuori debolezza, proficiency e relazioni, cosi' si vede il fail-closed («—»).
	 */
	FRTHeroProfileView MakeProfileB()
	{
		FRTHeroProfileView View;
		View.HeroId = FName(TEXT("Hero.ProbeBeta"));
		View.DisplayName = FText::FromString(TEXT("Sagoma Beta"));
		View.PrimaryRoleId = FName(TEXT("Role.Probe.Guard"));
		View.PrimaryRoleLabel = FText::FromString(TEXT("Guardia di prova"));
		View.SecondaryRoleId = FName(TEXT("Role.Probe.Support"));
		View.SecondaryRoleLabel = FText::FromString(TEXT("Sostegno di prova"));
		View.RadarAxes = MakeSixAxes({ 2, 9, 3, 8, 9, 4 });
		View.CombatIdentity = FText::FromString(TEXT("Tiene la linea e protegge chi avanza."));
		View.Strengths = { FText::FromString(TEXT("Regge la pressione")) };
		return View;
	}

	/** Valori estremi: `0` deve finire al centro e `MaxValue` sul bordo. */
	FRTHeroProfileView MakeProfileExtremes()
	{
		FRTHeroProfileView View;
		View.HeroId = FName(TEXT("Hero.ProbeEstremi"));
		View.DisplayName = FText::FromString(TEXT("Estremi 0 e 10"));
		View.RadarAxes = MakeSixAxes({ 0, 10, 0, 10, 0, 10 });
		return View;
	}

	void RemoveShown()
	{
		if (URTHeroProfileWidget* Shown = GShownProfile.Get())
		{
			Shown->RemoveFromParent();
		}
		GShownProfile.Reset();
	}

	void Command(const TArray<FString>& Args, UWorld* World, FOutputDevice& Ar)
	{
		if (!World)
		{
			Ar.Log(TEXT("[RT] Nessun mondo attivo."));
			return;
		}

		const FString Mode = Args.Num() > 0 ? Args[0].ToUpper() : TEXT("A");

		if (Mode == TEXT("0") || Mode == TEXT("OFF"))
		{
			RemoveShown();
			Ar.Log(TEXT("[RT] Profilo Tattico rimosso."));
			return;
		}

		APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
		if (!PC)
		{
			// ⚠️ Fuori da PIE non c'e' un player controller: il comando lo dice invece di fallire muto.
			Ar.Log(TEXT("[RT] Nessun PlayerController: esegui in PIE."));
			return;
		}

		UClass* WidgetClass = LoadClass<URTHeroProfileWidget>(nullptr, ProfileWidgetPath);
		if (!WidgetClass)
		{
			Ar.Logf(TEXT("[RT] Impossibile caricare %s"), ProfileWidgetPath);
			return;
		}

		FRTHeroProfileView View;
		if (Mode == TEXT("B")) { View = MakeProfileB(); }
		else if (Mode == TEXT("E") || Mode == TEXT("ESTREMI")) { View = MakeProfileExtremes(); }
		else if (Mode.IsNumeric())
		{
			const int32 Count = FCString::Atoi(*Mode);
			View.HeroId = FName(TEXT("Hero.ProbeAssi"));
			View.DisplayName = FText::FromString(FString::Printf(TEXT("%d assi"), Count));
			View.RadarAxes = MakeNAxes(Count);
		}
		else { View = MakeProfileA(); }

		RemoveShown();

		URTHeroProfileWidget* Widget = CreateWidget<URTHeroProfileWidget>(PC, WidgetClass);
		if (!Widget)
		{
			Ar.Log(TEXT("[RT] CreateWidget ha restituito nullptr."));
			return;
		}

		Widget->SetProfileView(View);
		Widget->AddToViewport();
		GShownProfile = Widget;

		Ar.Logf(TEXT("[RT] Profilo Tattico a schermo: %s (%d assi)."),
			*View.DisplayName.ToString(), View.RadarAxes.Num());
	}
}

static FAutoConsoleCommandWithWorldArgsAndOutputDevice GRTHeroProfileShow(
	TEXT("rt.Debug.HeroProfile"),
	TEXT("Mostra il Profilo Tattico a schermo con fixture di prova, per la verifica visiva. "
		 "Argomenti: A (default) | B | E per i valori estremi | un numero di assi (3, 5, 8) | 0 per togliere. "
		 "I valori sono inventati e non appartengono a nessun eroe del catalogo. Strumento di sviluppo."),
	FConsoleCommandWithWorldArgsAndOutputDeviceDelegate::CreateStatic(&RTHeroProfileConsole::Command));
