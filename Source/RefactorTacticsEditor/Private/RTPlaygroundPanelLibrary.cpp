#include "RTPlaygroundPanelLibrary.h"

#include "Camera/RTCameraPawn.h"

#include "RTPlaygroundLayout.h"
#include "World/RTGrayboxUnitFacingFixture.h"

namespace
{
	/** Il nome nudo della mappa del laboratorio. L'unica che il pannello riconosce. */
	const TCHAR* RTPanelMapName = TEXT("L_GrayKitPlayground");

	/**
	 * Le tre righe di `DIAGNOSTICS`, alla lettera.
	 *
	 * ⛔ Cambiarle e' cambiare cio' che il pannello **dichiara di non poter fare**, quindi il test le
	 * confronta una per una invece di contarle.
	 */
	const TCHAR* RTPanelDiagnostics[3] = {
		TEXT("Mode: PRESENTATION ONLY"),
		TEXT("Gameplay authority: NONE"),
		TEXT("Runtime state mutation: NONE"),
	};

	/**
	 * `Close` · `Tactical` · `Overview`, gli `ArmLength` della seduta `U25`.
	 *
	 * ⚠️ Sono la DICHIARAZIONE, non una copia verificabile: `U25` li tiene in prosa. Vedi la doc della
	 * funzione che li espone.
	 */
	// ⛔ I tre bracci NON stanno piu' qui. Erano `{100, 450, 4000}` scritti a mano — e sono, alla lettera,
	// i valori che `ARTCameraPawn` dichiara come `MinArmLength`, `MatchStartArmLength`, `MaxArmLength`.
	// Una copia: il legame con la camera del gioco era una **citazione, non un controllo**, ed e' cio' che
	// la seduta `U41` denunciava. Ora si derivano dal CDO, e non possono divergere.

	FRTPlaygroundStationInfo MakeInfo(const RTPlayground::FStation& Station)
	{
		FRTPlaygroundStationInfo Info;
		Info.Number = Station.Number;
		Info.Name   = FString(Station.Name);
		Info.bLive  = Station.bLive;

		// La conversione metri -> unita' avviene QUI una volta sola: `WorldFromMetres` resta l'unico punto
		// in cui le due misure si toccano, come il suo stesso commento prescrive.
		Info.MinWorld = FVector2D(RTPlayground::WorldFromMetres(Station.Bounds.Min.X),
		                          RTPlayground::WorldFromMetres(Station.Bounds.Min.Y));
		Info.MaxWorld = FVector2D(RTPlayground::WorldFromMetres(Station.Bounds.Max.X),
		                          RTPlayground::WorldFromMetres(Station.Bounds.Max.Y));

		const FVector2D CentreMetres = Station.Bounds.GetCenter();
		Info.CentreWorld = FVector(RTPlayground::WorldFromMetres(CentreMetres.X),
		                           RTPlayground::WorldFromMetres(CentreMetres.Y),
		                           0.0);
		return Info;
	}
}

TArray<FRTPlaygroundStationInfo> URTPlaygroundPanelLibrary::GetStations()
{
	// ⛔ Nessuna tabella qui: si delega. Una seconda planimetria e' il difetto di `#1459`.
	TArray<FRTPlaygroundStationInfo> Out;
	for (const RTPlayground::FStation& Station : RTPlayground::Stations())
	{
		Out.Add(MakeInfo(Station));
	}
	return Out;
}

bool URTPlaygroundPanelLibrary::FindStation(int32 Number, FRTPlaygroundStationInfo& OutStation)
{
	if (const RTPlayground::FStation* Station = RTPlayground::FindStation(Number))
	{
		OutStation = MakeInfo(*Station);
		return true;
	}
	// ⚠️ `OutStation` resta com'era: una station vuota restituita come valida sarebbe un pad a `(0,0)`
	// di lato zero, e `Focus` ci porterebbe la camera senza che niente segnali l'errore.
	return false;
}

TArray<FString> URTPlaygroundPanelLibrary::GetFacingOptions()
{
	TArray<FString> Out;
	const UEnum* Enum = StaticEnum<ERTHexDirection>();
	if (!Enum)
	{
		return Out;
	}

	// ⚠️ `NumEnums() - 1`: l'ultima voce e' il `_MAX` che UHT aggiunge, e non e' una direzione. Includerla
	// darebbe SETTE voci a un dropdown che ne deve avere sei — e il test lo direbbe.
	for (int32 I = 0; I < Enum->NumEnums() - 1; ++I)
	{
		Out.Add(Enum->GetNameStringByIndex(I));
	}
	return Out;
}

bool URTPlaygroundPanelLibrary::ParseFacingOption(const FString& Option, ERTHexDirection& OutFacing)
{
	const UEnum* Enum = StaticEnum<ERTHexDirection>();
	if (!Enum)
	{
		return false;
	}
	const int64 Value = Enum->GetValueByNameString(Option);
	if (Value == INDEX_NONE || Value >= Enum->NumEnums() - 1)
	{
		return false; // una stringa fuori set non diventa `E` per ripiego: sarebbe un dato inventato
	}
	OutFacing = static_cast<ERTHexDirection>(Value);
	return true;
}

FString URTPlaygroundPanelLibrary::StationOptionLabel(const FRTPlaygroundStationInfo& Station)
{
	// `%02d` e non `%d`: incolonnate, otto voci si leggono come una lista; disallineate sembrano otto
	// frasi diverse. E i due spazi separano senza aggiungere un simbolo da interpretare.
	//
	// 🔑 **`[PLANNED]` e' il primo consumatore di `bLive`.** Il campo era *dichiarato* e *trasportato*
	// fin qui, e **letto da nessuno**: la combo elencava otto station tutte uguali, e il `done_when` di
	// `U41` chiede l'opposto — che una station pianificata **non sembri funzionante**. Un pad con il
	// signage e nessun sistema dietro, presentato come le altre, e' il pannello che promette.
	return Station.bLive
		? FString::Printf(TEXT("%02d  %s"), Station.Number, *Station.Name)
		: FString::Printf(TEXT("%02d  %s  [PLANNED]"), Station.Number, *Station.Name);
}

bool URTPlaygroundPanelLibrary::ParseStationOption(const FString& Option, int32& OutNumber)
{
	OutNumber = 0;

	// ⛔ Non si parsifica «la parte prima dello spazio»: un nome che cominciasse con una cifra
	// ingannerebbe quella regola. Si cerca la station il cui LABEL coincide — l'unica lettura che non
	// puo' divergere dalla scrittura, perche' usa la stessa funzione.
	for (const FRTPlaygroundStationInfo& Station : GetStations())
	{
		if (StationOptionLabel(Station).Equals(Option))
		{
			OutNumber = Station.Number;
			return true;
		}
	}
	return false;
}

FRTPlaygroundMapState URTPlaygroundPanelLibrary::EvaluateMapState(const FString& OpenMapName)
{
	FRTPlaygroundMapState Out;

	// Il nome nudo, qualunque cosa arrivi: `/Game/RT/Maps/Dev/L_GrayKitPlayground/L_GrayKitPlayground`
	// e `L_GrayKitPlayground` sono la stessa mappa, e distinguerli non e' una decisione del pannello.
	FString Bare = OpenMapName;
	int32 Slash = INDEX_NONE;
	if (Bare.FindLastChar(TEXT('/'), Slash)) { Bare = Bare.RightChop(Slash + 1); }
	int32 Dot = INDEX_NONE;
	if (Bare.FindChar(TEXT('.'), Dot)) { Bare = Bare.Left(Dot); }

	Out.MapName = Bare;

	if (Bare.IsEmpty())
	{
		Out.State  = ERTPlaygroundReadiness::Error;
		Out.Reason = TEXT("Nessuna mappa aperta.");
		return Out;
	}
	if (Bare != RTPanelMapName)
	{
		Out.State = ERTPlaygroundReadiness::Error;
		// ⚠️ La ragione NOMINA entrambe le mappe: «errore» da solo manda a cercare un guasto dove c'e'
		// soltanto la mappa sbagliata aperta.
		Out.Reason = FString::Printf(
			TEXT("Aperta '%s': il Playground vive in '%s'."), *Bare, RTPanelMapName);
		return Out;
	}

	Out.State = ERTPlaygroundReadiness::Ready;
	return Out;
}

bool URTPlaygroundPanelLibrary::ApplyFixtureFacing(ARTGrayboxUnitFacingFixture* Fixture, ERTHexDirection Facing)
{
	if (!Fixture)
	{
		return false;
	}
	Fixture->Facing = Facing;
	// 🔑 **Il gesto che il Blueprint non puo' fare da solo.** Senza, il valore cambia e il marker resta
	// fermo — la trappola descritta nel *Why* della issue.
	Fixture->RerunConstructionScripts();
	return true;
}

bool URTPlaygroundPanelLibrary::ApplyFixtureParameters(ARTGrayboxUnitFacingFixture* Fixture,
	float BodyRadius, float BodyHeight, float FaceHeight, float MarkerLength)
{
	if (!Fixture)
	{
		return false;
	}
	Fixture->BodyRadius   = BodyRadius;
	Fixture->BodyHeight   = BodyHeight;
	Fixture->FaceHeight   = FaceHeight;
	Fixture->MarkerLength = MarkerLength;
	Fixture->RerunConstructionScripts();
	return true;
}

bool URTPlaygroundPanelLibrary::ResetFixture(ARTGrayboxUnitFacingFixture* Fixture)
{
	if (!Fixture)
	{
		return false;
	}

	// ⛔ Dal CDO, non da letterali: i default hanno un solo owner, ed e' la classe.
	const ARTGrayboxUnitFacingFixture* Defaults = GetDefault<ARTGrayboxUnitFacingFixture>();
	Fixture->Facing       = Defaults->Facing;
	Fixture->BodyRadius   = Defaults->BodyRadius;
	Fixture->BodyHeight   = Defaults->BodyHeight;
	Fixture->FaceHeight   = Defaults->FaceHeight;
	Fixture->MarkerLength = Defaults->MarkerLength;
	Fixture->RerunConstructionScripts();
	return true;
}

TArray<FString> URTPlaygroundPanelLibrary::DiagnosticsLines()
{
	TArray<FString> Out;
	for (const TCHAR* Line : RTPanelDiagnostics)
	{
		Out.Add(FString(Line));
	}
	return Out;
}

TArray<float> URTPlaygroundPanelLibrary::CameraPresetArmLengths()
{
	// Dal CDO della camera del gioco: ravvicinata, di gioco, tattica. Se un giorno il gioco cambia le
	// sue distanze, il pannello le segue invece di mentire con tre numeri fermi.
	const ARTCameraPawn* Camera = GetDefault<ARTCameraPawn>();
	return { Camera->GetMinArmLength(), Camera->GetMatchStartArmLength(), Camera->GetMaxArmLength() };
}

float URTPlaygroundPanelLibrary::CameraPresetPitch()
{
	// 🔴 **Il pannello inquadrava a picco.** Il grafo usava `-90`, un numero che avevo scelto io: fuori
	// dal clamp `[-89, 0]` che `ARTCameraPawn::AddPitch` impone, e soprattutto **non e' la vista del
	// gioco**. Chi guardava vedeva i tetti. Il pitch e' quello della camera tattica, e viene da li'.
	return GetDefault<ARTCameraPawn>()->GetCameraPitch();
}
