#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Turn/RTResolvedEvent.h"
#include "RTPresentationBinding.generated.h"

/**
 * Cosa una voce DICHIARA per il proprio tipo di evento.
 *
 * ⚠️ `Cues` e' il default apposta: una voce costruita di default e' «dovrebbe avere cue» e **non ne ha**,
 * quindi risulta MANCANTE. Se il default fosse `NoPresentation`, dimenticare una voce la coprirebbe da sola
 * — che e' esattamente il fallimento silenzioso che `#1801` esiste per impedire.
 */
UENUM(BlueprintType)
enum class ERTPresentationKind : uint8
{
	/** L'evento si mostra: la voce elenca le cue richieste. */
	Cues,
	/** L'evento NON si mostra, e qualcuno lo ha deciso. Richiede un motivo scritto (`Rationale`). */
	NoPresentation
};

/**
 * Il legame dichiarativo fra un `ERTResolvedEventType` e la sua presentazione (`D-278`).
 *
 * 🔑 **Dichiarativo si oppone a BRANCHING, non a "compilato".** Questa tabella e' C++ e non un Data Asset
 * perche' in v0.1 le cue le tocca chi scrive codice: `D-124` tiene *«Niagara dedicato a ogni abilita'»* fuori
 * dal perimetro e `Content/` non contiene un solo asset Niagara. Il giorno in cui un artista dovra'
 * riassegnarle senza ricompilare, cambia il **supporto** — non il gate, che interroga una funzione pura.
 *
 * ⛔ **Blueprint ESEGUE la presentazione; non possiede il branching delle regole del simulatore.** I nomi qui
 * dentro sono cio' che il C++ gia' chiama, non un vocabolario nuovo da mantenere in parallelo.
 */
USTRUCT(BlueprintType)
struct FRTPresentationBinding
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Presentation")
	ERTResolvedEventType Type = ERTResolvedEventType::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Presentation")
	ERTPresentationKind Kind = ERTPresentationKind::Cues;

	/**
	 * Le cue richieste, con il nome che il codice usa davvero (`PlayAttackMontage`, `HideForDefeat`, …).
	 * Una voce `Cues` con questo elenco vuoto — o con soli `NAME_None` — **non copre**: e' il caso del widget
	 * dichiarato e mai disegnato, e vale come mancanza.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Presentation")
	TArray<FName> Cues;

	/**
	 * Perche' questo evento non si mostra. **Obbligatorio** quando `Kind == NoPresentation`: senza, la
	 * dichiarazione positiva sarebbe indistinguibile da una dimenticanza che ha imparato a tacere.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|Presentation")
	FString Rationale;

	FRTPresentationBinding() = default;

	/** Voce con cue. */
	FRTPresentationBinding(ERTResolvedEventType InType, const TArray<FName>& InCues)
		: Type(InType), Kind(ERTPresentationKind::Cues), Cues(InCues) {}

	/** Voce `NoPresentation`: il motivo non e' decorativo, e' cio' che la distingue da un'assenza. */
	static FRTPresentationBinding MakeNoPresentation(ERTResolvedEventType InType, const FString& InRationale)
	{
		FRTPresentationBinding B;
		B.Type = InType;
		B.Kind = ERTPresentationKind::NoPresentation;
		B.Rationale = InRationale;
		return B;
	}
};

/**
 * Il contratto evento risolto -> presentazione, e il suo gate di esaustivita' (`D-278`, `#1801`).
 *
 * 🔴 **Il difetto che questa libreria esiste per rendere IMPOSSIBILE.** `ERTResolvedEventType` puo' crescere
 * senza che nulla se ne accorga: un valore nuovo senza presentazione compila, risolve correttamente la logica,
 * e **sparisce a schermo**. E' il fallimento piu' silenzioso possibile, perche' il gioco resta giusto e solo
 * la presentazione mente per omissione. Non e' un'ipotesi: `HazardDamage` e' entrato nell'enum ed e' rimasto
 * muto, e nessun test e' diventato rosso.
 *
 * 🔑 **Il gate itera l'enum VERO** (`StaticEnum<ERTResolvedEventType>()`), non una lista scritta a mano: una
 * lista sarebbe essa stessa una copertura da mantenere, e sposterebbe il difetto invece di chiuderlo. Cosi'
 * un valore aggiunto domani e' coperto **per costruzione**, senza che nessuno tocchi il gate.
 *
 * ⚠️ **La funzione pura e' l'owner del giudizio, non il test.** `FindMissingBindings` e' chiamabile da un
 * Automation Test, da un commandlet o da un validator Editor: quale di questi esista non cambia la regola.
 * E' la stessa disciplina di `URTIconLibrary::FindMissingRequiredIcons`, con la stessa ragione — un contratto
 * rotto si scopre in CI e non a schermo.
 */
UCLASS()
class REFACTORTACTICS_API URTPresentationBindingLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * La tabella dichiarata: una voce per ogni `ERTResolvedEventType`.
	 *
	 * ⚠️ Chi aggiunge un valore all'enum aggiunge una riga QUI, oppure il gate diventa rosso. E' l'unico
	 * scopo di questa lista: non poter dimenticare.
	 */
	static TArray<FRTPresentationBinding> DeclaredBindings();

	/**
	 * Le voci che mancano, come righe leggibili `"<Tipo>: <motivo>"`. **Vuoto significa copertura completa.**
	 *
	 * Un tipo risulta mancante quando:
	 *  - nessuna voce lo dichiara;
	 *  - piu' voci lo dichiarano (quale valga sarebbe ambiguo, e l'ambiguita' non e' una dichiarazione);
	 *  - la voce dice `Cues` ma non ne elenca nessuna valida;
	 *  - la voce dice `NoPresentation` senza scrivere perche'.
	 *
	 * 🔴 **Un elenco VUOTO in ingresso non e' «zero mancanze»: e' la mancanza totale.** E' la stessa scelta di
	 * `FindMissingRequiredIcons`, e serve perche' un gate che tace quando non trova nulla e' un gate che passa
	 * proprio nel caso peggiore.
	 *
	 * Deterministica: l'ordine dell'uscita segue i valori dell'enum, mai l'ordine di un `TMap` o di un `TSet`.
	 */
	static TArray<FString> FindMissingBindings(const TArray<FRTPresentationBinding>& Bindings);

	/**
	 * Quanti valori dichiara `ERTResolvedEventType`.
	 *
	 * ⚠️ E' `NumEnums() - 1`: l'ultima voce e' il `_MAX` sintetico che UHT aggiunge, e non e' un valore
	 * scrivibile — stessa convenzione di `RTScenarioLoader.cpp:66`. Restituisce `0` se la reflection non e'
	 * disponibile, e chi la usa per un ciclo non deve poterlo confondere con «nessun tipo da coprire».
	 */
	static int32 DeclaredEventTypeCount();

	/** Il nome leggibile di un tipo (`Attack`), per i messaggi del gate. */
	static FString EventTypeName(ERTResolvedEventType Type);
};
