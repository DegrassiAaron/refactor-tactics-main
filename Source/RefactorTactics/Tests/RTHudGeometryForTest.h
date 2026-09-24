#pragma once

#include "CoreMinimal.h"

/**
 * LA GEOMETRIA DELLO SCHERMO, per i gate che la misurano invece di guardarla. Un rettangolo, la loro
 * intersezione, e il riquadro centrale che nessuno puo' toccare.
 *
 * 🔴 **Perche' un header e non due copie.** Questa aritmetica nasceva dentro
 * `RTMatchWidgetAssetTests.cpp`, dove presidia le zone dell'HUD; #3319 ha avuto bisogno dello stesso
 * keep-out per il Context Inspector, che non e' una zona dell'HUD e non ha un `.uasset`. Copiare i
 * numeri avrebbe prodotto due soglie che divergono in silenzio appena una delle due viene indurita —
 * ed e' proprio il difetto che `RefactorTactics.Meta.AnonymousHelpersDoNotCollideUnderUnity` esiste per
 * prendere. **Namespace NOMINATO e funzioni `inline`**: la forma che non collide sotto Unity build, la
 * stessa di `RTReflectedFieldsForTest.h`.
 *
 * ⛔ **Qui sta solo la meta' PURA, e la divisione e' voluta.** `RettangoloDellaZona` — che legge un
 * `FAnchorData` di `UCanvasPanelSlot` — e tutto `RTGrigliaZone` — che parla di `ERTHudZone` — restano in
 * `RTMatchWidgetAssetTests.cpp`: dipendono da UMG, e un header di `Tests/` e' compilato in **ogni**
 * target. Chi ha bisogno di un rettangolo non deve pagare UMG per averlo.
 *
 * ⚠️ **L'oracolo e' una geometria DICHIARATA, non un fotogramma renderizzato.** Vale per cio' che i dati
 * dicono che occupera' lo schermo; un widget spostato a runtime da Blueprint sfugge di qui e resta di
 * PIE.
 */
namespace RTCenterFree
{
	/** La risoluzione a cui il DoD di `#613` chiede coerenza. Cambiarla cambia il significato del gate. */
	constexpr float RefWidth = 1920.f;
	constexpr float RefHeight = 1080.f;

	/**
	 * Il lato del riquadro centrale che nessuna zona puo' toccare, in frazione dello schermo.
	 *
	 * 🔑 **La soglia e' dichiarata qui e non altrove**: il riquadro e' il **60% x 60% centrato** della
	 * risoluzione di riferimento, cioe' `X 384..1536` e `Y 216..864`. Non e' un numero sacro — e' un
	 * numero **scritto**, che si discute in una issue invece che in un playtest.
	 */
	constexpr float CenterFraction = 0.6f;

	struct FRect
	{
		float Left = 0.f;
		float Top = 0.f;
		float Right = 0.f;
		float Bottom = 0.f;

		float Width() const { return Right - Left; }
		float Height() const { return Bottom - Top; }
	};

	/** Il riquadro che deve restare sgombro, in pixel di riferimento. */
	inline FRect CenterKeepOut()
	{
		const float MargineX = RefWidth * (1.f - CenterFraction) * 0.5f;
		const float MargineY = RefHeight * (1.f - CenterFraction) * 0.5f;
		return FRect{ MargineX, MargineY, RefWidth - MargineX, RefHeight - MargineY };
	}

	/** Intersezione: larghezza o altezza <= 0 significa che i due riquadri non si toccano. */
	inline FRect Intersezione(const FRect& A, const FRect& B)
	{
		return FRect{
			FMath::Max(A.Left, B.Left),
			FMath::Max(A.Top, B.Top),
			FMath::Min(A.Right, B.Right),
			FMath::Min(A.Bottom, B.Bottom) };
	}

	inline bool SiToccano(const FRect& A, const FRect& B)
	{
		const FRect I = Intersezione(A, B);
		return I.Width() > 0.f && I.Height() > 0.f;
	}

	inline FString Descrivi(const FRect& R)
	{
		return FString::Printf(TEXT("X %.0f..%.0f  Y %.0f..%.0f  (%.0fx%.0f)"),
			R.Left, R.Right, R.Top, R.Bottom, R.Width(), R.Height());
	}
}
