#include "UI/RTHeroProfileWidget.h"

#include "Components/TextBlock.h"
#include "UI/RTHeroRadarWidget.h"

namespace RTHeroProfileWidgetText
{
	// ⚠️ Namespace NOMINATO: la unity build concatena questo file con gli altri della cartella, e un
	// nome generico in un namespace anonimo e' la collisione gia' pagata da #2397.

	/** Unisce testi non vuoti con un separatore; `FText` vuoto se non resta niente da mostrare. */
	FText JoinNonEmpty(const TArray<FText>& Values, const TCHAR* Separator)
	{
		TArray<FString> Parts;
		for (const FText& Value : Values)
		{
			if (!Value.IsEmpty())
			{
				Parts.Add(Value.ToString());
			}
		}

		if (Parts.Num() == 0)
		{
			return FText::GetEmpty();
		}

		return FText::FromString(FString::Join(Parts, Separator));
	}

	/**
	 * Scrive un testo su un `UTextBlock` che puo' non esistere, mostrando il segnaposto quando il dato
	 * manca.
	 *
	 * 🔴 **Non collassa il widget e non lo nasconde**: la visibilita' e' una scelta di layout che
	 * appartiene al `.uasset`, e deciderla qui sovrascriverebbe in silenzio l'authoring di chi disegna
	 * la scheda. Il codice dice cosa c'e' scritto, non cosa si vede.
	 */
	void SetTextOrPlaceholder(UTextBlock* Block, const FText& Value, const FText& Placeholder)
	{
		if (!Block)
		{
			return;
		}

		Block->SetText(Value.IsEmpty() ? Placeholder : Value);
	}
}

FText URTHeroProfileWidget::GetAbsentValueText()
{
	// Un trattino lungo, non uno zero e non una stringa vuota: «non dichiarato» deve essere leggibile
	// come tale, e uno spazio bianco sembrerebbe un difetto di layout.
	return FText::FromString(TEXT("—"));
}

void URTHeroProfileWidget::SetProfileView(const FRTHeroProfileView& InProfileView)
{
	ProfileView = InProfileView;
	RefreshBoundWidgets();
	OnProfileViewChanged();
}

void URTHeroProfileWidget::NativeConstruct()
{
	Super::NativeConstruct();

	// ⚠️ La view puo' essere arrivata PRIMA che i widget esistessero: `SetProfileView` su una scheda non
	// ancora costruita trova i `BindWidget` a `nullptr` e non scrive niente. Senza questa riga la
	// scheda resterebbe col testo di default proprio nel caso piu' comune — costruita gia' popolata.
	RefreshBoundWidgets();
}

void URTHeroProfileWidget::RefreshBoundWidgets()
{
	using namespace RTHeroProfileWidgetText;

	const FText Absent = GetAbsentValueText();

	SetTextOrPlaceholder(HeroName, ProfileView.DisplayName, Absent);

	// I due ruoli su una riga sola. ⚠️ Si mostrano le ETICHETTE, mai gli ID: `Role.Controller` e'
	// un identificatore tecnico, e stamparlo spaccerebbe una chiave per una traduzione.
	SetTextOrPlaceholder(RoleLine,
		JoinNonEmpty({ ProfileView.PrimaryRoleLabel, ProfileView.SecondaryRoleLabel }, TEXT(" · ")),
		Absent);

	SetTextOrPlaceholder(StyleTagLine, JoinNonEmpty(ProfileView.StyleTags, TEXT(" · ")), Absent);

	// 🔵 Affinita' e debolezza usano la label e NON ricadono sull'ID: un profilo con l'ID ma senza
	// etichetta e' un dato incompleto che `ValidateProfileView` gia' segnala, e mostrare l'ID lo
	// nasconderebbe facendolo sembrare un testo.
	SetTextOrPlaceholder(AffinityText, ProfileView.AffinityLabel, Absent);
	SetTextOrPlaceholder(WeaknessText, ProfileView.WeaknessLabel, Absent);

	// Portata e difficolta'. `0` significa «non dichiarata», quindi non entra nella riga: mostrarlo
	// insegnerebbe al giocatore una difficolta' che nessuno ha stabilito.
	{
		TArray<FText> Parts;
		if (!ProfileView.RangeLabel.IsEmpty())
		{
			Parts.Add(ProfileView.RangeLabel);
		}
		if (ProfileView.LearningDifficulty > 0)
		{
			Parts.Add(FText::AsNumber(ProfileView.LearningDifficulty));
		}
		if (ProfileView.MasteryDifficulty > 0)
		{
			Parts.Add(FText::AsNumber(ProfileView.MasteryDifficulty));
		}
		SetTextOrPlaceholder(RangeAndDifficulty, JoinNonEmpty(Parts, TEXT(" · ")), Absent);
	}

	SetTextOrPlaceholder(CombatIdentity, ProfileView.CombatIdentity, Absent);
	SetTextOrPlaceholder(StrengthsText, JoinNonEmpty(ProfileView.Strengths, TEXT("\n")), Absent);
	SetTextOrPlaceholder(TradeoffsText, JoinNonEmpty(ProfileView.Tradeoffs, TEXT("\n")), Absent);

	if (HeroRadar)
	{
		// ⛔ Gli assi si passano SEMPRE, anche quando sono zero o non rappresentabili: il radar sa gia'
		// rifiutarsi di disegnare (`ComputeRadarPoints` e' fail-closed), mentre filtrarli qui
		// creerebbe una seconda regola su cosa sia disegnabile — e due regole divergono.
		HeroRadar->SetRadarAxes(ProfileView.RadarAxes);
	}
}

bool URTHeroProfileWidget::HasRadar() const
{
	if (ProfileView.RadarAxes.Num() == 0)
	{
		return false;
	}

	// La stessa domanda che si fa il renderer, fatta alla stessa funzione: se qui rispondessimo con un
	// controllo scritto a mano, la scheda potrebbe mostrare una sezione che il radar poi rifiuta di
	// disegnare — un riquadro vuoto invece di una sezione assente.
	TArray<FVector2D> Points;
	return URTHeroRadarWidget::ComputeRadarPoints(ProfileView.RadarAxes, FVector2D::ZeroVector, 1.f, Points);
}

bool URTHeroProfileWidget::HasPrimaryRole() const
{
	return !ProfileView.PrimaryRoleId.IsNone();
}

bool URTHeroProfileWidget::HasSecondaryRole() const
{
	return !ProfileView.SecondaryRoleId.IsNone();
}

bool URTHeroProfileWidget::HasAffinity() const
{
	return !ProfileView.AffinityId.IsNone();
}

bool URTHeroProfileWidget::HasWeakness() const
{
	return !ProfileView.WeaknessId.IsNone();
}

bool URTHeroProfileWidget::HasElementalProficiencies() const
{
	// ⛔ Volutamente NON guarda `AffinityId`. La proficiency e' derivata dalle capability del kit (`#995`),
	// l'affinita' e' dichiarata dal catalogo: legarle qui reintrodurrebbe in una riga la deduzione che
	// tutto il resto del componente evita.
	return ProfileView.ElementalProficiencies.Num() > 0;
}

bool URTHeroProfileWidget::HasElementRelations() const
{
	return ProfileView.ElementRelations.Num() > 0;
}

bool URTHeroProfileWidget::HasDifficultyRatings() const
{
	// `0` significa «non dichiarata», non «facilissimo»: mostrarlo come un valore insegnerebbe al
	// giocatore una difficolta' che nessuno ha stabilito.
	return ProfileView.LearningDifficulty > 0 || ProfileView.MasteryDifficulty > 0;
}
