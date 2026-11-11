#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"

#if WITH_DEV_AUTOMATION_TESTS

namespace RTGateCoherence
{
	/** I due documenti, letti dal SORGENTE: nessuno stato di gate vive in questo file. */
	FString DoDPath()  { return FPaths::Combine(FPaths::ProjectDir(), TEXT("docs/roadmap/v0.1-definition-of-done.md")); }
	FString PiaPath()  { return FPaths::Combine(FPaths::ProjectDir(), TEXT("docs/roadmap/roadmap-pia.md")); }

	/**
	 * Il campo `Indice` (1-based) di una riga di tabella markdown, contando **solo le pipe non escapate**.
	 *
	 * 🔴 **E lo `\|` non e' un caso di scuola: la cella di `G9` lo usa.** Quella riga documenta il comando che
	 * legge gli stati, e il comando contiene un'alternanza `✅ \| \U0001F7E1 \| ⏳` — pipe escapate **e** glifi di stato
	 * dentro una colonna che non e' quella dello stato. Uno split ingenuo su `|` spezzerebbe la cella e
	 * leggerebbe il glifo sbagliato **proprio sul gate piu' delicato**.
	 */
	FString CampoTabella(const FString& Riga, int32 Indice)
	{
		int32 Campo = 0;
		int32 Inizio = INDEX_NONE;
		for (int32 i = 0; i < Riga.Len(); ++i)
		{
			if (Riga[i] != TEXT('|')) { continue; }
			if (i > 0 && Riga[i - 1] == TEXT('\\')) { continue; } // pipe escapata: fa parte del testo
			++Campo;
			if (Campo == Indice) { Inizio = i + 1; }
			else if (Campo == Indice + 1 && Inizio != INDEX_NONE)
			{
				return Riga.Mid(Inizio, i - Inizio).TrimStartAndEnd();
			}
		}
		return Inizio != INDEX_NONE ? Riga.Mid(Inizio).TrimStartAndEnd() : FString();
	}

	/**
	 * **Un gate è soddisfatto se e solo se la sua cella di stato APRE con `✅`.**
	 *
	 * 🔴 **E NON si usa la convenzione dichiarata da `G9`, che qui darebbe la risposta sbagliata.** Quella
	 * cella prescrive di leggere *«la prima icona fra `✅ 🟡 ❌ ⏳`, ignorando `⚠️` e `🔴` che aprono molte celle
	 * come marcatori di prosa»* — ed è giusta **per le voci PIE**, che quella riga conta.
	 *
	 * ⛔ **Su `G12` cade.** Misurato il 2026-09-09: la sequenza dei glifi nella sua cella è
	 * `🔴 ✅ ⏳ ⌫ ✅`. Ignorando `🔴` come la convenzione prescrive, la «prima icona di stato» diventa quel
	 * `✅`, che però appartiene a *«✅ La causa è rimossa e i tre target ricompilano»* — **prosa, non esito**.
	 * L'esito vero è il `🔴 STANTIO` in testa. Con quella convenzione questo oracolo leggerebbe `G12` come
	 * soddisfatto, cioè sarebbe **cieco esattamente al difetto che esiste per prendere**.
	 *
	 * 🔑 La regola qui è più stretta e regge su tutte le celle: **l'esito è il glifo che APRE la cella**.
	 * Un `✅` che compare più avanti è prosa, qualunque cosa lo preceda.
	 */
	bool ApreConSoddisfatto(const FString& Cella)
	{
		return Cella.TrimStart().StartsWith(TEXT("✅"), ESearchCase::CaseSensitive);
	}
}

/**
 * **I due documenti dei gate devono dire la stessa cosa, e nessuno lo verificava.**
 *
 * `roadmap-pia.md` §4 raccoglie i *«gate gia' soddisfatti — da registrare, non da lavorare»*. Lo **stato** di
 * quei gate non vive li': vive in `v0.1-definition-of-done.md` §3, che ne e' la sola autorita'. §4 e' un
 * **consumatore**, e puo' citare ma mai contraddire.
 *
 * 🔴 **Il difetto e' gia' capitato, ed e' la ragione per cui questo test esiste** (`#2771`, trovato da
 * `#2615`). Il 2026-09-09 §3 dichiarava `G12` **`STANTIO`** mentre §4 lo elencava ancora fra i soddisfatti:
 * chi avesse letto solo la mappa avrebbe concluso che il packaging era gia' fatto, mentre la fonte lo dava da
 * rifare. E' sopravvissuto **tre giorni** in un documento revisionato due volte, perche' nessun controllo
 * confrontava i due file.
 *
 * ⛔ **Cosa questo test NON fa**: non verifica che gli stati di §3 siano **veri** — quello richiede rieseguire
 * i gate, e senza CI ([D-182]) non e' automatizzabile. Verifica che i due documenti **dicano la stessa cosa**.
 *
 * ⚠️ **E il limite va detto qui, non scoperto da chi legge**: senza CI questo oracolo gira solo quando
 * qualcuno lancia la suite. Non impedisce a uno stato di scadere — lo rende **visibile alla prima corsa**.
 * E' comunque tre giorni prima di quanto e' successo.
 */
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTGateCoherenceTest,
	"RefactorTactics.Meta.SatisfiedGatesAgreeWithTheDefinitionOfDone",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTGateCoherenceTest::RunTest(const FString&)
{
	using namespace RTGateCoherence;

	FString DoD, Pia;
	if (!TestTrue(TEXT("v0.1-definition-of-done.md e' leggibile"), FFileHelper::LoadFileToString(DoD, *DoDPath()))
		|| !TestTrue(TEXT("roadmap-pia.md e' leggibile"), FFileHelper::LoadFileToString(Pia, *PiaPath())))
	{
		return false;
	}

	// --- 1. Lo stato canonico di ogni gate, da §3 ------------------------------------------------------
	TMap<FString, FString> StatoDiGate;
	{
		TArray<FString> Righe;
		DoD.ParseIntoArrayLines(Righe, /*CullEmpty=*/ false);
		for (const FString& Riga : Righe)
		{
			if (!Riga.StartsWith(TEXT("| **G"))) { continue; }
			const FString Nome = CampoTabella(Riga, 1).Replace(TEXT("*"), TEXT("")).TrimStartAndEnd();
			// La colonna STATO e' la quarta: `| gate | criterio | evidenza | stato |`.
			const FString Cella = CampoTabella(Riga, 4);
			if (!Nome.IsEmpty()) { StatoDiGate.Add(Nome, Cella); }
		}
	}
	if (!TestTrue(TEXT("premessa: §3 ha prodotto gli stati dei gate"), StatoDiGate.Num() > 0)) { return false; }

	// --- 2. I gate che §4 dichiara SODDISFATTI ---------------------------------------------------------
	//
	// ⚠️ **Solo la TABELLA, non l'intera §4.** La sezione contiene anche la registrazione delle rimisure, che
	// cita di proposito stati **non** `✅` — leggerla come se fossero rivendicazioni di soddisfazione
	// renderebbe questo test rosso su ciò che è corretto.
	TArray<FString> Rivendicati;
	{
		TArray<FString> Righe;
		Pia.ParseIntoArrayLines(Righe, /*CullEmpty=*/ false);
		bool bDentroTabella = false;
		for (const FString& Riga : Righe)
		{
			if (Riga.StartsWith(TEXT("| Gate | Evidenza |"))) { bDentroTabella = true; continue; }
			if (!bDentroTabella) { continue; }
			if (!Riga.StartsWith(TEXT("|"))) { break; }            // la tabella finisce alla prima riga non-tabella
			if (Riga.StartsWith(TEXT("|---")))  { continue; }      // separatore

			// Ogni `G<n>` nominato nella riga è una rivendicazione di soddisfazione.
			for (int32 i = 0; i + 1 < Riga.Len(); ++i)
			{
				if (Riga[i] != TEXT('G') || !FChar::IsDigit(Riga[i + 1])) { continue; }
				FString Numero;
				int32 j = i + 1;
				while (j < Riga.Len() && FChar::IsDigit(Riga[j])) { Numero.AppendChar(Riga[j]); ++j; }
				Rivendicati.AddUnique(FString(TEXT("G")) + Numero);
				i = j - 1;
			}
		}
	}

	AddInfo(FString::Printf(TEXT("§4 rivendica come soddisfatti: %s"),
		Rivendicati.Num() > 0 ? *FString::Join(Rivendicati, TEXT(", ")) : TEXT("(nessuno)")));

	// --- 3. Il confronto ------------------------------------------------------------------------------
	for (const FString& Gate : Rivendicati)
	{
		const FString* Stato = StatoDiGate.Find(Gate);
		if (!Stato)
		{
			// Un gate citato che §3 non conosce e' un errore, non un caso da ignorare: o il nome e' sbagliato,
			// o la matrice l'ha perso.
			AddError(FString::Printf(
				TEXT("`roadmap-pia.md` §4 dichiara soddisfatto **%s**, che `v0.1-definition-of-done.md` §3 non ")
				TEXT("contiene affatto"), *Gate));
			continue;
		}
		if (!ApreConSoddisfatto(*Stato))
		{
			AddError(FString::Printf(
				TEXT("`roadmap-pia.md` §4 dichiara soddisfatto **%s**, ma in §3 la sua cella apre con «%s» — i due ")
				TEXT("documenti si contraddicono, e §3 e' la fonte. E' il difetto di #2771."),
				*Gate, *Stato->TrimStart().Left(40)));
		}
	}

	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
