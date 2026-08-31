#include "RTHexAnchorReadout.h"

namespace RTHexAnchor
{
	FReadout Describe(const FRTAnchorRef& From, const FRTAnchorRef& To, ERTAnchorPairRefusal Refusal)
	{
		FReadout Out;
		Out.Anchor = FString::Printf(TEXT("%s -> %s"), *From.ToString(), *To.ToString());
		Out.bValid = Refusal == ERTAnchorPairRefusal::None;

		switch (Refusal)
		{
		case ERTAnchorPairRefusal::None:
			// Niente da spiegare: il muro si fa. La riga resta vuota invece di dire «ok», che sarebbe rumore
			// su ogni gesto riuscito — cioe' su quasi tutti.
			break;

		case ERTAnchorPairRefusal::DifferentCell:
			Out.Reason = TEXT("i due estremi stanno in celle diverse: un muro lungo e' piu' segmenti, uno per cella");
			break;

		case ERTAnchorPairRefusal::DifferentLayer:
			Out.Reason = TEXT("i due estremi stanno su piani diversi: la geometria di un piano non descrive l'altro");
			break;

		case ERTAnchorPairRefusal::SameAnchor:
			Out.Reason = TEXT("il gesto torna sullo stesso punto: un muro ha due estremi distinti");
			break;

		case ERTAnchorPairRefusal::NoTacticalAxis:
			// ⚠️ La frase dice ANCHE cosa fare, e non e' cortesia: senza, l'autore sa di aver sbagliato e non
			// sa come non sbagliare. Le ventiquattro sono tutte e sole vertice <-> punto medio non adiacente,
			// quindi il rimedio e' sempre lo stesso e si puo' scrivere una volta per tutte.
			Out.Reason = TEXT("questa coppia non sta su nessun asse tattico: un vertice si collega ai punti ")
				TEXT("medi dei propri due lati, non agli altri quattro");
			break;

		default:
			// Un valore d'enum che questo file non conosce arriva da un runtime piu' recente. Dire «non si
			// puo', e non so perche'» e' meglio di una riga vuota, che somiglierebbe a un successo.
			Out.Reason = TEXT("questo gesto non produce un muro");
			break;
		}

		return Out;
	}

	FReadout DescribePending(const FRTAnchorRef& From)
	{
		FReadout Out;
		Out.Anchor = From.ToString();
		Out.bValid = false; // non c'e' ancora un muro: il ghost non deve promettere
		Out.Reason = TEXT("trascina fino a un secondo punto");
		return Out;
	}
}
