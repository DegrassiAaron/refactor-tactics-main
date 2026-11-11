#pragma once

#include "CoreMinimal.h"
#include "Player/Overlay/RTPlanningOverlay.h" // FRTPlanningOverlayInputs, FRTFocusLatch

class ARTHexMapActor;
class ARTUnit;
class FRTPlanningOverlayHost;

/**
 * **L'ARBITRO DELLE AREE DI PIANIFICAZIONE** — `#3511` (CX-2 di `aree-di-contesto-spec-panel-2026-10-06.md`), `#3750` —
 * uscito dal controller (#4065, S5).
 *
 * Raccoglie gli ingressi (`FRTPlanningOverlayInputs`), li confronta con quelli dell'ultimo calcolo e rifa' qualcosa
 * **solo se sono cambiati**: a ingressi invariati non ricalcola niente. Decide anche **chi** scrive la mappa — il piano,
 * il fuoco o la portata di una skill in hover, mai due insieme — e chiede a `RTPlanningPreview::Refresh` di scriverla.
 *
 * Parla col controller attraverso `FRTPlanningOverlayHost`, che sono tutte letture. Il controller tiene le facciate con le
 * firme di prima (`UpdatePlanningOverlay`, `RefreshPlanningOverlay`, i contatori, l'hover della skill).
 *
 * ⚠️ **Nessuna riflessione**, come `FRTPlaybackDirector`: lo stato e' di presentazione e muore col controller.
 */
class REFACTORTACTICS_API FRTPlanningOverlayArbiter
{
public:
	/**
	 * Il confronto di ogni fotogramma: lo chiama `PlayerTick` subito dopo aver posato l'hover, e un test senza `PlayerTick`.
	 *
	 * 🔑 **Due contatori, perche' sono due domande.** `GetComputations` conta i cambi di QUALUNQUE ingresso;
	 * `GetPlanComputations` solo quelli che rifanno ventaglio e portata, cioe' tutti tranne il cambio della sola cella sotto
	 * il cursore. Il secondo e' il criterio di `#711`: *«nessun produttore all'hover ricalcola il reachable set»*, e un test
	 * lo pinna.
	 *
	 * ⚠️ **Spegne anche, dallo stato**: se `ShowsAreas()` e' falso (commit, playback, `Modal`, KO, fine partita) azzera ogni
	 * area. L'uscita che qualcuno dimentica di elencare si corregge al tick successivo.
	 *
	 * @param KnownHexMap la mappa, se il chiamante l'ha gia' cercata (`PlayerTick`): evita una seconda ricerca di actor.
	 */
	void Update(const FRTPlanningOverlayHost& Host, ARTHexMapActor* KnownHexMap = nullptr);

	/**
	 * Il piano di `Unit` e' cambiato (o e' cambiata l'unita'): ricalcola SUBITO le aree. `Unit == nullptr` le spegne.
	 *
	 * 🔑 Sale `PlanRevision`, perche' un waypoint aggiunto non cambia nessun altro ingresso, e passa per lo stesso confronto e
	 * lo stesso reset di `Update`. Il tick successivo trova gli ingressi uguali e non rifa' niente.
	 *
	 * ⚠️ **`nullptr` spegne SUBITO, ma non decide lo stato**: se l'unita' selezionata e' viva e si sta pianificando, il tick
	 * successivo vede che le aree si mostrano e le riaccende. I due chiamanti con `nullptr` — commit e fine partita — sono in
	 * uno stato che dice il contrario (`ResolutionPlayback`, `bMatchEnded`), ed e' per questo che il reset non si ripete.
	 */
	void Refresh(const FRTPlanningOverlayHost& Host, const ARTUnit* Unit);

	/** Quanti calcoli hanno fatto gli ingressi cambiati dall'inizio (`#3511`). Per i test e la diagnostica. */
	int32 GetComputations() const { return PlanningOverlayComputations; }

	/** ...di questi, quanti hanno rifatto ventaglio e portata: i cambi della sola cella non ci sono. */
	int32 GetPlanComputations() const { return PlanningOverlayPlanComputations; }

	/**
	 * **IL SEGNALE DELL'HOVER SU UNA SKILL** (`#3750`, fetta D): lo slot della barra lo manda quando il cursore ci entra.
	 * Dice solo QUALE skill di QUALE unita'; non scrive la mappa, non calcola niente: lo fa l'arbitro, al tick, dagli
	 * ingressi (`FRTPlanningOverlayInputs::ActiveSource`).
	 *
	 * 🔑 **L'hover non e' un gesto di pianificazione**: non tocca selezione, piano, azione armata, ne' il fuoco. Finisce con
	 * `ClearSkillHover`, e allora torna la sorgente di prima.
	 *
	 * ⚠️ **L'unita' dev'essere quella comandata, o quella ispezionata e fissata mentre la barra la segue** (`#3748`): la barra
	 * propria e' di chi si comanda, la barra in sola lettura di chi e' a fuoco, e il produttore e' lo slot dell'una o
	 * dell'altra. Il filtro e' del risolutore degli ingressi, che la confronta con la selezione di adesso e con l'ispezione
	 * fissata (`bDockFollowsInspection` e pin), non di questa porta: l'hover di un'altra unita' non tiene la mappa, e il
	 * risolutore lo SCARTA al primo tick (non riapparirebbe alla riselezione di quella unita'). Anche l'hover dell'unita'
	 * ispezionata cade quando la barra torna alla selezione (`SelectUnit`), benche' la scheda resti fissata: la barra non e'
	 * piu' sua, e Slate non manda l'uscita a uno slot che il dock ricostruisce.
	 */
	void SetSkillHover(const ARTUnit* Unit, int32 AbilityIndex);

	/**
	 * Il cursore e' uscito dalla skill. **Spegne solo se l'hover registrato e' proprio quello** (stessa unita', stesso indice):
	 * l'uscita dallo slot A puo' arrivare dopo l'ingresso nello slot B, e non deve cancellare B.
	 */
	void ClearSkillHover(const ARTUnit* Unit, int32 AbilityIndex);

	/** L'hover registrato. `INDEX_NONE` se nessuno. */
	int32 GetSkillHoverIndex() const { return SkillHoverIndex; }
	const ARTUnit* GetSkillHoverUnit() const; // nel .cpp: il puntatore debole vuole il tipo completo

	/** `PinInspection` ha fissato un'ispezione: e' cio' che riporta l'area del fuoco, anche sulla stessa unita'. */
	void NoteInspectionPinned() { ++InspectionPinSerial; }

private:
	/** Gli ingressi di adesso per `Unit` (la selezionata, o quella che il chiamante dichiara). Non scrive niente. */
	FRTPlanningOverlayInputs Gather(const FRTPlanningOverlayHost& Host, const ARTUnit* Unit, const ARTHexMapActor* HexMap) const;

	/**
	 * **Il cancello del fuoco** (`#3750`, P2): riempie `FocusSubject`, `FocusMove` e `FocusKnowledgeStamp` se l'unita'
	 * ispezionata e' un NEMICO VIVO con una voce `Live` nella vista di chi guarda, e non tocca niente altrimenti.
	 *
	 * 🔴 **Il cancello e' qui e non nell'HUD**, anche se l'HUD rilascia gia' l'ispezione di chi non e' piu' `Live`
	 * (`ReleaseInspectionUnlessLive`): quella arriva al tick dell'HUD, e fra il tick in cui il nemico esce di vista e il suo
	 * l'area mostrerebbe a un osservatore cio' che non vede. La vista e' la stessa del velo
	 * (`URTHudViewModel::BuildObserverKnowledgeView`), e la cella del soggetto viene dalla SUA VOCE, non dall'attore.
	 * ⛔ Non guarda `ARTUnit::IsKnownToObserver()`, che nasce vero.
	 *
	 * Costa una ricerca di unita' e la costruzione della vista, e solo quando c'e' un'unita' ispezionata.
	 */
	void GatherFocus(const FRTPlanningOverlayHost& Host, FRTPlanningOverlayInputs& Inputs) const;

	/**
	 * **I cancelli dell'hover su una skill** (`#3750`, fetta D): riempie `HoverSkillUnit`, `SkillRange` e
	 * `SkillKnowledgeStamp` solo se la skill e' dell'unita' COMANDATA (`Unit`, e della squadra di chi guarda) o di quella a
	 * FUOCO (ispezionata e fissata, `#3748`), viva, se chiede un bersaglio e ha una portata positiva (`MostraLaPortata`), e se
	 * la cella da cui mira e' una cella che la squadra vede adesso. Non tocca niente altrimenti. Per la skill di un'unita'
	 * ispezionata (lo slot della barra in sola lettura) scrive anche `bSkillHoverOfInspected`, che e' cio' che apre l'arbitro
	 * per un'unita' che non e' la selezione.
	 *
	 * 🔴 **L'origine di un'unita' propria e' `AimOriginFor(fase)`, come la portata armata**, cosi' che l'insieme in hover
	 * coincida con quello dell'azione armata: e' un dato PROPRIO (il suo scatto pianificato). **Per un nemico a fuoco e' la
	 * cella della VOCE DELLA VISTA, mai il piano**: `AimOriginFor` leggerebbe lo scatto pianificato del nemico e la portata
	 * mostrerebbe da dove mirera'. Il cancello e' quello del fuoco: una voce `Live` nella vista di chi guarda.
	 */
	void GatherSkillHover(const FRTPlanningOverlayHost& Host, const ARTUnit* Unit, FRTPlanningOverlayInputs& Inputs) const;

	/**
	 * Gli ingressi, pronti per il confronto: raccolti, con il fuoco passato dal cancello e dal latch (`FocusLatch`), e
	 * **normalizzati** — i campi del fuoco valgono i default quando l'area non e' sulla mappa. Scrive il latch, quindi non e'
	 * `const`; non scrive niente sulla mappa.
	 */
	FRTPlanningOverlayInputs Resolve(const FRTPlanningOverlayHost& Host, const ARTUnit* Unit, const ARTHexMapActor* HexMap);

	/** Confronta, e se cambiano calcola o spegne. L'unico punto che chiama `RTPlanningPreview::Refresh`. */
	void Apply(const FRTPlanningOverlayHost& Host, ARTHexMapActor* HexMap, const ARTUnit* Unit,
		const FRTPlanningOverlayInputs& Inputs);

	/** Gli ingressi dell'ultimo calcolo, e se ce n'e' stato uno. */
	FRTPlanningOverlayInputs LastPlanningOverlayInputs;
	bool bHasPlanningOverlayInputs = false;

	/** Sale a ogni `Refresh`: il piano cambia senza che nessun altro ingresso lo dica. */
	int32 PlanningOverlayRevision = 0;

	int32 PlanningOverlayComputations = 0;
	int32 PlanningOverlayPlanComputations = 0;

	/** L'hover registrato dallo slot: quale unita' e quale posizione di kit. Vedi `SetSkillHover`. */
	TWeakObjectPtr<const ARTUnit> SkillHoverUnit;
	int32 SkillHoverIndex = INDEX_NONE;

	/** Quante volte `PinInspection` ha fissato dall'inizio. */
	int32 InspectionPinSerial = 0;

	/** L'area del fuoco e' ancora sulla mappa? (#3750) */
	FRTFocusLatch FocusLatch;
};
