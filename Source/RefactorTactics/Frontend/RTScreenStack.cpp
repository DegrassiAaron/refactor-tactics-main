#include "Frontend/RTScreenStack.h"

ERTNavResult FRTScreenStack::PushScreen(FName ScreenId)
{
	if (ScreenId.IsNone())
	{
		return ERTNavResult::InvalidScreen;
	}

	// Un modale disabilita la schermata sotto (DoD di CP 46.1). Una schermata che non riceve input non
	// puo' nemmeno navigare: il rifiuto e' la conseguenza di quella regola, non una regola in piu'.
	if (IsModalOpen())
	{
		return ERTNavResult::BlockedByModal;
	}

	Screens.Add(ScreenId);
	return ERTNavResult::Ok;
}

ERTNavResult FRTScreenStack::PopScreen()
{
	if (IsModalOpen())
	{
		return ERTNavResult::BlockedByModal;
	}

	// La radice non ha Back. Non e' un errore del chiamante — e' lo stato in cui il pulsante non esiste,
	// e `CanGoBack()` lo dice **prima** che qualcuno provi.
	if (Screens.Num() <= 1)
	{
		return ERTNavResult::BlockedAtRoot;
	}

	Screens.Pop();
	return ERTNavResult::Ok;
}

ERTNavResult FRTScreenStack::ShowModal(FName ModalId)
{
	if (ModalId.IsNone())
	{
		return ERTNavResult::InvalidScreen;
	}

	// I modali si impilano: un errore aperto sopra una conferma non cancella la conferma, e chiudendolo
	// si torna a lei. Cio' che non si impila e' la NAVIGAZIONE sotto, che resta bloccata finche' l'ultimo
	// modale non e' chiuso.
	Modals.Add(ModalId);
	return ERTNavResult::Ok;
}

ERTNavResult FRTScreenStack::CloseModal()
{
	if (Modals.Num() == 0)
	{
		// Distinto da `Ok` di proposito: se questo caso ricadesse sul percorso normale, un doppio click
		// sul pulsante di chiusura mangerebbe una schermata invece di non fare niente.
		return ERTNavResult::NoModalOpen;
	}

	Modals.Pop();
	return ERTNavResult::Ok;
}

ERTNavResult FRTScreenStack::ReturnMain()
{
	// ⚠️ **Non passa da `PopScreen`/`CloseModal` di proposito.** Quelli hanno guardie — radice, modale —
	// e l'uscita di sicurezza non puo' essere soggetta alle condizioni da cui deve far uscire: un
	// `ReturnMain` bloccato da un modale sarebbe esattamente il dead-end che il DoD vieta.
	Modals.Reset();

	if (Screens.Num() > 1)
	{
		Screens.SetNum(1, EAllowShrinking::No);
	}

	return ERTNavResult::Ok;
}
