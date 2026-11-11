#pragma once

#include "CoreMinimal.h"
#include "RTHeroProfileView.generated.h"

/**
 * I contratti di PRESENTAZIONE del Profilo Tattico (player-facing) / Hero Profile (tecnico).
 *
 * ⚠️ **Qui non si calcola niente.** Questo header dichiara cosa la scheda personaggio puo' MOSTRARE; chi
 * produce i numeri sta altrove e resta l'unico a possederli. La regola non e' stilistica: i sei assi del
 * Profile Radar hanno gia' un derivatore in `tools/radar/profile.ts` (D-107, `#562`), e una seconda
 * implementazione in C++ non sarebbe una comodita' — sarebbe una fonte concorrente che puo' divergere
 * senza che nessun gate se ne accorga. ∴ la view arriva **gia' risolta**, e la UI la presenta.
 *
 * 🔴 **Nessun puntatore gameplay entra in queste struct, ed e' una proprieta' della FIRMA, non una
 * disciplina.** Se un Blueprint potesse raggiungere `ARTUnit`, `ARTTurnManager` o un catalogo da qui,
 * avrebbe il modo di ricalcolare e di leggere stato non pubblico — e' lo stesso confine che
 * `RTScreenHudWidgets.h` difende per lo Screen HUD (§4.1 di `progettazione-hud.md`, `#613`).
 * `RefactorTactics.HeroProfile.NoGameplayPointersInView` lo pinna per riflessione.
 *
 * 🔵 **Identita' come `FName`, testo come `FText`.** L'ID e' cio' che resta stabile mentre le etichette
 * cambiano lingua o wording; il testo e' gia' risolto/localizzabile a monte. Un widget che confrontasse
 * etichette invece di ID si romperebbe alla prima traduzione.
 */

/**
 * Un asse del radar tattico: identita' stabile, etichetta gia' risolta, valore e fondoscala.
 *
 * ⚠️ **`MaxValue` viaggia con l'asse invece di essere una costante del widget.** Il Profile Radar corrente
 * pubblica su scala `1..10`, ma cablare `10` nel renderer significherebbe che un dataset con un fondoscala
 * diverso verrebbe disegnato sbagliato **in silenzio**, invece di essere rifiutato dalla validazione.
 */
USTRUCT(BlueprintType)
struct FRTProfileRadarAxisView
{
	GENERATED_BODY()

	/** ID stabile dell'asse (es. `Radar.Profile.Offense`). E' cio' su cui si cercano i duplicati. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName AxisId;

	/** Etichetta player-facing gia' risolta (es. «Offesa»). Non e' la chiave: non ci si confronta sopra. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText Label;

	/** Valore pubblicato dal produttore autorevole. Ammesso `0`, che significa «al centro», non «assente». */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	int32 Value = 0;

	/** Fondoscala dell'asse. `<= 0` non e' un default neutro: e' un dato rotto, e la validazione lo dice. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	int32 MaxValue = 10;
};

/**
 * Una relazione elementale dichiarata (affinita', sintonia, contrasto...) come coppia di ID semantici.
 *
 * 🔴 **`RelationId` e' un `FName`, non un enum**, perche' la matrice completa delle relazioni elementali
 * non ha ancora un owner runtime. Un `enum class ERTElementRelation` scritto oggi canonizzerebbe in C++ un
 * vocabolario che il design non ha chiuso, e ogni voce futura costerebbe una modifica al motore invece di
 * una riga di dato.
 */
USTRUCT(BlueprintType)
struct FRTElementRelationView
{
	GENERATED_BODY()

	/** L'elemento in relazione (es. `Element.Water`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName ElementId;

	/** Il TIPO di relazione, come ID semantico (es. `Element.Relation.Synergy`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName RelationId;

	/** Etichetta gia' risolta della relazione. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText Label;
};

/**
 * Il grado di padronanza di un elemento e le capability che lo giustificano (`#995`).
 *
 * 🔴 **Non e' l'Affinity, e la distinzione e' il motivo per cui questa struct esiste separata.**
 * `URTHeroData::Affinity` e' un'affinita' DICHIARATA dal catalogo — che puo' anche non essere elementale
 * (strutture, movimento) — mentre la proficiency e' DERIVATA da cosa il kit sa davvero fare con
 * quell'elemento. Un eroe con affinita' elettrica e nessuna capability elettrica ha proficiency vuota, e
 * mostrarne una dedotta dall'affinita' sarebbe inventare un dato.
 * `RefactorTactics.HeroProfile.AffinityDoesNotImplyProficiency` lo pinna.
 */
USTRUCT(BlueprintType)
struct FRTElementProficiencyView
{
	GENERATED_BODY()

	/** L'elemento a cui il grado si riferisce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName ElementId;

	/** Grado come ID stabile (es. `Element.Grade.Adept`). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName GradeId;

	/** Etichetta gia' risolta del grado. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText GradeLabel;

	/**
	 * Le capability che il kit esprime su quell'elemento — la grammatica di `#995`:
	 * `Generate` · `Apply` · `Propagate` · `Transform` · `Consume`.
	 *
	 * ⚠️ Restano `FName` e non un enum per la stessa ragione di `RelationId`: la grammatica e' di `#995`,
	 * non di questo widget, e la UI non deve diventare il posto in cui si canonizza.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	TArray<FName> Capabilities;
};

/**
 * La vista completa del Profilo Tattico: tutto cio' che la scheda mostra, e nient'altro.
 *
 * ⚠️ **Autosufficiente per costruzione.** Il componente deve poter vivere in Character Select, in una
 * preview di Wiki, in un inspector o nell'HUD senza dipendere da `ARTTurnManager`: se per disegnarsi
 * dovesse chiedere qualcosa al match, sarebbe riusabile solo dentro una partita.
 *
 * 🔵 **I campi opzionali sono opzionali davvero.** Un ruolo assente, una relazione mancante o una
 * proficiency vuota sono lo stato normale finche' un provider autorevole non li fornisce — e la regola
 * fail-closed della UI e' nasconderli o mostrare «—», mai dedurli.
 */
USTRUCT(BlueprintType)
struct FRTHeroProfileView
{
	GENERATED_BODY()

	/** ID stabile dell'eroe, copiato dal catalogo. ⚠️ Nessun `HeroId` specifico e' cablato nella UI. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName HeroId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText DisplayName;

	/**
	 * Ruolo primario come ID + etichetta. ⚠️ **Non e' un enum, e non lo diventa in questa issue.**
	 * Il tooling ha oggi quattro ruoli hard-coded (`Controller`/`Support`/`Guardian`/`Striker` in
	 * `tools/radar/generate.ts`) e il design ne propone sei diversi per la v0.2: migrarne uno dei due in
	 * C++ adesso sceglierebbe la tassonomia canonica per omissione, senza un owner che l'abbia decisa.
	 * ∴ la UI tollera un profilo **senza** ruolo finche' il dato non esiste.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName PrimaryRoleId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText PrimaryRoleLabel;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName SecondaryRoleId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText SecondaryRoleLabel;

	/** Tag di stile gia' risolti (es. «Tattico», «Zoner»). Testo, perche' nessuno ci si confronta sopra. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	TArray<FText> StyleTags;

	/**
	 * Gli assi del radar **nell'ordine in cui vanno disegnati**.
	 *
	 * 🔴 **L'ordine e' dato, non ricostruito.** Il Profile Radar dichiara un ordine normativo
	 * (Offesa → Durabilita' → Mobilita' → Controllo → Supporto → Informazione); riordinare per etichetta o
	 * per `FName` produrrebbe una sagoma diversa per lo stesso eroe, cioe' una seconda verita' visiva.
	 * `RefactorTactics.HeroProfile.RadarPreservesAxisOrder` lo pinna.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	TArray<FRTProfileRadarAxisView> RadarAxes;

	/**
	 * Affinita' dichiarata dal catalogo. ⚠️ **Non e' detto che sia un elemento**: il dato corrente ammette
	 * affinita' a strutture o movimento, quindi la sezione non puo' assumere un'iconografia elementale.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName AffinityId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText AffinityLabel;

	/**
	 * Debolezza dichiarata. 🔴 **E' un'identita', non un moltiplicatore.** Trasformarla in resistenza o in
	 * danno e' una decisione di gameplay con un owner che non e' questo widget.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FName WeaknessId;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText WeaknessLabel;

	/** Relazioni elementali ulteriori, se e solo se un provider autorevole le fornisce. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	TArray<FRTElementRelationView> ElementRelations;

	/** Proficiency elementali (`#995`). Sezione separata e opzionale: vuota e' uno stato valido. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	TArray<FRTElementProficiencyView> ElementalProficiencies;

	/** Portata come testo gia' risolto (fascia leggibile), non come numero da reinterpretare. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText RangeLabel;

	/** Difficolta' di apprendimento. `0` = non dichiarata; la UI la nasconde invece di mostrare uno zero. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	int32 LearningDifficulty = 0;

	/** Difficolta' di maestria. Stessa convenzione di `LearningDifficulty`. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	int32 MasteryDifficulty = 0;

	/** Una frase che dice cosa l'eroe vuole fare in campo. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	FText CombatIdentity;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	TArray<FText> Strengths;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "RefactorTactics|HeroProfile")
	TArray<FText> Tradeoffs;
};
