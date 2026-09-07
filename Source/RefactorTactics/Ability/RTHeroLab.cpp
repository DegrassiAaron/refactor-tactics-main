#include "Ability/RTHeroLab.h"

#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTHeroData.h"

TArray<FRTHeroLabEntry> URTHeroLabLibrary::ListCanonicalHeroes()
{
	TArray<FRTHeroLabEntry> Out;

	for (const URTHeroData* Hero : URTHeroCatalogLibrary::GetHeroRoster())
	{
		if (!Hero) { continue; }

		FRTHeroLabEntry Entry;
		Entry.HeroId = Hero->HeroId;
		Entry.DisplayName = Hero->DisplayName;
		Entry.MaxHealth = Hero->MaxHealth;
		Entry.MovePoints = Hero->MovePoints;
		Entry.VisionRange = Hero->VisionRange;
		Entry.HearingThreshold = Hero->HearingThreshold;
		Entry.PushResistance = Hero->PushResistance;
		Entry.Affinity = Hero->Affinity;
		Entry.Weakness = Hero->Weakness;
		Entry.ReactionProfileId = Hero->ReactionProfileId;
		// Il CONTEGGIO viene dal dato, non da una costante: il catalogo dichiara quante voci ha il kit, e
		// scrivere «quattro» qui renderebbe il pannello muto il giorno in cui ne aggiunge una quinta.
		Entry.DeclaredAbilityCount = Hero->Actions.Num();
		Out.Add(Entry);
	}

	return Out;
}

bool URTHeroLabLibrary::FindHero(const FName& HeroId, FRTHeroLabEntry& OutEntry)
{
	for (const FRTHeroLabEntry& Entry : ListCanonicalHeroes())
	{
		if (Entry.HeroId == HeroId)
		{
			OutEntry = Entry;
			return true;
		}
	}
	return false;
}

TArray<FRTAbilityLabEntry> URTHeroLabLibrary::ListHeroKit(const FName& HeroId)
{
	TArray<FRTAbilityLabEntry> Kit;

	// Si FILTRA l'elenco canonico di #2599 invece di rileggere `URTHeroData::Actions` e rimappare: una
	// seconda mappatura degli stessi campi divergerebbe al primo campo aggiunto a una sola delle due.
	for (const FRTAbilityLabEntry& Entry : URTAbilityLabLibrary::ListCanonicalAbilities())
	{
		if (!Entry.bIsCoreAction && Entry.OwnerHeroId == HeroId)
		{
			Kit.Add(Entry);
		}
	}

	return Kit;
}

bool URTHeroLabLibrary::BuildHeroFixture(const FName& HeroId, const FName& AbilityId,
	const FRTAbilityLabFixtureSpec& Spec, FRTTestScenario& OutScenario, FString& OutError)
{
	FRTHeroLabEntry Hero;
	if (!FindHero(HeroId, Hero))
	{
		OutError = FString::Printf(
			TEXT("HeroId non canonico: '%s'. Il roster della v0.1 e' Hero.Gadget, Hero.Phase, Hero.Branth, Hero.Wraith."),
			*HeroId.ToString());
		return false;
	}

	// L'unica domanda che l'Ability Lab non puo' porre. Per #2599 ogni ability canonica e' eseguibile; qui
	// conta anche DI CHI e'. Senza questo controllo Hero Lab farebbe lanciare a Gadget un'abilita' di Wraith
	// e la fixture funzionerebbe — mostrando come «kit dell'eroe» qualcosa che non lo e'.
	bool bOwnsIt = false;
	for (const FRTAbilityLabEntry& Owned : ListHeroKit(HeroId))
	{
		if (Owned.AbilityId == AbilityId) { bOwnsIt = true; break; }
	}

	if (!bOwnsIt)
	{
		OutError = FString::Printf(TEXT("'%s' non e' nel kit di '%s'."),
			*AbilityId.ToString(), *HeroId.ToString());
		return false;
	}

	// DELEGA, e nient'altro. Se un giorno questa funzione costruisse anche un solo campo dello scenario per
	// conto proprio, il secondo Ability Runner che il non-goal di #2600 vieta sarebbe nato esattamente qui.
	return URTAbilityLabLibrary::BuildFixture(AbilityId, Spec, OutScenario, OutError);
}
