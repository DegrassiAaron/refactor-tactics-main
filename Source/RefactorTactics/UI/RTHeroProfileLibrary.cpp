#include "UI/RTHeroProfileLibrary.h"

#include "Ability/RTHeroData.h"

FRTHeroProfileView URTHeroProfileLibrary::BuildBaseProfile(const URTHeroData* Hero)
{
	FRTHeroProfileView View;

	if (!Hero)
	{
		// Una view vuota e' la risposta giusta: la scheda sa gia' nascondere le sezioni assenti, mentre un
		// profilo riempito con segnaposto sarebbe indistinguibile da un profilo reale povero.
		return View;
	}

	View.HeroId = Hero->HeroId;
	View.DisplayName = Hero->DisplayName;
	View.AffinityId = Hero->Affinity;
	View.WeaknessId = Hero->Weakness;

	// ⛔ Qui NON si aggiunge altro. `URTHeroData` possiede anche statistiche base (salute, movimento,
	// vista, udito), ma il Profilo Tattico non le espone come campi: mostrarle richiederebbe decidere
	// quale sia la loro forma player-facing, che e' una scelta di design con un owner diverso.
	//
	// ⛔ E soprattutto: nessuna label viene sintetizzata dagli ID, nessun asse radar viene calcolato,
	// nessuna proficiency viene dedotta dall'affinita'.

	return View;
}

bool URTHeroProfileLibrary::ValidateProfileView(const FRTHeroProfileView& View, TArray<FString>& OutDiagnostics)
{
	OutDiagnostics.Reset();

	// ---- Assi del radar -------------------------------------------------------------------------
	//
	// L'identita' dell'asse si controlla per prima: senza `AxisId` non esiste il concetto di duplicato,
	// e due assi anonimi sarebbero indistinguibili pur disegnando due raggi diversi.
	TSet<FName> SeenAxisIds;
	for (int32 Index = 0; Index < View.RadarAxes.Num(); ++Index)
	{
		const FRTProfileRadarAxisView& Axis = View.RadarAxes[Index];

		if (Axis.AxisId.IsNone())
		{
			OutDiagnostics.Add(FString::Printf(TEXT("asse %d: AxisId assente"), Index));
		}
		else if (SeenAxisIds.Contains(Axis.AxisId))
		{
			OutDiagnostics.Add(FString::Printf(TEXT("asse %d: AxisId duplicato (%s)"), Index, *Axis.AxisId.ToString()));
		}
		else
		{
			SeenAxisIds.Add(Axis.AxisId);
		}

		if (Axis.MaxValue <= 0)
		{
			// ⚠️ Fermarsi qui per questo asse: con un fondoscala non positivo il controllo di range che
			// segue direbbe cose senza senso (`Value > MaxValue` sarebbe vero per qualunque valore utile),
			// e produrrebbe una seconda diagnosi che descrive lo stesso difetto.
			OutDiagnostics.Add(FString::Printf(TEXT("asse %d (%s): MaxValue non positivo (%d)"),
				Index, *Axis.AxisId.ToString(), Axis.MaxValue));
			continue;
		}

		if (Axis.Value < 0 || Axis.Value > Axis.MaxValue)
		{
			OutDiagnostics.Add(FString::Printf(TEXT("asse %d (%s): Value %d fuori da [0, %d]"),
				Index, *Axis.AxisId.ToString(), Axis.Value, Axis.MaxValue));
		}
	}

	// ---- Proficiency elementali -----------------------------------------------------------------
	//
	// Un elemento con due gradi diversi non e' un dato ricco: e' un dato contraddittorio, e la scheda non
	// ha modo di scegliere quale mostrare.
	TSet<FName> SeenProficiencyElements;
	for (int32 Index = 0; Index < View.ElementalProficiencies.Num(); ++Index)
	{
		const FRTElementProficiencyView& Proficiency = View.ElementalProficiencies[Index];

		if (Proficiency.ElementId.IsNone())
		{
			OutDiagnostics.Add(FString::Printf(TEXT("proficiency %d: ElementId assente"), Index));
		}
		else if (SeenProficiencyElements.Contains(Proficiency.ElementId))
		{
			OutDiagnostics.Add(FString::Printf(TEXT("proficiency %d: elemento duplicato (%s)"),
				Index, *Proficiency.ElementId.ToString()));
		}
		else
		{
			SeenProficiencyElements.Add(Proficiency.ElementId);
		}

		// Un grado senza elemento, o un'etichetta di grado senza il grado, sono meta' dato: la sezione
		// mostrerebbe un testo che non si puo' ricondurre a niente.
		if (Proficiency.GradeId.IsNone() && !Proficiency.GradeLabel.IsEmpty())
		{
			OutDiagnostics.Add(FString::Printf(TEXT("proficiency %d (%s): GradeLabel senza GradeId"),
				Index, *Proficiency.ElementId.ToString()));
		}
	}

	// ---- Relazioni elementali -------------------------------------------------------------------
	//
	// Il duplicato qui e' la COPPIA: lo stesso elemento puo' legittimamente comparire in due relazioni
	// diverse (sintonia con l'acqua, contrasto con l'acqua non avrebbe senso, ma sintonia + un'altra
	// relazione si), mentre la stessa relazione ripetuta sullo stesso elemento e' rumore.
	TSet<TPair<FName, FName>> SeenRelations;
	for (int32 Index = 0; Index < View.ElementRelations.Num(); ++Index)
	{
		const FRTElementRelationView& Relation = View.ElementRelations[Index];

		if (Relation.ElementId.IsNone() || Relation.RelationId.IsNone())
		{
			OutDiagnostics.Add(FString::Printf(TEXT("relazione %d: ElementId o RelationId assente"), Index));
			continue;
		}

		const TPair<FName, FName> Key(Relation.ElementId, Relation.RelationId);
		if (SeenRelations.Contains(Key))
		{
			OutDiagnostics.Add(FString::Printf(TEXT("relazione %d: coppia duplicata (%s / %s)"),
				Index, *Relation.ElementId.ToString(), *Relation.RelationId.ToString()));
		}
		else
		{
			SeenRelations.Add(Key);
		}
	}

	// ---- Combinazioni incoerenti ----------------------------------------------------------------
	//
	// ⚠️ Un'etichetta senza il suo ID e' il caso che conta davvero: significa che qualcuno ha scritto il
	// testo a mano invece di riceverlo da chi possiede il dato, ed e' esattamente il modo in cui una
	// seconda fonte di verita' entra in una UI.
	if (View.AffinityId.IsNone() && !View.AffinityLabel.IsEmpty())
	{
		OutDiagnostics.Add(TEXT("AffinityLabel presente senza AffinityId"));
	}

	if (View.WeaknessId.IsNone() && !View.WeaknessLabel.IsEmpty())
	{
		OutDiagnostics.Add(TEXT("WeaknessLabel presente senza WeaknessId"));
	}

	if (View.PrimaryRoleId.IsNone() && !View.PrimaryRoleLabel.IsEmpty())
	{
		OutDiagnostics.Add(TEXT("PrimaryRoleLabel presente senza PrimaryRoleId"));
	}

	// Un ruolo secondario senza primario non e' un profilo parziale: e' un profilo che ha saltato il
	// campo principale, e la scheda lo mostrerebbe come se il secondario fosse l'identita' dell'eroe.
	if (View.PrimaryRoleId.IsNone() && !View.SecondaryRoleId.IsNone())
	{
		OutDiagnostics.Add(TEXT("SecondaryRoleId presente senza PrimaryRoleId"));
	}

	return OutDiagnostics.Num() == 0;
}
