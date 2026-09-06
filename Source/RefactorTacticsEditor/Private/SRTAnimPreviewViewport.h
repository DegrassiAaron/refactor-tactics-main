#pragma once

#include "CoreMinimal.h"
#include "PreviewScene.h"
#include "SEditorViewport.h"

class UAnimSequence;
class USkeletalMeshComponent;

/**
 * L'anteprima di UNA clip, su uno skeletal in una scena sua.
 *
 * 🔴 **Non c'era niente da riusare, ed e' stato misurato prima di scriverlo.** Il corpo di #2443
 * elencava `RTScenarioPreviewActor` fra le capability riusabili: quel file ha **zero** occorrenze di
 * `Skeletal`, `AnimSequence`, `AnimInstance` — mostra scenari esagonali, non personaggi. La riga «se
 * esiste gia' infrastructure utile, riusarla» era vera per il browser e falsa per l'anteprima.
 *
 * ⚠️ **Serve a giudicare, non a decidere.** Deformazioni, silhouette, timing, partenza, recovery: sono
 * le cose che `Promoted` significa, e senza vederle la promozione sarebbe una firma su una descrizione.
 * Nessuna misura presa qui entra nel gioco — questa scena non ha un `ARTUnit`, non ha un `TurnManager`,
 * e la sua velocita' di riproduzione non tocca niente (invariante #1).
 */
class SRTAnimPreviewViewport : public SEditorViewport, public FGCObject
{
public:
	SLATE_BEGIN_ARGS(SRTAnimPreviewViewport) {}
	SLATE_END_ARGS()

	void Construct(const FArguments& InArgs);
	virtual ~SRTAnimPreviewViewport() override;

	/**
	 * Mostra questa clip. `nullptr` svuota la scena.
	 *
	 * ⚠️ **Lo skeletal viene dallo Skeleton della clip**, non da un default: montare la clip di Gadget su
	 * una mesh qualunque produrrebbe deformazioni che sembrano un difetto della clip. Se lo skeleton non
	 * ha una mesh d'anteprima, la scena resta vuota e `LastError` lo dice — invece di mostrare qualcosa
	 * di sbagliato.
	 */
	void SetClip(UAnimSequence* Clip);

	// ── Trasporto ───────────────────────────────────────────────────────────────────────────────────
	void Play();
	void Pause();
	void Restart();
	void SetLooping(bool bLoop);
	void SetPlayRate(float Rate);

	bool IsPlaying() const;
	bool IsLooping() const { return bLooping; }
	float GetPlayRate() const { return PlayRate; }

	/** Perche' l'anteprima e' vuota, o stringa vuota se non lo e'. */
	const FString& GetLastError() const { return LastError; }

	// FGCObject
	virtual void AddReferencedObjects(FReferenceCollector& Collector) override;
	virtual FString GetReferencerName() const override { return TEXT("SRTAnimPreviewViewport"); }

protected:
	virtual TSharedRef<FEditorViewportClient> MakeEditorViewportClient() override;

private:
	/**
	 * Stacca la mesh corrente dalla scena e la distrugge, in quest'ordine.
	 *
	 * 🔑 **L'ordine e' il punto, non un dettaglio**: `DestroyComponent` non toglie il componente dalla
	 * lista di `FPreviewScene`, che continua a puntarlo. Invertirlo — o ometterlo — lascia nella scena un
	 * puntatore a un oggetto morto, e alla distruzione della scena `Uninitialize` ci chiama sopra
	 * `UnregisterComponent`. E' il crash di #2540.
	 */
	void ClearMesh();

	TSharedPtr<FPreviewScene> PreviewScene;
	TObjectPtr<USkeletalMeshComponent> Mesh = nullptr;

	bool bLooping = true;
	float PlayRate = 1.f;
	FString LastError;
};
