#include "UI/RTInspect.h"

#include "UI/RTGlossary.h"

namespace
{
	/** Nomi distinti per file: il progetto usa unity build. */
	FString InspectDirectionName(ERTHexDirection Direction)
	{
		const UEnum* Enum = StaticEnum<ERTHexDirection>();
		return Enum ? Enum->GetNameStringByValue(static_cast<int64>(Direction)) : FString();
	}

	const TCHAR* InspectSlotName(ERTEquipmentSlot Slot)
	{
		switch (Slot)
		{
		case ERTEquipmentSlot::WeaponVariant: return TEXT("Variante d'arma");
		case ERTEquipmentSlot::Gadget: return TEXT("Gadget");
		case ERTEquipmentSlot::ReactionModule: return TEXT("Modulo");
		}
		return TEXT("");
	}
}

FRTInspectView URTInspectLibrary::Compose(const FRTInspectSubject& Subject)
{
	FRTInspectView View;
	auto AddLine = [&View](const FString& Line) { View.Lines.Add(Line); };
	AddLine(Subject.DisplayName.ToString());

	FString Vita = FString::Printf(TEXT("PV %d/%d"), Subject.Card.Health, Subject.Card.MaxHealth);
	if (Subject.Card.Shield > 0)
	{
		Vita += FString::Printf(TEXT(", scudo %d"), Subject.Card.Shield);
	}
	AddLine(Vita);

	if (Subject.bShowFacing)
	{
		AddLine(FString::Printf(TEXT("Guarda verso %s."), *InspectDirectionName(Subject.Facing)));
	}

	for (const FRTStatusBadgeView& Badge : Subject.Statuses)
	{
		const FString Name = RTGlossary::NameFor(Badge.Tag).ToString();
		if (Name.IsEmpty()) { continue; }
		const FString Effect = RTGlossary::EffectFor(Badge.Tag).ToString();
		AddLine(Effect.IsEmpty() ? Name : FString::Printf(TEXT("%s - %s"), *Name, *Effect));
	}

	// ⛔ Nessuna sezione «Kit»: il kit di chi e' a fuoco lo mostra la barra delle skill, in sola lettura (`#3748`).

	if (Subject.Equipment.Num() > 0)
	{
		AddLine(TEXT("Equipaggiamento"));
		for (const FRTInspectEquipmentLine& Piece : Subject.Equipment)
		{
			AddLine(FString::Printf(TEXT("%s (%s)"), *Piece.Name.ToString(), InspectSlotName(Piece.Slot)));
			if (!Piece.Advantage.IsEmpty()) { AddLine(TEXT("  + ") + Piece.Advantage.ToString()); }
			if (!Piece.Drawback.IsEmpty()) { AddLine(TEXT("  - ") + Piece.Drawback.ToString()); }
		}
	}

	if (Subject.DeducedCooldowns.Num() > 0)
	{
		// L'intestazione dichiara che e' una STIMA (#3751): le ricariche vere dell'avversario non si leggono, e questa e'
		// ricavata da cio' che la squadra ha visto. Un effetto non visto che allunghi o accorci la ricarica la sbaglia.
		AddLine(TEXT("Ricariche (stima da cio' che hai visto)"));
		for (const FRTInspectDeducedCooldown& Row : Subject.DeducedCooldowns)
		{
			AddLine(FString::Printf(TEXT("  %s - vista usare al turno %d, di nuovo disponibile dal turno %d"),
				*Row.Title.ToString(), Row.LastSeenTurn, Row.AvailableFromTurn));
		}
	}
	return View;
}

bool URTInspectLibrary::MakeDeducedCooldown(FName ActionId, const FText& Title, int32 CooldownTurns, int32 LastSeenTurn,
	FRTInspectDeducedCooldown& OutRow)
{
	if (ActionId.IsNone() || Title.IsEmpty() || CooldownTurns <= 0 || LastSeenTurn <= 0)
	{
		return false;
	}
	OutRow.ActionId = ActionId;
	OutRow.Title = Title;
	OutRow.LastSeenTurn = LastSeenTurn;
	OutRow.AvailableFromTurn = LastSeenTurn + CooldownTurns;
	return true;
}

const FRTKnowledgeEntry* URTInspectLibrary::LiveEntry(const FRTKnowledgeView& View, int32 StableUnitId)
{
	const FRTKnowledgeEntry* Entry = URTKnowledgeViewLibrary::FindEntry(View, StableUnitId);
	return Entry != nullptr && Entry->Visibility == ERTKnowledgeVisibility::Live ? Entry : nullptr;
}

bool URTInspectLibrary::IsInspectable(const FRTKnowledgeView& View, int32 StableUnitId)
{
	return LiveEntry(View, StableUnitId) != nullptr;
}

FRTInspectView URTInspectLibrary::ResolveInspect(const FRTKnowledgeView& View, int32 StableUnitId, const FRTInspectSubject& Subject)
{
	return IsInspectable(View, StableUnitId) ? Compose(Subject) : FRTInspectView();
}

bool URTInspectLibrary::ShouldShowCard(ERTPointerContext Context, bool bPinned)
{
	if (!bPinned)
	{
		return false;
	}
	switch (Context)
	{
	// In `ReactionWindow` e in `Modal` il Back e' di quei passi (`ResolveBack` li mette prima di `Inspector`): la scheda aspetta.
	case ERTPointerContext::ReactionWindow:
	case ERTPointerContext::Modal:
		return false;

	// Nessun `default:`, come `ContextAllowsFocus` e `ContextAllowsInspectedHover`: un contesto nuovo deve dire QUI se nasconde la scheda,
	// e il compilatore lo chiede. Una lista negativa renderebbe visibile, per omissione, ogni contesto che nessuno ha guardato.
	case ERTPointerContext::IdleSelection:
	case ERTPointerContext::Planning:
	case ERTPointerContext::Pathing:
	case ERTPointerContext::Targeting:
	case ERTPointerContext::Facing:
	case ERTPointerContext::ResolutionPlayback:
		return true;
	}
	return false;
}

int32 URTInspectLibrary::ClampFirstLine(int32 TotalLines, int32 MaxLines, int32 FirstLine)
{
	if (MaxLines <= 0 || TotalLines <= MaxLines)
	{
		return 0;
	}
	// A fondo corsa il marcatore in basso non serve e il suo slot va a quello in alto: il corpo ha `MaxLines - 1` righe.
	return FMath::Clamp(FirstLine, 0, TotalLines - (MaxLines - 1));
}

FRTInspectView URTInspectLibrary::VisibleWindow(const FRTInspectView& View, int32 FirstLine, int32 MaxLines)
{
	const int32 Total = View.Lines.Num();
	if (MaxLines <= 0)
	{
		return FRTInspectView();
	}
	if (Total <= MaxLines)
	{
		return View;
	}

	FRTInspectView Out;
	auto AddRow = [&View, &Out](int32 Source)
	{
		// Totale per costruzione: coi parametri di questa funzione `Source` sta sempre dentro `Lines`, ma un errore di conto non deve
		// fermare il motore. Una finestra sbagliata cade nel test; un indice fuori dall'array cadrebbe sul processo intero, e una suite
		// con un test che non completa esce comunque con zero.
		if (!View.Lines.IsValidIndex(Source)) { return; }
		Out.Lines.Add(View.Lines[Source]);
	};

	// Sotto le tre righe nemmeno i marcatori entrano: taglio secco dalle prime, dichiarato nel commento della funzione.
	if (MaxLines < 3)
	{
		for (int32 Source = 0; Source < MaxLines; ++Source) { AddRow(Source); }
		return Out;
	}

	const int32 First = ClampFirstLine(Total, MaxLines, FirstLine);
	const bool bTopMarker = First > 0;
	// Il marcatore in basso serve se il corpo, senza di esso, non arriva all'ultima riga.
	const bool bBottomMarker = First + (MaxLines - (bTopMarker ? 1 : 0)) < Total;
	const int32 Body = MaxLines - (bTopMarker ? 1 : 0) - (bBottomMarker ? 1 : 0);

	if (bTopMarker)
	{
		Out.Lines.Add(FString::Printf(TEXT("… %d righe sopra"), First));
	}
	for (int32 Source = First; Source < First + Body; ++Source) { AddRow(Source); }
	if (bBottomMarker)
	{
		Out.Lines.Add(FString::Printf(TEXT("… e altre %d righe"), Total - (First + Body)));
	}
	return Out;
}
