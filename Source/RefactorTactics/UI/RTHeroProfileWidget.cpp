#include "UI/RTHeroProfileWidget.h"

#include "UI/RTHeroRadarWidget.h"

void URTHeroProfileWidget::SetProfileView(const FRTHeroProfileView& InProfileView)
{
	ProfileView = InProfileView;
	OnProfileViewChanged();
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
