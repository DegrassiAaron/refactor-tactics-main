#pragma once

#include "CoreMinimal.h"
#include "UI/RTHudLayout.h" // #3661: il riquadro libero e' la scena, e la scena viene dal dato delle fasce

/**
 * LA GEOMETRIA DELLO SCHERMO, per i gate che la misurano invece di guardarla. Un rettangolo, la loro
 * intersezione, e il rettangolo della scena che nessuna zona puo' toccare (#3661, D-497).
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


	struct FRect
	{
		float Left = 0.f;
		float Top = 0.f;
		float Right = 0.f;
		float Bottom = 0.f;

		float Width() const { return Right - Left; }
		float Height() const { return Bottom - Top; }
	};

	/**
	 * **Il riquadro che nessuna zona puo' toccare e' la scena**, per un viewport che in unita' Slate misura
	 * `Larghezza x Altezza`.
	 *
	 * ⌫ **Fino al 2026-10-11 era il 60% x 60% centrato** (`CenterFraction`, `X 384..1536` e `Y 216..864`):
	 * una frazione scritta qui, un'altra griglia in `RTGrigliaZone`, e gli anchor dell'asset. Con D-497 il
	 * dato e' uno solo, `RTHudLayout::Bands()`, e qui non resta nessuna costante propria.
	 */
	inline FRect SceneKeepOut(float Larghezza = RefWidth, float Altezza = RefHeight)
	{
		const FBox2D Scena = RTHudLayout::SceneRectSlate(FVector2D(Larghezza, Altezza), RTHudLayout::Bands());
		return FRect{ static_cast<float>(Scena.Min.X), static_cast<float>(Scena.Min.Y),
			static_cast<float>(Scena.Max.X), static_cast<float>(Scena.Max.Y) };
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

/**
 * **Le risoluzioni a cui #3661 chiede coerenza**: 16:9, 21:9 e due 16:10. Con la scala `ShortestSide` il 16:10
 * e' il caso che morde: le fasce sono misure in unita' Slate, e in unita' Slate lo schermo e' piu' stretto.
 */
namespace RTHudRisoluzioni
{
	struct FRisoluzione
	{
		int32 Larghezza = 0;
		int32 Altezza = 0;
		const TCHAR* Nome = TEXT("");
	};

	inline const TArray<FRisoluzione>& Tutte()
	{
		static const TArray<FRisoluzione> Elenco = {
			{ 1920, 1080, TEXT("1920x1080 (16:9)") },
			{ 2560, 1080, TEXT("2560x1080 (21:9)") },
			{ 1280,  800, TEXT("1280x800 (16:10)") },
			{ 1920, 1200, TEXT("1920x1200 (16:10)") } };
		return Elenco;
	}

	/** Il viewport in unita' Slate, con la scala che UE vi applica (`RTHudLayout::UiScaleFor`). */
	inline FVector2D InSlate(const FRisoluzione& R)
	{
		const float Scala = RTHudLayout::UiScaleFor(FIntPoint(R.Larghezza, R.Altezza));
		return FVector2D(R.Larghezza / Scala, R.Altezza / Scala);
	}
}
