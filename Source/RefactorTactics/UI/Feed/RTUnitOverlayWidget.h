#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "UI/HUD/RTHudViewModel.h" // FRTUnitOverlayView: la vista arriva gia' composta, il widget non la calcola
#include "UI/Feed/RTSkillCalloutView.h" // #4025: il fumetto attivo e' un membro per valore
#include "RTUnitOverlayWidget.generated.h"

class UBorder;
class UHorizontalBox;
class UProgressBar;
class UTextBlock;
class UWidget;
class URTIconCatalogData;

/**
 * La sovrapposizione sopra un'unita': nome, vita, scudo e stati (`#2288`, `D-320`).
 *
 * 🔴 **Il widget DISEGNA e non decide, e il disegno sta in C++.** Riceve una `FRTUnitOverlayView` gia'
 * composta da `URTHudViewModel::BuildUnitOverlay` — che unisce due produttori testati headless — e la posa
 * sugli elementi legati con `BindWidget`. Nessun `HasStatus`, nessun ordinamento, nessuna soglia.
 *
 * 🔑 **Perche' `BindWidget` e non un `BlueprintImplementableEvent` che lascia disegnare al grafo.** La
 * prima stesura di questa classe faceva cosi', ed era la stessa scelta che tutto `#2274` esiste per
 * evitare: ogni `if` scritto in un grafo Blueprint e' un `if` che **nessun test vede**. Con `BindWidget`
 * l'asset porta il **layout** — dove stanno gli elementi, che aspetto hanno — e il C++ porta **cosa ci
 * finisce dentro. Il confine e' quello di `AGENTS.md` §3: *«Data Asset/Blueprint configurano varianti e
 * presentazione»*, non le decidono.
 *
 * ⚠️ **Tutti i binding sono `Optional`, deliberatamente**: un `WBP` a cui manca un elemento **degrada** —
 * mostra il resto — invece di rifiutarsi di compilare. Una sovrapposizione senza barra dello scudo e' un
 * difetto visibile e riparabile; un widget che non compila fa sparire nome, vita e stati insieme.
 *
 * ⚠️ **Chi decide SE mostrarla non e' questo widget**: lo dice `ARTUnit::IsKnownToObserver()`, scritto dal
 * velo di conoscenza (`#2246`). Il componente che ospita questo widget si accende e si spegne con quello.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class REFACTORTACTICS_API URTUnitOverlayWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	URTUnitOverlayWidget(const FObjectInitializer& ObjectInitializer);

	/**
	 * L'ultima vista ricevuta. `BlueprintReadOnly` perche' il disegno la **legge**: un widget che potesse
	 * scriverla diventerebbe una seconda fonte di cio' che l'unita' e'.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|HUD")
	FRTUnitOverlayView View;

	/**
	 * Aggiorna la vista e ridisegna. Chiamata dal driver una volta per fotogramma.
	 *
	 * ⚠️ **Ridisegna SEMPRE, anche se la vista non e' cambiata**, e non e' una svista: confrontare due
	 * `FRTUnitOverlayView` richiederebbe un `operator==` su una struct che cresce, e il primo campo
	 * dimenticato sarebbe un'icona che non si aggiorna piu' — un difetto silenzioso, contro un costo che a
	 * quattro unita' non si misura.
	 */
	void SetOverlayView(const FRTUnitOverlayView& InView);

	/**
	 * Il testo che la barra della vita mostra, come funzione PURA dei due numeri.
	 *
	 * 🔑 Statica e testabile: e' l'unica regola di formato che questa classe possiede, e sta qui invece che
	 * dentro `NativeUpdate` perche' un test non debba costruire un widget per verificarla.
	 *
	 * ⚠️ `MaxHealth <= 0` non e' «morto», e' «nessuna unita'»: risponde vuoto invece di `0/0`.
	 */
	static FString ComposeHealthLabel(int32 Health, int32 MaxHealth);

	/**
	 * La frazione per una barra, sempre in `[0,1]`.
	 *
	 * ⚠️ Un massimo non positivo da' `0` e non una divisione per zero: e' il caso dell'unita' non
	 * configurata, non un errore da segnalare.
	 */
	static float SafeFraction(int32 Value, int32 Max);

	/**
	 * L'etichetta della durata da mostrare accanto a un'icona di stato.
	 *
	 * @return  i turni residui come stringa · **vuoto** se lo stato e' legato alla cella, dove un conteggio
	 *          non esiste — e vuoto anche per una durata non positiva, che non e' un tempo da mostrare.
	 *
	 * 🔑 Statica e pura perche' sia verificabile senza costruire un widget: e' l'unica regola di formato che
	 * la riga di uno stato possiede.
	 */
	static FString ComposeStatusDurationLabel(int32 RemainingTurns, bool bCellBound);

	/**
	 * Mostra il token di un colpo appena rivelato dal playback, e — se c'e' danno — enfatizza la barra
	 * (`#2455`).
	 *
	 * 🔴 **Il token vive QUI, dentro questo widget, ed e' un requisito di PRIVACY prima che di layout.**
	 * Il velo di conoscenza spegne l'intero componente che ospita questa sovrapposizione:
	 * `ARTHUD::UpdateObserverVeil` -> `ARTUnit::SetKnownToObserver` -> `RefreshComponentVisibility` ->
	 * `OverlayWidget->SetVisibility(bRender, false)`. Un token disegnato altrove — un widget world-space
	 * nuovo, un Actor di testo fluttuante — **non erediterebbe quel filtro**, e una cifra sopra un nemico
	 * mai osservato rivelerebbe insieme che esiste, dove sta e che ha incassato (`AGENTS.md` §4, [D-223]).
	 * ⚠️ E nessun test diventerebbe rosso: e' la stessa firma del difetto gia' misurato su `DrawHUD`.
	 *
	 * ⛔ **Non applica niente e non tocca `View`.** Il colpo e' gia' stato risolto e la vita che la barra
	 * mostra continua ad arrivare da `SetOverlayView`: questa chiamata aggiunge il **cambiamento**, sopra il
	 * canale che mostra lo **stato**.
	 *
	 * ⚠️ **La barra pulsa solo se la vita e' cambiata** — `bPulsesHealthBar`, che il token porta: un colpo con
	 * danno, una cura che ha restituito qualcosa ([D-505]). Con il caso zero non e' cambiato niente, e
	 * un'enfasi direbbe il contrario di cio' che e' successo.
	 */
	void PushDamageToken(const FRTDamageTokenView& Token);

	/**
	 * Avvia l'enfasi breve della barra della vita.
	 *
	 * ⚠️ **Enfasi, non sostituzione**: la percentuale resta quella che `SetOverlayView` ha posato. Questa
	 * funzione tocca soltanto la scala di render, che a riposo vale esattamente `(1,1)`.
	 */
	void PulseHealthBar();

	/**
	 * La frazione residua di un'animazione a tempo, sempre in `[0,1]` e **esattamente `0`** a scadenza.
	 *
	 * 🔑 **Statica e pura: e' l'unica regola temporale che questa classe possiede**, ed e' qui invece che
	 * dentro `NativeTick` perche' un test non debba montare un widget — la stessa disciplina di
	 * `ComposeHealthLabel` e `SafeFraction`.
	 *
	 * 🔴 **Lo zero esatto e' il contenuto della funzione, non un dettaglio.** L'AC di `#2455` chiede che la
	 * barra *«torni a riposo senza restare in uno stato transitorio»*: con un ritorno approssimato la scala
	 * si fermerebbe a `1.0001` e la sovrapposizione resterebbe per sempre appena piu' grande, in un punto
	 * dove nessuno guarderebbe piu'.
	 *
	 * ⚠️ `Duration <= 0` risponde `0` — «gia' finita» — e non divide per zero: un'animazione di durata nulla
	 * non e' un errore da segnalare, e' una che non si vede.
	 */
	static float FadeAlpha(float Elapsed, float Duration);

	/**
	 * Mostra il fumetto di una skill di firma sopra questa unita' (`#4025`): posa il testo in MAIUSCOLO, rende
	 * visibile il root e porta il tempo del fumetto al suo `StartAt`, cosi' che nasca con il pop e non a meta'.
	 *
	 * 🔑 **Vive QUI, dentro questo widget, per la stessa ragione di PRIVACY del token di danno** (vedi
	 * `PushDamageToken`): il velo spegne l'intero componente che ospita questa sovrapposizione, quindi il
	 * fumetto di un nemico mai osservato non si vede e non rivela che esiste.
	 *
	 * ⛔ **Non applica niente e non tocca `View`.** L'attivazione e' gia' stata risolta; questa chiamata disegna
	 * un fatto. Il maiuscolo e' presentazione e lo applica il widget: la vista porta il testo com'e' nel kit.
	 *
	 * ⚠️ Senza `SkillCalloutRoot` il widget non fa niente e non crasha: tutti i binding sono opzionali.
	 */
	void PushSkillCallout(const FRTSkillCalloutView& Callout);

	/**
	 * Porta al fumetto il tempo del playback (`ARTTurnManager::SkillCalloutClock`) e ne ridisegna opacita' e
	 * scala con le funzioni pure di `URTHudViewModel` (`SkillCalloutAlpha`, `SkillCalloutScale`).
	 *
	 * 🔴 **Il widget NON misura il tempo del fumetto**, e per questo `NativeTick` non lo tocca: un `DeltaTime`
	 * locale farebbe seguire al fumetto il frame invece del playback, e un hitch lo allungherebbe rispetto alla
	 * sequenza che commenta. Con `Alpha <= 0` il root si collassa: un fumetto trasparente ma presente occupa
	 * ancora il layout.
	 */
	void SetSkillCalloutClock(float Now);

	/**
	 * Chiude la vita del fumetto: il root si collassa. Il testo resta, ma non si vede.
	 *
	 * ⚠️ Non azzera `ActiveCallout`: testo e `StartAt` restano, ma con la vita chiusa nessun nuovo tempo e nessun
	 * ritorno dalla vista strategica lo riaccende. Solo un nuovo `PushSkillCallout` lo fa.
	 */
	void ClearSkillCallout();

	/**
	 * Dice al widget se la vista e' strategica: il fumetto resta collassato finche' lo e', e al ritorno in tattica
	 * riapplica l'ultimo tempo ricevuto (`#4025`).
	 *
	 * ⚠️ **Non e' un giudizio di visibilita' per l'osservatore**: quello e' il velo, che spegne l'intero
	 * componente. Questo e' solo il cambio di vista, inoltrato da `ARTUnit::SetStrategicPresentation`.
	 */
	void SetStrategicPresentation(bool bStrategic);

	/** Vedi `FadeAlpha`: il tempo lo fa scorrere qui, e non decide **nessun** esito (invariante #1). */
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

protected:
	/**
	 * Il catalogo iconografico da cui risolvere le icone di stato.
	 *
	 * 🔴 **Non si eredita risalendo l'albero UMG, e la ragione e' strutturale.**
	 * `URTScreenHudWidgetBase::GetIconCatalog()` risale con `GetTypedOuter` fino a `URTTacticalHUDWidget` —
	 * e quel doc-comment dichiara gia' il proprio limite: *«se un widget viene innestato fuori dal
	 * `WBP_RT_TacticalHUD` questa funzione restituisce `nullptr` in silenzio»*. Questo widget vive dentro un
	 * `UWidgetComponent` su `ARTUnit`: e' **esattamente** quel caso.
	 *
	 * ⚠️ **`EditDefaultsOnly` con un default in C++, e non N assegnazioni a mano.** Il commento di
	 * `GetIconCatalog` scarta l'idea di dichiararlo sulla base perche' *«ogni `WBP_RT_*` avrebbe la propria
	 * copia da assegnare a mano — N occasioni di dimenticarne una»*. Qui **N vale uno**: esiste un solo
	 * `WBP_RT_UnitOverlay`. Il default lo mette il costruttore, e il Blueprint conserva l'ultima parola.
	 *
	 * ⚠️ **Nullo non e' un difetto da nascondere**: `ResolveIcon` restituisce il missing-icon con una
	 * warning che nomina chiave e consumer. A schermo si vede che manca, invece di un buco silenzioso.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	TObjectPtr<URTIconCatalogData> IconCatalog;

	/** Il nome dell'eroe. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> NameText;

	/** La vita, come frazione. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> HealthBar;

	/** La vita, come numeri (`60/100`). */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> HealthText;

	/** Lo scudo, come frazione di `MaxHealth` — la stessa scala che usava il canvas. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UProgressBar> ShieldBar;

	/**
	 * Il contenitore delle icone di stato: il C++ lo **svuota e lo riempie** a ogni aggiornamento.
	 *
	 * ⚠️ Ricostruito e non riciclato: gli stati cambiano di numero e di ordine, e tenere un pool
	 * significherebbe una seconda struttura da mantenere allineata a `View.Statuses` — cioe' un secondo
	 * posto dove sbagliare, per un risparmio che a dieci icone non si misura.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UHorizontalBox> StatusBox;

	/**
	 * Chiamato dopo che il C++ ha posato la vista, per cio' che il **layout** vuole aggiungere.
	 *
	 * 🔑 Esiste ancora, ma non e' piu' il canale del disegno: e' l'estensione. Un `WBP` che non lo
	 * implementa e' completo — a differenza della prima stesura, dove non implementarlo significava non
	 * disegnare niente.
	 */
	UFUNCTION(BlueprintImplementableEvent, Category = "RefactorTactics|HUD")
	void OnOverlayUpdated();

	/**
	 * Il testo del token di danno. **Opzionale come tutti gli altri**: un `WBP` che non lo porta mostra il
	 * resto invece di non compilare.
	 *
	 * ⚠️ **E il degrado e' silenzioso, ed e' dichiarato**: senza questo elemento il colpo non ha una cifra a
	 * schermo e nessun test lo nota, perche' il giudizio sta nella funzione pura e non nel disegno. E' il
	 * motivo per cui la verifica a occhio di `#2455` non e' opzionale.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> DamageTokenText;

	/**
	 * Il contenitore del fumetto della skill (`#4025`): e' lui che appare, si ingrandisce e si dissolve. **Un
	 * `UWidget` qualunque**, perche' il layout puo' farne una bolla (bordo, immagine, testo) e al C++ serve solo
	 * accenderlo, spegnerlo e darne opacita' e scala.
	 *
	 * ⚠️ **Opzionale come tutti gli altri**: senza questo elemento il fumetto non si vede e nessun test lo nota,
	 * perche' il giudizio sta nelle funzioni pure e nel campo dell'unita'. Il `WBP_RT_UnitOverlay` si modifica
	 * solo in Editor, dal clone principale: il C++ resta completo senza.
	 *
	 * 🔴 **Requisito di designer: nel `WBP_RT_UnitOverlay` il root nasce `Collapsed`.** Il C++ lo riafferma a
	 * ogni `SetOverlayView` finche' nessun fumetto e' vivo (come fa col token), quindi un root `Visible` non
	 * sopravvive oltre il primo fotogramma, ma il primo resta visibile e l'asset non deve dipendere dal codice
	 * per essere pulito in Editor. Il riposo dichiarato nell'asset e quello affermato dal codice si tengono entrambi.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UWidget> SkillCalloutRoot;

	/** Il testo del fumetto, in maiuscolo. Opzionale: senza, il root si anima ma non dice niente. */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UTextBlock> SkillCalloutText;

	/**
	 * Il contorno della bolla del fumetto (`#4025`): prende il colore di squadra dal C++ — `SetOverlayView` ci posa
	 * `View.TeamColor` — cosi' chi guarda capisce di chi e' la skill senza leggere il nome. Il colore autorato
	 * nell'asset vale solo finche' la prima vista non arriva.
	 *
	 * 🔑 **Il colore lo mette il C++ e non il grafo del `WBP`**: il ponte MCP non sa creare il getter di una
	 * variabile-widget nel grafo, quindi un binding scritto li' resterebbe a mano; e il C++ e' gia' l'unico
	 * proprietario dei dati per-aggiornamento dell'overlay (nome, barre, stati ricevono la loro parte dalla
	 * stessa vista, nello stesso punto). Un secondo canale per il colore sarebbe un secondo posto dove
	 * dimenticarlo.
	 *
	 * ⚠️ **Opzionale come tutti gli altri**: senza, la bolla resta del colore autorato nell'asset e il fumetto
	 * funziona uguale. Il colore e' quello di squadra e basta: non segue gli avvisi di fuoco amico o di bersaglio
	 * che il nome ha — il fumetto parla della skill, non del bersaglio.
	 */
	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UBorder> SkillCalloutOutline;

	/** Quanto a lungo il token resta visibile prima di svanire. `0` lo spegne. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	float DamageTokenSeconds = 0.9f;

	/**
	 * Quanto a lungo il fumetto della skill resta a schermo, contando dal suo `StartAt` sul tempo del playback.
	 * `0` lo spegne (`SkillCalloutAlpha` risponde 0 per una durata non positiva).
	 */
	UPROPERTY(EditDefaultsOnly, Category = "RefactorTactics|HUD")
	float SkillCalloutSeconds = 3.f;

	/** Quanto dura l'enfasi della barra. `0` la spegne. */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	float HealthPulseSeconds = 0.35f;

	/**
	 * Di quanto la barra si ingrandisce al culmine dell'enfasi.
	 *
	 * ⚠️ Piccolo apposta: sopra un cilindro la barra e' larga poche decine di pixel, e un'enfasi vistosa
	 * uscirebbe dalla `DrawSize` di `220x228` che `ARTUnit` assegna al componente.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	float HealthPulseScale = 0.18f;

private:
	/**
	 * Il tempo trascorso da quando il token e' comparso, e da quando l'enfasi e' cominciata.
	 *
	 * ⚠️ **Stato di sola presentazione**, `Transient` per costruzione (non sono `UPROPERTY` serializzate):
	 * non entra in `MapState`, in nessuno snapshot, nel TurnLog ne' in `StateHash`. Un colpo mostrato due
	 * volte disegna due volte e **non cambia nessun esito** — e' l'invariante #1 letta dal lato della
	 * presentazione.
	 */
	float TokenElapsed = 0.f;
	float PulseElapsed = 0.f;

	/** Se un'animazione e' in corso. Spente entrambe, `NativeTick` non tocca nulla. */
	bool bTokenActive = false;
	bool bPulseActive = false;

	/**
	 * Ridisegna visibilita', opacita' e scala del fumetto dallo stato che il widget tiene: l'unica funzione che
	 * decide, chiamata da `PushSkillCallout`, `SetSkillCalloutClock`, `ClearSkillCallout` e
	 * `SetStrategicPresentation`. Una sola porta, cosi' le quattro non possono divergere.
	 */
	void ApplySkillCalloutState();

	/**
	 * Il fumetto in corso e il tempo del playback che l'ultima chiamata ha portato. ⚠️ Nessuno dei due e' un
	 * `DeltaTime`: `CalloutNow` lo scrive soltanto `SetSkillCalloutClock` (e `PushSkillCallout`, al `StartAt`).
	 * Stato di sola presentazione, non `UPROPERTY`, fuori da ogni hash.
	 */
	FRTSkillCalloutView ActiveCallout;
	float CalloutNow = 0.f;

	/** Fra `PushSkillCallout` e `ClearSkillCallout`. */
	bool bCalloutLive = false;

	/** Scritto da `SetStrategicPresentation`: la vista strategica tiene il fumetto collassato. */
	bool bStrategicPresentation = false;
};
