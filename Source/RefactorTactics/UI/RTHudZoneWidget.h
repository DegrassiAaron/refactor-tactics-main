#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "RTHudZoneWidget.generated.h"

/**
 * Le otto celle della griglia 3x3 che circonda il centro tattico.
 *
 * 🔑 **Il centro NON e' un valore di questo enum**, e l'assenza e' deliberata: e' definito da cio' che non
 * contiene (`progettazione-hud.md` §3.1), e un `ERTHudZone::Center` sarebbe un invito a riempirlo. Il layer
 * §4.2 disegna li' path, AoE e barre ancorate sopra la mappa.
 *
 * ⚠️ **L'ordine dei valori e' significativo**: `BlockoutColor` deriva la tinta dall'indice, quindi
 * riordinarli rimescola i colori del blockout. Non e' un difetto — sono colori da cantiere — ma chi
 * riordina se ne accorga invece di sorprendersi.
 */
UENUM(BlueprintType)
enum class ERTHudZone : uint8
{
	TopLeft      UMETA(DisplayName = "Alto sinistra"),
	TopCenter    UMETA(DisplayName = "Alto centro"),
	TopRight     UMETA(DisplayName = "Alto destra"),
	MiddleLeft   UMETA(DisplayName = "Mezzo sinistra"),
	MiddleRight  UMETA(DisplayName = "Mezzo destra"),
	BottomLeft   UMETA(DisplayName = "Basso sinistra"),
	BottomCenter UMETA(DisplayName = "Basso centro"),
	BottomRight  UMETA(DisplayName = "Basso destra"),

	/** Sentinella per il conteggio. Non e' una zona: non assegnarla mai a un `ZoneId`. */
	Count        UMETA(Hidden)
};

/**
 * `WBP_RT_HudZone` — UNA zona dello Screen HUD: un contenitore geometrico con un `NamedSlot Content`.
 *
 * 🔑 **`UUserWidget` e non `URTScreenHudWidgetBase`, e la differenza e' il punto.** Una zona e' geometria:
 * non interroga il ViewModel, non sa cosa sia un turno, non ha niente da aggiornare quando la partita
 * cambia. Derivarla dalla base le darebbe un accesso al ViewModel che non le serve — e un accesso che
 * esiste, prima o poi qualcuno lo usa. E' la stessa disciplina di `URTActionSlotWidget` e
 * `URTFastDecisionOptionWidget`.
 *
 * 🔑 **`ZoneId` esiste per rendere la zona MISURABILE.** Prima di questa classe una zona era un
 * `UCanvasPanelSlot` con un nome scelto nel Designer: per un test, indistinguibile da qualunque altro
 * nodo. E' la condizione in cui `cc5ca967` ha risalvato l'albero allo stato precedente al fix di `#2760`
 * con la suite verde.
 */
UCLASS(BlueprintType)
class REFACTORTACTICS_API URTHudZoneWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	/**
	 * Quale cella della griglia e' questa istanza.
	 *
	 * ⚠️ `EditAnywhere` e non `EditDefaultsOnly`: le otto istanze condividono la stessa classe e si
	 * distinguono **per istanza**. E' l'opposto di `bShowDebug` su `URTScreenHudWidgetBase`, che e'
	 * `EditDefaultsOnly` proprio per non restare acceso in una schermata dimenticata.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	ERTHudZone ZoneId = ERTHudZone::TopLeft;

	/**
	 * Accende i bordi spessi colorati da cantiere. Si spegne quando il contenuto della zona arriva.
	 *
	 * ⛔ **Non e' uno stile di produzione**: vedi `BlockoutColor`.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|HUD")
	bool bBlockoutVisible = true;

	/**
	 * Il colore del bordo da cantiere, DERIVATO da `ZoneId`.
	 *
	 * 🔑 **Pura e statica**: stesso ingresso, stesso colore, e la mappa zona -> tinta sta in un punto solo
	 * invece che in otto istanze da tenere allineate a mano.
	 *
	 * ⛔ **Queste tinte NON vengono dalla palette di gioco, e non passano i criteri di accessibilita' del
	 * progetto.** La palette e' Okabe-Ito e ogni tinta e' impegnata (`spec-icon-card-grammar.md`):
	 * `#009E73` e' Movement, `#D55E00` Attack, `#0072B2` Utility, `#56B4E9` Defense. Un bordo verde
	 * verrebbe letto come «movimento». Qui si usano tonalita' HSV equispaziate che nella palette non
	 * esistono: stonano di proposito, perche' un bordo da cantiere che sembra design finito e' un bordo che
	 * resta montato. Non raggiungono il giocatore; se ci arrivassero, il difetto sarebbe quello.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|HUD")
	static FLinearColor BlockoutColor(ERTHudZone Zone);

	/** L'etichetta diagnostica di una zona, per i messaggi di test e i log. Mai a schermo. */
	static FString ZoneName(ERTHudZone Zone);
};
