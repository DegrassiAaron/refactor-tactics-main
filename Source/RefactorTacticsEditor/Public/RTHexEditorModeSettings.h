#pragma once

#include "CoreMinimal.h"
#include "RTHexEditorModeSettings.generated.h"

/**
 * LE IMPOSTAZIONI DEL MODE Hex Map — una sola istanza per sessione di editing (#921).
 *
 * 🔴 **Nasce fuori dai `UInteractiveToolPropertySet`, ed e' il punto.** #921 ha misurato il difetto:
 * `bShowOverlay` viveva in due `PropertySet` distinti, ciascuno con la propria istanza creata in `Setup()`,
 * quindi accenderlo in Select non lo accendeva in Paint e cambiando strumento l'impostazione «si perdeva».
 * Uno stato che deve sopravvivere al cambio di strumento non puo' stare dentro lo strumento — e' la stessa
 * ragione per cui la selezione condivisa vive in `URTHexSelectionStore` (#1864).
 *
 * ⚠️ **L'alternativa scartata e' una property set base condivisa fra i tool**: lascerebbe il flag nel
 * pannello di *ogni* strumento, cioe' sette posti che mostrano la stessa impostazione. `UEdMode` ha gia' il
 * meccanismo giusto — `SettingsClass` / `SettingsObject` — e il toolkit mostra questo oggetto **sopra** i
 * tool, non dentro uno di essi (`FModeToolkit::SetModeSettingsObject`).
 *
 * 🔑 **`config` non e' decorativo, ed e' la ragione per cui questa classe non e' `Transient`.** I sette
 * property set del mode sono `UCLASS(Transient)` — lo stato di uno strumento non si conserva — quindi un
 * `UPROPERTY(config)` dichiarato li' dentro non persisterebbe, e il difetto sarebbe invisibile finche'
 * qualcuno non riapre l'editor. Qui `UEdMode::Enter` fa `LoadConfig()` e `UEdMode::Exit` fa `SaveConfig()`:
 * il flag sopravvive al rientro nel mode **e** alla chiusura dell'editor.
 */
UCLASS(config = EditorPerProjectUserSettings)
class REFACTORTACTICSEDITOR_API URTHexEditorModeSettings : public UObject
{
	GENERATED_BODY()

public:
	/**
	 * Disegna le superfici di tutte le celle sotto lo strumento attivo, qualunque esso sia.
	 *
	 * ⚠️ **Il contenuto dell'overlay non e' deciso qui**: lo disegna `RTHexEditor::DrawSurfaceOverlay`, che
	 * questa issue non tocca. Questo flag dice *se*, non *cosa*.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Hex Map",
		meta = (DisplayName = "Mostra overlay superfici",
			ToolTip = "Colora ogni cella secondo la propria superficie. Vale per tutti gli strumenti del mode."))
	bool bShowSurfaceOverlay = false;

	/**
	 * La griglia di lavoro: dove le celle NON esistono ancora (#622).
	 *
	 * 🔑 **Default `true`, ed e' un criterio del DoD**: *«entrando in Hex Map mode la griglia di lavoro e'
	 * visibile senza accendere nulla»*. E' l'opposto di `bShowSurfaceOverlay`, che nasce spento perche'
	 * ridipinge celle che si vedono gia'.
	 *
	 * ⚠️ **Dal primo `Exit()` vince la scelta di chi lavora, non questo default.** `UEdMode::Exit` fa
	 * `SaveConfig()` in `EditorPerProjectUserSettings.ini`: il criterio del DoD parla di chi entra la prima
	 * volta, e chi la spegne la ritrova spenta — che e' il comportamento giusto per un'impostazione.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Hex Map",
		meta = (DisplayName = "Mostra griglia di lavoro",
			ToolTip = "Marca le coordinate vuote attorno alla mappa, per vedere dove cadra' la prossima cella."))
	bool bShowWorkGrid = true;

	/**
	 * Di quanti anelli la griglia di lavoro deborda oltre le celle che esistono gia'.
	 *
	 * ⚠️ **Il tetto del clamp e' il quarto criterio del DoD** (*«l'estensione non cresce senza limite»*), e
	 * non e' l'unica difesa: `RTHexWorkGrid::BuildPlan` porta anche un tetto sul NUMERO di esagoni, perche'
	 * su una mappa sparsa un margine piccolo puo' comunque dilatare moltissimo. Il clamp difende dal gesto,
	 * il tetto dal dato.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Hex Map",
		meta = (DisplayName = "Griglia: margine (anelli)", ClampMin = "0", ClampMax = "6",
			ToolTip = "Quanti anelli di celle vuote mostrare attorno alla mappa esistente."))
	int32 WorkGridMargin = 2;

	/**
	 * Raggio del seme quando il layer attivo e' **vuoto**.
	 *
	 * 🔑 **Separato da `WorkGridMargin` perche' significa un'altra cosa**: un margine dice «quanto oltre il
	 * gia' disegnato», un seme «da dove si comincia quando non c'e' niente». Con un parametro solo, chi lo
	 * alza per vedere piu' bordo si ritroverebbe un seme enorme su una mappa vuota.
	 *
	 * ⛔ **Non e' `DemoRadius`**, ed e' la differenza che la issue chiede: quello e' un campo dell'actor che
	 * fa disegnare alla board celle che il dato non contiene. Questo e' un'impostazione dello strumento,
	 * disegna solo fantasmi, e non entra mai nell'asset.
	 */
	UPROPERTY(config, EditAnywhere, Category = "Hex Map",
		meta = (DisplayName = "Griglia: raggio del seme", ClampMin = "0", ClampMax = "8",
			ToolTip = "Quanto e' grande la griglia quando il layer attivo non ha ancora nessuna cella."))
	int32 WorkGridSeedRadius = 3;

	/**
	 * CHE COSA CONTIENE la mappa aperta (#1186) — sola lettura, e si vede **senza premere nulla**.
	 *
	 * 🔑 **Nasce da mezza giornata persa in seduta `U21`**, dove la domanda «quanti piani ha
	 * questa mappa?» ha prodotto in sequenza una misura headless che rispondeva a un'altra domanda, un
	 * numero letto dal Play che descriveva un'altra mappa, una prenotazione su una necessita' inesistente e
	 * un cambio di `MapAsset` che nessuno aveva notato. La risposta e' arrivata solo da `Frame Map`, cioe'
	 * da un comando che si **preme**: un dato che si consulta guardando deve vedersi guardando.
	 *
	 * ⛔ **Nessuno di questi campi e' modificabile**, ed e' un vincolo della issue: `ActiveLayer` si
	 * imposta gia' dai tool (#567), e un readout che scrive e' un tool. Sono `VisibleAnywhere` — la
	 * convenzione che `URTHexGeometryToolProperties` ha stabilito.
	 *
	 * ⚠️ **E non sono `config`**: descrivono la mappa aperta adesso, non una preferenza. Scriverli
	 * in `EditorPerProjectUserSettings.ini` significherebbe rileggere all'avvio la descrizione di un'altra
	 * mappa — che e' esattamente il difetto numero 2 di `U21`.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Hex Map|Mappa aperta",
		meta = (DisplayName = "Asset collegato"))
	FString MappaAsset;

	UPROPERTY(VisibleAnywhere, Category = "Hex Map|Mappa aperta", meta = (DisplayName = "Celle"))
	FString MappaCelle;

	UPROPERTY(VisibleAnywhere, Category = "Hex Map|Mappa aperta", meta = (DisplayName = "Layer"))
	FString MappaLayer;

	UPROPERTY(VisibleAnywhere, Category = "Hex Map|Mappa aperta", meta = (DisplayName = "Layer attivo"))
	FString MappaLayerAttivo;

	/**
	 * CHE COSA DICE IL VALIDATORE, senza premere niente (#1864, casella 8).
	 *
	 * 🔴 **E' il canale che mancava.** La issue chiede *«si rifiuta il gesto **o si segnala**»*: il rifiuto
	 * tipizzato esisteva (`ERTMapEditOutcome`), il segnalare no — nel modulo Editor `ValidateMap` non era
	 * chiamata da nessuna riga di produzione.
	 *
	 * ⛔ **Non un `UE_LOG`, e non un bottone.** L'Output Log e' un pannello che si deve avere aperto, ed e'
	 * il difetto che `RTMapTemplateValidationHookTests` documenta: regole scritte, testate e verdi che non
	 * eseguiva nessuno. Un bottone e' il difetto di `U21`, che questo pannello esiste per chiudere — *«un
	 * dato che si consulta guardando deve vedersi guardando»*.
	 *
	 * 🔑 **Si aggiorna sullo STESSO trigger degli altri readout**, cioe' quando `Revision` cambia e non a
	 * ogni fotogramma: e' la guardia che `#1186` ha gia' costruito, e senza di essa una `ValidateMap()` per
	 * tick sarebbe un costo che nessuno ha chiesto.
	 *
	 * ⚠️ **Non e' `config`**, come gli altri quattro: descrive la mappa aperta adesso, non una preferenza.
	 */
	UPROPERTY(VisibleAnywhere, Category = "Hex Map|Mappa aperta", meta = (DisplayName = "Validazione"))
	FString MappaValidazione;
};
