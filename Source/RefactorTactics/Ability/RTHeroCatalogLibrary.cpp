#include "Ability/RTHeroCatalogLibrary.h"
#include "Ability/RTActionData.h"
#include "Ability/RTHeroData.h"

TArray<FString> URTHeroCatalogLibrary::ValidateHeroes(const TArray<const URTHeroData*>& Heroes)
{
	TArray<FString> Errors;
	TSet<FName> Seen;

	// Numero di azioni atteso: 1 attacco base + 4 abilita' fondamentali (catalogo v0.1 §"Struttura di un eroe").
	constexpr int32 ExpectedActionCount = 5;

	for (int32 i = 0; i < Heroes.Num(); ++i)
	{
		const URTHeroData* Hero = Heroes[i];
		if (Hero == nullptr)
		{
			Errors.Add(FString::Printf(TEXT("eroe #%d: riferimento nullo"), i));
			continue;
		}

		const FString Where = Hero->HeroId.IsNone()
			? FString::Printf(TEXT("eroe #%d"), i)
			: Hero->HeroId.ToString();

		if (Hero->HeroId.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: HeroId mancante (l'ID e' la chiave stabile dell'asset)"), *Where));
		}
		else if (Seen.Contains(Hero->HeroId))
		{
			Errors.Add(FString::Printf(TEXT("%s: HeroId duplicato"), *Where));
		}
		else
		{
			Seen.Add(Hero->HeroId);
		}

		if (Hero->MaxHealth <= 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: salute non positiva (%d)"), *Where, Hero->MaxHealth));
		}
		if (Hero->MovePoints <= 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: movimento non positivo (%d)"), *Where, Hero->MovePoints));
		}
		if (Hero->VisionRange < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: range visivo negativo (%d)"), *Where, Hero->VisionRange));
		}
		if (Hero->PushResistance < 0)
		{
			Errors.Add(FString::Printf(TEXT("%s: resistenza push negativa (%d)"), *Where, Hero->PushResistance));
		}
		if (Hero->Affinity.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: affinita' non dichiarata"), *Where));
		}
		// La debolezza non si inventa (catalogo v0.1 §5): un eroe senza debolezza dichiarata resta
		// strutturalmente incompleto, e il validator lo dice prima che arrivi in partita.
		if (Hero->Weakness.IsNone())
		{
			Errors.Add(FString::Printf(TEXT("%s: debolezza non dichiarata"), *Where));
		}

		if (Hero->Actions.Num() != ExpectedActionCount)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: attese %d azioni (attacco base + quattro fondamentali), trovate %d"),
				*Where, ExpectedActionCount, Hero->Actions.Num()));
			continue; // struttura gia' rotta: contare le varianti su un array della forma sbagliata non aiuta
		}

		// Indice 0 = attacco base, escluso dal conteggio delle varianti: la variante e' di un'abilita'
		// FONDAMENTALE (catalogo v0.1 §6), mai dell'attacco base (quello lo modificano le varianti d'arma,
		// equipaggiamento E7).
		int32 VariantCount = 0;
		for (int32 a = 1; a < Hero->Actions.Num(); ++a)
		{
			if (Hero->Actions[a] && Hero->Actions[a]->Variants.Num() > 0)
			{
				++VariantCount;
			}
		}
		if (VariantCount != 1)
		{
			Errors.Add(FString::Printf(
				TEXT("%s: %d abilita' fondamentali con variante (attesa esattamente 1)"), *Where, VariantCount));
		}
	}

	return Errors;
}
