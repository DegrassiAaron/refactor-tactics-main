#include "UI/RTGlance.h"

#include "UI/RTGlossary.h"

namespace
{
	FString DirectionName(ERTHexDirection Direction)
	{
		const UEnum* Enum = StaticEnum<ERTHexDirection>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Direction)) : FString();
	}

	/** «Nome - effetto», oppure il solo nome se l'effetto manca, oppure vuoto se la chiave non ha voce. */
	FString NameAndEffect(const FName& Key)
	{
		const FString Name = RTGlossary::NameFor(Key).ToString();
		if (Name.IsEmpty()) { return FString(); }
		const FString Effect = RTGlossary::EffectFor(Key).ToString();
		return Effect.IsEmpty() ? Name : FString::Printf(TEXT("%s - %s"), *Name, *Effect);
	}
}

FRTGlanceView URTGlanceLibrary::ComposeUnit(const FRTGlanceUnitSubject& Subject)
{
	FRTGlanceView View;
	View.Lines.Add(Subject.DisplayName.ToString());
	if (Subject.bShowFacing)
	{
		View.Lines.Add(FString::Printf(TEXT("Guarda verso %s."), *DirectionName(Subject.Facing)));
	}

	TArray<FString> StatusLines;
	for (const FRTStatusBadgeView& Badge : Subject.Statuses)
	{
		const FString Line = NameAndEffect(Badge.Tag);
		if (!Line.IsEmpty()) { StatusLines.Add(Line); }
	}

	const int32 Room = MaxUnitLines - View.Lines.Num();
	if (StatusLines.Num() <= Room)
	{
		View.Lines.Append(StatusLines);
		return View;
	}

	// Il taglio non e' muto: l'ultima riga dice quanti stati restano fuori.
	const int32 Shown = FMath::Max(0, Room - 1);
	for (int32 i = 0; i < Shown; ++i) { View.Lines.Add(StatusLines[i]); }
	View.Lines.Add(FString::Printf(TEXT("E altri %d stati."), StatusLines.Num() - Shown));
	View.bTruncated = true;
	return View;
}

FRTGlanceView URTGlanceLibrary::ComposeCell(const FRTGlanceCellSubject& Subject)
{
	FRTGlanceView View;

	const FString Surface = NameAndEffect(RTGlossary::KeyForSurface(Subject.Surface));
	if (!Surface.IsEmpty()) { View.Lines.Add(Surface); }

	if (Subject.EntryCost != 1)
	{
		View.Lines.Add(FString::Printf(TEXT("Costo d'ingresso: %d."), Subject.EntryCost));
	}

	View.Lines.Add(FString::Printf(TEXT("Piano %d."), Subject.Layer));

	TArray<FString> Covers;
	for (const FRTGlanceCoverSide& Side : Subject.Covers)
	{
		const FString Name = RTGlossary::NameFor(RTGlossary::KeyForCover(Side.Type)).ToString();
		if (!Name.IsEmpty()) { Covers.Add(FString::Printf(TEXT("%s: %s"), *DirectionName(Side.Edge), *Name)); }
	}
	if (Covers.Num() > 0)
	{
		View.Lines.Add(FString::Printf(TEXT("Ripari - %s."), *FString::Join(Covers, TEXT(", "))));
	}

	TArray<FString> Marks;
	if (Subject.bBlocksLineOfSight) { Marks.Add(TEXT("Blocca la vista.")); }
	if (Subject.bIsObjective) { Marks.Add(TEXT("Obiettivo.")); }
	if (Subject.bHasDoor) { Marks.Add(TEXT("Porta.")); }
	if (Marks.Num() > 0) { View.Lines.Add(FString::Join(Marks, TEXT(" "))); }

	return View;
}

bool URTGlanceLibrary::UnitMayBeNamed(const FRTKnowledgeView& View, const FRTGlanceTarget& Target)
{
	if (Target.Kind != ERTGlanceKind::Unit)
	{
		return false;
	}
	const FRTKnowledgeEntry* Entry = URTKnowledgeViewLibrary::FindEntry(View, Target.StableUnitId);
	return Entry != nullptr && Entry->Visibility == ERTKnowledgeVisibility::Live;
}

FRTGlanceView URTGlanceLibrary::ResolveGlance(const FRTKnowledgeView& View, const FRTTeamKnowledge& Knowledge,
	const FRTGlanceTarget& Target, const FRTGlanceUnitSubject& Unit, const FRTGlanceCellSubject& CellAtTarget)
{
	if (!Target.IsValid())
	{
		return FRTGlanceView();
	}

	if (UnitMayBeNamed(View, Target))
	{
		return ComposeUnit(Unit);
	}
	// Degrado (P10): lo sguardo cade sulla cella, e vale quel che vale per una cella vuota.

	if (!Target.bHasCell || !Knowledge.VisibleCells.Contains(Target.Cell))
	{
		return FRTGlanceView();
	}
	return ComposeCell(CellAtTarget);
}

TArray<FString> URTGlanceLibrary::WrapLines(const TArray<FString>& Lines, int32 MaxChars)
{
	TArray<FString> Out;
	for (const FString& Line : Lines)
	{
		TArray<FString> Words;
		Line.ParseIntoArray(Words, TEXT(" "), /*bCullEmpty=*/ true);
		FString Current;
		for (const FString& Word : Words)
		{
			if (Current.IsEmpty())
			{
				Current = Word;
			}
			else if (Current.Len() + 1 + Word.Len() <= MaxChars)
			{
				Current += TEXT(" ") + Word;
			}
			else
			{
				Out.Add(Current);
				Current = Word;
			}
		}
		Out.Add(Current); // anche vuota: una riga vuota in ingresso resta una riga
	}
	return Out;
}
