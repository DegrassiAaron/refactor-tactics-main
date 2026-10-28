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
	View.Lines.Add(Subject.DisplayName.ToString());

	FString Vita = FString::Printf(TEXT("PV %d/%d"), Subject.Card.Health, Subject.Card.MaxHealth);
	if (Subject.Card.Shield > 0)
	{
		Vita += FString::Printf(TEXT(", scudo %d"), Subject.Card.Shield);
	}
	View.Lines.Add(Vita);

	if (Subject.bShowFacing)
	{
		View.Lines.Add(FString::Printf(TEXT("Guarda verso %s."), *InspectDirectionName(Subject.Facing)));
	}

	for (const FRTStatusBadgeView& Badge : Subject.Statuses)
	{
		const FString Name = RTGlossary::NameFor(Badge.Tag).ToString();
		if (Name.IsEmpty()) { continue; }
		const FString Effect = RTGlossary::EffectFor(Badge.Tag).ToString();
		View.Lines.Add(Effect.IsEmpty() ? Name : FString::Printf(TEXT("%s - %s"), *Name, *Effect));
	}

	if (Subject.Kit.Num() > 0)
	{
		View.Lines.Add(TEXT("Kit"));
		for (const FRTActionTooltipView& Action : Subject.Kit)
		{
			const FString Description = Action.Description.ToString();
			View.Lines.Add(Description.IsEmpty() ? Action.Title.ToString()
				: FString::Printf(TEXT("%s - %s"), *Action.Title.ToString(), *Description));
			TArray<FString> Numbers;
			for (const FRTActionTooltipLine& Line : Action.Lines)
			{
				Numbers.Add(FString::Printf(TEXT("%s %s"), *Line.Label.ToString(), *Line.Value.ToString()));
			}
			if (!Action.Variant.IsEmpty()) { Numbers.Add(Action.Variant.ToString()); }
			if (Numbers.Num() > 0) { View.Lines.Add(TEXT("  ") + FString::Join(Numbers, TEXT(", "))); }
		}
	}

	if (Subject.Equipment.Num() > 0)
	{
		View.Lines.Add(TEXT("Equipaggiamento"));
		for (const FRTInspectEquipmentLine& Piece : Subject.Equipment)
		{
			View.Lines.Add(FString::Printf(TEXT("%s (%s)"), *Piece.Name.ToString(), InspectSlotName(Piece.Slot)));
			if (!Piece.Advantage.IsEmpty()) { View.Lines.Add(TEXT("  + ") + Piece.Advantage.ToString()); }
			if (!Piece.Drawback.IsEmpty()) { View.Lines.Add(TEXT("  - ") + Piece.Drawback.ToString()); }
		}
	}

	if (Subject.DeducedCooldowns.Num() > 0)
	{
		// L'intestazione dichiara che e' una STIMA (#3751): le ricariche vere dell'avversario non si leggono, e questa e'
		// ricavata da cio' che la squadra ha visto. Un effetto non visto che allunghi o accorci la ricarica la sbaglia.
		View.Lines.Add(TEXT("Ricariche (stima da cio' che hai visto)"));
		for (const FRTInspectDeducedCooldown& Row : Subject.DeducedCooldowns)
		{
			View.Lines.Add(FString::Printf(TEXT("  %s - vista usare al turno %d, di nuovo disponibile dal turno %d"),
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

bool URTInspectLibrary::IsInspectable(const FRTKnowledgeView& View, int32 StableUnitId)
{
	const FRTKnowledgeEntry* Entry = URTKnowledgeViewLibrary::FindEntry(View, StableUnitId);
	return Entry != nullptr && Entry->Visibility == ERTKnowledgeVisibility::Live;
}

FRTInspectView URTInspectLibrary::ResolveInspect(const FRTKnowledgeView& View, int32 StableUnitId, const FRTInspectSubject& Subject)
{
	return IsInspectable(View, StableUnitId) ? Compose(Subject) : FRTInspectView();
}
