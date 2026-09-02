#include "Map/RTHexLibrary.h"
// 🔑 `RT_OccupancySectorCount` per il CONTEGGIO dei settori, e il nome e' ereditato dal primo
// consumatore invece che dalla semantica: quel `12` e' del PARTIZIONAMENTO, non dell'occupancy.
// Dichiararne un secondo qui sarebbe la seconda copia della stessa convenzione, che
// `spec-pointer-interaction.md` §4.9 vieta e che `#712` ha gia' pagato una volta.
#include "Map/RTHexOccupancyLibrary.h"
#include "Map/RTMapVisuals.h"   // RTCellPrismRadius: la mesh che CellVolumeTransform scala nasce con quel raggio

namespace
{
	// sqrt(3) in doppia precisione (matematica pointy-top). Usata solo nelle conversioni verso/da il mondo.
	constexpr double RT_SQRT3 = 1.7320508075688772;

	// Vettori assiali (dq,dr) delle sei direzioni pointy-top, ordine stabile E,NE,NW,W,SW,SE.
	constexpr int32 RT_HEX_DX[6] = { +1, +1,  0, -1, -1,  0 };
	constexpr int32 RT_HEX_DY[6] = {  0, -1, -1,  0, +1, +1 };

	// Arrotondamento cubico: dal cubo frazionario (q,r,s) alla cella intera piu' vicina, mantenendo q+r+s=0.
	FRTCellId CubeRound(double Fq, double Fr, int32 Layer)
	{
		const double Fs = -Fq - Fr;
		int32 Rq = FMath::RoundToInt(Fq);
		int32 Rr = FMath::RoundToInt(Fr);
		int32 Rs = FMath::RoundToInt(Fs);

		const double Dq = FMath::Abs(Rq - Fq);
		const double Dr = FMath::Abs(Rr - Fr);
		const double Ds = FMath::Abs(Rs - Fs);

		if (Dq > Dr && Dq > Ds)
		{
			Rq = -Rr - Rs;
		}
		else if (Dr > Ds)
		{
			Rr = -Rq - Rs;
		}
		// altrimenti Rs si aggiusta implicitamente (non serve, X e Y bastano)
		return FRTCellId(Rq, Rr, Layer);
	}
}

FIntPoint URTHexLibrary::AxialDirection(ERTHexDirection Dir)
{
	const int32 I = static_cast<int32>(Dir);
	return FIntPoint(RT_HEX_DX[I], RT_HEX_DY[I]);
}

FRTCellId URTHexLibrary::Neighbor(const FRTCellId& Cell, ERTHexDirection Dir)
{
	const FIntPoint D = AxialDirection(Dir);
	return FRTCellId(Cell.X + D.X, Cell.Y + D.Y, Cell.Layer);
}

TArray<FRTCellId> URTHexLibrary::Neighbors(const FRTCellId& Cell)
{
	TArray<FRTCellId> Out;
	Out.Reserve(6);
	for (int32 I = 0; I < 6; ++I)
	{
		Out.Add(FRTCellId(Cell.X + RT_HEX_DX[I], Cell.Y + RT_HEX_DY[I], Cell.Layer));
	}
	return Out;
}

int32 URTHexLibrary::HexDistance(const FRTCellId& A, const FRTCellId& B)
{
	// Distanza cubica: (|dq| + |dr| + |ds|) / 2.
	const int32 Dq = FMath::Abs(A.X - B.X);
	const int32 Dr = FMath::Abs(A.Y - B.Y);
	const int32 Ds = FMath::Abs(A.CubeZ() - B.CubeZ());
	return (Dq + Dr + Ds) / 2;
}

bool URTHexLibrary::DirectionBetween(const FRTCellId& From, const FRTCellId& To, ERTHexDirection& OutDirection)
{
	if (From.Layer != To.Layer)
	{
		return false;
	}

	const int32 Dq = To.X - From.X;
	const int32 Dr = To.Y - From.Y;
	for (int32 I = 0; I < 6; ++I)
	{
		if (RT_HEX_DX[I] == Dq && RT_HEX_DY[I] == Dr)
		{
			OutDirection = static_cast<ERTHexDirection>(I);
			return true;
		}
	}
	return false;
}

bool URTHexLibrary::DirectionTowards(const FRTCellId& From, const FRTCellId& To, ERTHexDirection& OutDirection)
{
	if (HexDistance(From, To) <= 0)
	{
		return false;
	}

	// Il primo passo della linea e' per costruzione adiacente a From (HexLine produce celle consecutive
	// adiacenti), quindi DirectionBetween lo riconosce sempre: una sola definizione di "verso dove".
	const TArray<FRTCellId> Line = HexLine(From, To);
	if (Line.Num() < 2)
	{
		return false;
	}
	return DirectionBetween(From, Line[1], OutDirection);
}

FVector URTHexLibrary::AxialToWorld(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight)
{
	const double Size = static_cast<double>(HexSize);
	const double Q = static_cast<double>(Cell.X);
	const double R = static_cast<double>(Cell.Y);
	const double Wx = Size * RT_SQRT3 * (Q + R / 2.0);
	const double Wy = Size * 1.5 * R;
	return FVector(
		Origin.X + Wx,
		Origin.Y + Wy,
		Origin.Z + static_cast<double>(Cell.Layer) * static_cast<double>(LayerHeight));
}

TArray<FVector> URTHexLibrary::CellCorners(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight)
{
	// Nessuna trigonometria qui: la convenzione pointy-top (primo vertice a -30 gradi) vive gia' in
	// HexCorners, ed e' la stessa funzione da cui nasce il prisma di ARTHexMapActor::GetCellPrismMesh.
	// Quel commento avverte di non scriverne una seconda copia, perche' due disegni della stessa forma
	// che partono da due formule tornano a divergere: e' il difetto visto a schermo nella seduta U22,
	// celle piene tonde e contorno esagonale. Questa funzione COMPONE soltanto — il centro dalla cella
	// logica, i vertici da HexCorners — e aggiunge il solo pezzo che mancava: l'esposizione a Blueprint.
	return HexCorners(AxialToWorld(Cell, Origin, HexSize, LayerHeight), HexSize);
}

FTransform URTHexLibrary::CellVolumeTransform(const FRTCellId& Cell, const FVector& Origin, float HexSize, float LayerHeight, float PlanarFraction)
{
	// La mesh nasce con circumraggio RTCellPrismRadius e mezza-altezza RTCellPrismRadius, centrata: la
	// scala e' quindi il rapporto fra la misura voluta e quel raggio, su ciascun asse col proprio
	// denominatore. Sull'altezza il fattore e' meta' di LayerHeight perche' la mezza-altezza della mesh
	// e' gia' meta' del suo ingombro — dimenticarlo darebbe un volume alto il doppio, che tassella con
	// il layer sbagliato e sembra corretto guardandolo da solo.
	const double Radius   = static_cast<double>(RTCellPrismRadius);
	const double Planar   = static_cast<double>(HexSize) * static_cast<double>(PlanarFraction) / Radius;
	const double Vertical = (static_cast<double>(LayerHeight) * 0.5) / Radius;

	return FTransform(
		FQuat::Identity,
		AxialToWorld(Cell, Origin, HexSize, LayerHeight),
		FVector(Planar, Planar, Vertical));
}

FRTCellId URTHexLibrary::WorldToAxial(const FVector& World, const FVector& Origin, float HexSize, int32 Layer)
{
	const double Size = static_cast<double>(HexSize);
	const double Lx = World.X - Origin.X;
	const double Ly = World.Y - Origin.Y;
	const double Fq = (RT_SQRT3 / 3.0 * Lx - 1.0 / 3.0 * Ly) / Size;
	const double Fr = (2.0 / 3.0 * Ly) / Size;
	return CubeRound(Fq, Fr, Layer);
}

int32 URTHexLibrary::WorldToLayer(double WorldZ, double OriginZ, float LayerHeight)
{
	if (LayerHeight <= 0.f)
	{
		return 0;
	}
	return FMath::RoundToInt((WorldZ - OriginZ) / static_cast<double>(LayerHeight));
}

FRTCellId URTHexLibrary::WorldToCellId(const FVector& World, const FVector& Origin, float HexSize, float LayerHeight)
{
	const int32 Layer = WorldToLayer(World.Z, Origin.Z, LayerHeight);
	return WorldToAxial(World, Origin, HexSize, Layer);
}

FRTCellId URTHexLibrary::ResolveRayToCellOnLayer(const FVector& RayOrigin, const FVector& RayDirection,
	const FVector& Origin, float HexSize, float LayerHeight, int32 ActiveLayer,
	bool bHasValidHit, const FVector& HitPoint)
{
	if (bHasValidHit)
	{
		return WorldToAxial(HitPoint, Origin, HexSize, ActiveLayer);
	}

	// Nessun colpo utile: si punta il PIANO del layer attivo. La quota e' quella del layer, non quella del
	// raggio, perche' la domanda e' «quale cella di questo piano sto indicando» e non «cosa ho colpito».
	const double PlaneZ = Origin.Z + static_cast<double>(ActiveLayer) * static_cast<double>(LayerHeight);

	// 🔴 **Il raggio deve andare VERSO il piano, e prima non lo si verificava** (code review, 2026-08-30).
	// `FMath::RayPlaneIntersection` e' `Origin + Dir * t` senza guardia su `t >= 0` ne' sul denominatore:
	// con il cursore **sopra l'orizzonte** — raggiungibile a ogni pitch piu' radente della meta' del FOV
	// verticale, e `MaxPitch` e' `0` — `t` diventa negativo e la cella risolta sta *dietro* la camera;
	// con il raggio parallelo al piano il quoziente e' infinito e `RoundToInt` produce una cella spazzatura.
	//
	// ⚠️ **Non era raggiungibile prima di `D-255`**: il vecchio percorso richiedeva un colpo del raycast,
	// quindi «nessun colpo» significava «nessuna cella». Da quando il ripiego sul piano e' la via normale,
	// questo caso e' in partita.
	//
	// Il ripiego del ripiego e' la **cella sotto la camera**: e' il punto piu' vicino a cio' che si sta
	// guardando fra quelli che esistono, e non e' una posizione arbitraria.
	const double Denom = RayDirection.Z;
	const double Numer = PlaneZ - RayOrigin.Z;
	if (FMath::Abs(Denom) < UE_DOUBLE_SMALL_NUMBER || (Numer / Denom) < 0.0)
	{
		return WorldToAxial(FVector(RayOrigin.X, RayOrigin.Y, PlaneZ), Origin, HexSize, ActiveLayer);
	}

	const FPlane LayerPlane(FVector(0, 0, PlaneZ), FVector(0, 0, 1));
	const FVector Point = FMath::RayPlaneIntersection(RayOrigin, RayDirection, LayerPlane);
	return WorldToAxial(Point, Origin, HexSize, ActiveLayer);
}

FVector URTHexLibrary::CellsCentroidWorld(const TArray<FRTCellId>& Cells, const FVector& Origin, float HexSize,
	float LayerHeight)
{
	if (Cells.Num() == 0)
	{
		return Origin; // nessuna cella: si ripiega sull'origine, non su un punto arbitrario
	}

	FVector Sum = FVector::ZeroVector;
	for (const FRTCellId& Cell : Cells)
	{
		Sum += AxialToWorld(Cell, Origin, HexSize, LayerHeight);
	}
	// Somma di centri: commutativa, quindi l'ordine dell'input non cambia il risultato.
	return Sum / static_cast<double>(Cells.Num());
}

FBox URTHexLibrary::CellsBoundsWorld(const TArray<FRTCellId>& Cells, const FVector& Origin, float HexSize,
	float LayerHeight)
{
	FBox Box(ForceInit); // non valido finche' non entra una cella: una mappa vuota non ha un'inquadratura

	// Semi-estensione dell'esagono, non del suo centro. I sei vertici di un pointy-top di circumraggio
	// `HexSize` stanno a -30 + 60k gradi (`HexCorners`), quindi |cos| massimo e' sqrt(3)/2 e |sin| e' 1:
	// la cella e' piu' STRETTA in X che in Y, e le due semi-estensioni non sono intercambiabili.
	const double HalfX = static_cast<double>(HexSize) * RT_SQRT3 * 0.5;
	const double HalfY = static_cast<double>(HexSize);

	for (const FRTCellId& Cell : Cells)
	{
		const FVector Centre = AxialToWorld(Cell, Origin, HexSize, LayerHeight);
		// `+=` su un FBox e' min/max componente per componente: piega commutativa, quindi l'ordine
		// dell'input non cambia il risultato.
		Box += FVector(Centre.X - HalfX, Centre.Y - HalfY, Centre.Z);
		Box += FVector(Centre.X + HalfX, Centre.Y + HalfY, Centre.Z);
	}
	return Box;
}

FBox URTHexLibrary::CellsBoundsWorld(const TArray<FRTHexCellData>& Cells, const FVector& Origin, float HexSize,
	float LayerHeight)
{
	FBox Box(ForceInit);

	const double HalfX = static_cast<double>(HexSize) * RT_SQRT3 * 0.5;
	const double HalfY = static_cast<double>(HexSize);

	for (const FRTHexCellData& Cell : Cells)
	{
		const FVector Centre = AxialToWorld(Cell.Id, Origin, HexSize, LayerHeight);
		// La quota d'autore alza la cella nel RENDER (`ARTHexMapActor::RebuildInstances`): il box deve
		// contenere dove la cella si vede, non dove il suo layer sarebbe.
		const double Z = Centre.Z + static_cast<double>(Cell.Height);
		Box += FVector(Centre.X - HalfX, Centre.Y - HalfY, Z);
		Box += FVector(Centre.X + HalfX, Centre.Y + HalfY, Z);
	}
	return Box;
}

int32 URTHexLibrary::SurfaceRingCount(ERTHexSurface Surface)
{
	// I quattro segni di `D-183`, per numero di anelli concentrici. L'area cresce col conteggio — 9,7% /
	// 18,4% / 26,2% / 32,9% della faccia — quindi il canale si legge a picco come contrasto d'AREA, che e'
	// la sola cosa che la seduta U18 ha misurato leggibile dall'alto (`PIE-HEX-VIZ-COSTO` ✅ contro
	// `PIE-HEX-VIZ-BLOCCHI` ❌, dove «la differenza e' una proporzione che la vista di lavoro azzera»).
	//
	// ⚠️ `switch` esplicito e senza `default`: quando qualcuno aggiungera' una decima superficie, il
	// compilatore chiedera' **che segno ha** invece di darle zero in silenzio. Un `default` qui renderebbe
	// ogni superficie nuova mono-canale per omissione — ed e' il difetto che il gate esiste per impedire.
	switch (Surface)
	{
	case ERTHexSurface::Smoke:       return 1;
	case ERTHexSurface::HighGround:  return 2;
	case ERTHexSurface::Conductive:  return 3;
	case ERTHexSurface::Ice:         return 4;

	// Mono-canale per SCELTA, non per omissione (criterio 1 di #956): i quattro segni chiudono le
	// collisioni misurate che le riguardano.
	case ERTHexSurface::Floor:
	case ERTHexSurface::ShallowWater:
	case ERTHexSurface::Rough:
	case ERTHexSurface::Fire:
	case ERTHexSurface::Void:
		return 0;
	}

	return 0;
}

FColor URTHexLibrary::SurfaceColor(ERTHexSurface Surface)
{
	// Tinte scelte per essere distinguibili fra loro e dal rosso del blocco (test:
	// Hex.SurfaceColorsAreDistinguishable). Cambiandone una, quel test dice subito se si e' persa la leggibilita'.
	switch (Surface)
	{
	// ⚠️ **G alzato da 120 a 145 il 2026-08-23** (#956), e il numero e' derivato, non scelto a occhio.
	// A `(60,120,255)` la luminanza Rec.601 valeva **117,5** contro i **107,4** di `Rough`: 10,1 di distanza
	// su una soglia di 20, cioe' due superfici comuni indistinguibili in una board vista in scala di grigi —
	// e nessuna delle due riceve un glifo, quindi il canale forma non le separa.
	//
	// La coppia non compariva fra le esenzioni della issue: la spec dichiarava «5 collisioni su 7 chiuse» e
	// ne nominava UNA sola come residua. L'ha trovata il gate, non una rilettura.
	//
	// Le cinque superfici senza glifo occupano `Void 85 · Rough 107 · ShallowWater 117 · Fire 157 · Floor 160`.
	// Con `Floor~Fire` esente servono quattro slot a distanza 20: **60 punti su ~75 disponibili**. `+25` su G
	// porta la luminanza a **132**, a 24,6 da `Rough` e 25,1 da `Fire` — l'unico punto che non stringe
	// nessun'altra coppia. Abbassare `Rough` invece l'avrebbe schiacciata su `Void`, che dista gia' solo 22.
	//
	// Il criterio 6 di #956 autorizza il ritocco: «i colori restano placeholder sostituibili, il vincolo e' la
	// ridondanza, non la tavolozza».
	case ERTHexSurface::ShallowWater: return FColor(60, 145, 255);
	case ERTHexSurface::Rough:        return FColor(140, 100, 60);
	case ERTHexSurface::Fire:         return FColor(255, 130, 40);
	case ERTHexSurface::Conductive:   return FColor(80, 230, 230);
	case ERTHexSurface::Ice:          return FColor(160, 220, 255);
	case ERTHexSurface::Void:         return FColor(150, 40, 150);
	// Grigio-bluastro slavato, distinto dal grigio neutro del Floor: il Fumo ha una regola VERA (cap di
	// targeting a 2 celle) e chi dipinge la mappa deve vedere dove l'ha messo.
	case ERTHexSurface::Smoke:        return FColor(180, 190, 215);
	case ERTHexSurface::HighGround:   return FColor(230, 200, 80);
	case ERTHexSurface::Floor:
	default:                          return FColor(160, 160, 160);
	}
}

FVector URTHexLibrary::EdgeMidpointWorld(const FRTCellId& Cell, ERTHexDirection Dir, const FVector& Origin,
	float HexSize, float LayerHeight)
{
	// Punto medio fra i due centri: derivato dalla convenzione delle direzioni, non da un angolo inciso.
	const FVector Here = AxialToWorld(Cell, Origin, HexSize, LayerHeight);
	const FVector There = AxialToWorld(Neighbor(Cell, Dir), Origin, HexSize, LayerHeight);
	return (Here + There) * 0.5;
}

FRotator URTHexLibrary::EdgeRotation(const FRTCellId& Cell, ERTHexDirection Dir)
{
	// Lo yaw si ricava dallo spostamento fra i due centri in coordinate-mondo. La scala non conta (e' una
	// direzione), quindi si usano origine e dimensioni neutre: cio' che importa e' l'ANGOLO.
	const FVector Here = AxialToWorld(Cell, FVector::ZeroVector, 100.f, 0.f);
	const FVector There = AxialToWorld(Neighbor(Cell, Dir), FVector::ZeroVector, 100.f, 0.f);
	return (There - Here).Rotation();
}

FRotator URTHexLibrary::FacingRotation(ERTHexDirection Facing)
{
	// Delega pura: la convenzione dei sei lati resta scritta in UN posto solo. La cella non entra nella
	// risposta perche' `AxialToWorld` e' affine in (q,r) — misurato da
	// `RefactorTactics.Hex.FacingRotationIsCellIndependent`, non assunto qui.
	return EdgeRotation(FRTCellId(0, 0, 0), Facing);
}

FVector URTHexLibrary::FacingMarkerOrigin(ERTHexDirection Facing, const FVector& UnitCenter,
	float BodyRadius, float FaceHeight)
{
	// `Vector()` di una rotazione di solo yaw e' unitario e planare: tutta la quota viene da `FaceHeight`,
	// e le sei origini restano complanari. Sommare `Up` a parte — invece di ruotare un unico offset
	// (BodyRadius, 0, FaceHeight) — e' cio' che tiene la verticale ancorata al MONDO.
	const FVector Forward = FacingRotation(Facing).Vector();
	return UnitCenter
		+ Forward * static_cast<double>(BodyRadius)
		+ FVector::UpVector * static_cast<double>(FaceHeight);
}

ERTHexDirection URTHexLibrary::OppositeDirection(ERTHexDirection Dir)
{
	// Le sei direzioni sono in ordine stabile 0..5 e diametralmente opposte a tre di distanza: E(0)<->W(3),
	// NE(1)<->SW(4), NW(2)<->SE(5). Derivato dall'ordine, non da una tabella da tenere allineata.
	return static_cast<ERTHexDirection>((static_cast<int32>(Dir) + 3) % 6);
}

float URTHexLibrary::ReliefHeightForCost(int32 MoveCost)
{
	// Il pavimento (costo 1) sta a zero: il rilievo misura il sovrapprezzo, non il costo assoluto. Costi
	// minori di 1 non esistono a catalogo, ma un asset editato a mano puo' contenerli: si trattano come piani
	// invece di produrre una buca, che direbbe «qui si va piu' veloci» — cosa che il gioco non prevede.
	return FMath::Max(0, MoveCost - 1) * ReliefUnitHeight;
}

FColor URTHexLibrary::BlockedCellColor()
{
	return FColor(230, 40, 40); // rosso: non ci si passa
}

FColor URTHexLibrary::SightBlockerColor()
{
	return FColor(250, 240, 90); // giallo: non ci si vede attraverso, ma ci si passa
}

FColor URTHexLibrary::SpawnTeam0Color()
{
	return FColor(60, 170, 255); // azzurro: di qui parte la squadra 0
}

FColor URTHexLibrary::SpawnTeam1Color()
{
	return FColor(255, 130, 40); // arancio: di qui parte la squadra 1
}

FColor URTHexLibrary::UnreachableCellColor()
{
	return FColor(255, 0, 255); // magenta: il colore che nella mappa non usa nessuno, perche' e' un allarme
}

TArray<FVector> URTHexLibrary::HexCorners(const FVector& Center, float Radius)
{
	TArray<FVector> Corners;
	Corners.Reserve(6);
	for (int32 I = 0; I < 6; ++I)
	{
		const double Angle = PI / 180.0 * (60.0 * I - 30.0); // pointy-top: primo vertice a -30 gradi
		Corners.Add(Center + FVector(Radius * FMath::Cos(Angle), Radius * FMath::Sin(Angle), 0.0));
	}
	return Corners;
}

ERTHexDirection URTHexLibrary::DirectionForEdgeIndex(int32 EdgeIndex)
{
	// Rispecchiamento attorno all'asse X: il bordo geometrico `k` ha il punto medio a `60k` gradi, mentre
	// `AxialDirection(j)` punta a `-60j`. `E` (0) e `W` (3) restano fermi perche' sono i due punti fissi.
	// Il modulo positivo regge anche un indice fuori intervallo, che altrimenti diventerebbe un cast a un
	// enum inesistente.
	const int32 Wrapped = ((EdgeIndex % 6) + 6) % 6;
	return static_cast<ERTHexDirection>((6 - Wrapped) % 6);
}

int32 URTHexLibrary::PointingSectorAt(const FVector2D& LocalPoint, float HexSize)
{
	// Fail-closed: senza scala non c'e' geometria, e la dead-zone non e' calcolabile. Rispondere `0` sarebbe
	// un settore plausibile e falso — il difetto che vale sempre la pena non introdurre.
	if (HexSize <= 0.f)
	{
		return INDEX_NONE;
	}

	// L'INRAGGIO, cioe' la distanza dal centro al punto medio di un lato. E' la stessa riga di
	// `SectorBoundaryPoints`, e non e' un caso: la dead-zone si misura dove la cella e' piu' stretta.
	const double InRadius = static_cast<double>(HexSize) * FMath::Cos(FMath::DegreesToRadians(30.0));
	const double DeadZone = InRadius * static_cast<double>(PointingDeadZoneFraction);

	const double X = static_cast<double>(LocalPoint.X);
	const double Y = static_cast<double>(LocalPoint.Y);
	const double DistanceSq = X * X + Y * Y;
	if (DistanceSq < DeadZone * DeadZone)
	{
		return INDEX_NONE; // troppo vicino al centro: non si punta niente
	}

	// 🔑 **La convenzione si EREDITA, e questa e' la riga che la eredita.** `SectorBoundaryPoints`
	// mette `P[k]` a `-30 + 30k` gradi, quindi il settore `k` occupa `[-30 + 30k, -30 + 30(k+1))`. Sommare
	// `30` prima di dividere per `30` e' esattamente l'inversa di quella formula — non una seconda
	// convenzione che le somiglia. `RefactorTactics.Hex.PointingSectorFollowsTheBoundaryPoints` lo ancora
	// alla funzione invece che a un letterale, perche' due copie della stessa formula sono il modo in cui
	// `#712` e' nato.
	const double Degrees = FMath::RadiansToDegrees(FMath::Atan2(Y, X));

	// `Atan2` rende `(-180, 180]`. Il `+ 360` porta tutto in positivo prima del modulo, cosi' il wrap a
	// `359,x -> 0` cade nello stesso ramo di ogni altro angolo invece che in un caso speciale.
	const double Shifted = Degrees + 30.0 + 360.0;
	const int32 Sector = static_cast<int32>(FMath::FloorToDouble(Shifted / 30.0)) % RT_OccupancySectorCount;
	return Sector;
}

int32 URTHexLibrary::EdgeIndexForDirection(ERTHexDirection Dir)
{
	// Stessa operazione: un rispecchiamento e' l'inverso di se' stesso. Scritta come chiamata invece che
	// ricopiata, cosi' se la convenzione cambia c'e' un posto solo da cambiare.
	return static_cast<int32>(DirectionForEdgeIndex(static_cast<int32>(Dir)));
}

void URTHexLibrary::SplitSegmentAcrossCells(const FVector2D& WorldStart, const FVector2D& WorldEnd,
	const FVector& Origin, float HexSize, int32 Layer, float MinLength, TArray<FRTCellSegment>& Out)
{
	Out.Reset();

	const FVector2D Delta = WorldEnd - WorldStart;
	const double Length = Delta.Size();
	if (Length <= UE_KINDA_SMALL_NUMBER || HexSize <= 0.f)
	{
		return;
	}

	// QUALI CELLE. Si campiona lungo il gesto e si raccolgono le celle distinte, in ordine di incontro.
	// ⚠️ Il passo e' un quarto del raggio, e non e' una cifra scelta a occhio: la corda piu' corta che una
	// retta puo' tagliare dentro un esagono e' comunque piu' lunga di questo passo, quindi il campionamento
	// non puo' saltare una cella che il gesto attraversa davvero. Le celle sfiorate a un angolo — dove la
	// corda tende a zero — vengono scartate piu' sotto da `MinLength`, che e' il posto giusto per farlo.
	const double Step = static_cast<double>(HexSize) * 0.25;
	const int32 Samples = FMath::Max(2, FMath::CeilToInt(Length / Step) + 1);

	TArray<FRTCellId> Visited;
	for (int32 Index = 0; Index < Samples; ++Index)
	{
		const double T = static_cast<double>(Index) / static_cast<double>(Samples - 1);
		const FVector2D Point = WorldStart + Delta * T;
		const FRTCellId Cell = WorldToAxial(FVector(Point.X, Point.Y, 0.0), Origin, HexSize, Layer);
		if (Visited.Num() == 0 || !(Visited.Last() == Cell))
		{
			Visited.AddUnique(Cell);
		}
	}

	// RITAGLIO. L'esagono e' convesso, quindi il taglio e' l'intersezione di sei semipiani: si restringe
	// l'intervallo `[T0, T1]` un lato per volta. Nessun caso speciale per gli angoli, e nessuna divisione
	// quando il gesto e' parallelo a un lato.
	for (const FRTCellId& Cell : Visited)
	{
		const FVector Centre = AxialToWorld(Cell, Origin, HexSize, 0.f);
		const FVector2D Local(Centre.X, Centre.Y);
		const TArray<FVector> Corners = HexCorners(FVector(Local.X, Local.Y, 0.0), HexSize);
		if (Corners.Num() != 6)
		{
			continue;
		}

		double T0 = 0.0;
		double T1 = 1.0;
		bool bOutside = false;

		for (int32 Edge = 0; Edge < 6 && !bOutside; ++Edge)
		{
			const FVector2D V0(Corners[Edge].X, Corners[Edge].Y);
			const FVector2D V1(Corners[(Edge + 1) % 6].X, Corners[(Edge + 1) % 6].Y);

			// Normale uscente del lato. `HexCorners` enumera i vertici in ordine di angolo crescente, quindi
			// «dentro» sta sempre dallo stesso lato: la costanza e' quello che serve, non il verso.
			const FVector2D EdgeDir = V1 - V0;
			const FVector2D Normal(EdgeDir.Y, -EdgeDir.X);

			const double NumeratorStart = FVector2D::DotProduct(Normal, WorldStart - V0);
			const double Denominator = FVector2D::DotProduct(Normal, Delta);

			if (FMath::Abs(Denominator) <= UE_KINDA_SMALL_NUMBER)
			{
				// Parallelo a questo lato: o e' tutto dentro il semipiano, o e' tutto fuori.
				bOutside = NumeratorStart > UE_KINDA_SMALL_NUMBER;
				continue;
			}

			const double T = -NumeratorStart / Denominator;
			if (Denominator > 0.0)
			{
				T1 = FMath::Min(T1, T); // il gesto ESCE da questo lato
			}
			else
			{
				T0 = FMath::Max(T0, T); // il gesto ENTRA da questo lato
			}
		}

		if (bOutside || T1 <= T0)
		{
			continue; // il campionamento aveva incluso una cella che il ritaglio esatto smentisce
		}

		const FVector2D ClippedStart = WorldStart + Delta * T0;
		const FVector2D ClippedEnd = WorldStart + Delta * T1;
		if (FVector2D::Distance(ClippedStart, ClippedEnd) < static_cast<double>(MinLength))
		{
			continue; // sfiorata a un angolo: non ci si disegna un muro
		}

		FRTCellSegment Piece;
		Piece.Cell = Cell;
		Piece.LocalStart = ClippedStart - Local;
		Piece.LocalEnd = ClippedEnd - Local;
		Out.Add(Piece);
	}
}

float URTHexLibrary::DistanceRayToSegment(const FVector& RayOrigin, const FVector& RayDir, const FVector& A, const FVector& B)
{
	// Closest points tra semi-retta (s>=0, dir unitaria) e segmento (t in [0,1]). Adattato da Ericson, con doppio clamp.
	const FVector D1 = RayDir.GetSafeNormal(); // direzione ray (unitaria) -> s = distanza lungo il ray
	const FVector D2 = B - A;                   // direzione segmento -> t in [0,1]
	const double E = FVector::DotProduct(D2, D2);
	const FVector R = RayOrigin - A;
	const double B1 = FVector::DotProduct(D1, D2);
	const double C = FVector::DotProduct(D1, R);
	const double F = FVector::DotProduct(D2, R);

	double S = 0.0; // lungo il ray (>=0)
	double T = 0.0; // lungo il segmento ([0,1])
	if (E <= (double)SMALL_NUMBER)
	{
		// Segmento degenere (A==B): T=0, S = proiezione di A sul ray (clamp>=0).
		T = 0.0;
		S = FMath::Max(0.0, -C);
	}
	else
	{
		const double Denom = E - B1 * B1; // = |D2|^2 - dot(D1,D2)^2 (>=0)
		T = (Denom > (double)SMALL_NUMBER) ? ((F - C * B1) / Denom) : (F / E);
		T = FMath::Clamp(T, 0.0, 1.0);
		S = FMath::Max(0.0, T * B1 - C);
		T = FMath::Clamp((F + S * B1) / E, 0.0, 1.0);
	}

	const FVector PRay = RayOrigin + static_cast<float>(S) * D1;
	const FVector QSeg = A + static_cast<float>(T) * D2;
	return static_cast<float>(FVector::Dist(PRay, QSeg)); // FVector::Dist ritorna double in UE5: cast esplicito (no C4244)
}

bool URTHexLibrary::StableLess(const FRTCellId& A, const FRTCellId& B)
{
	if (A.Layer != B.Layer) { return A.Layer < B.Layer; }
	if (A.X != B.X) { return A.X < B.X; }
	return A.Y < B.Y;
}

TArray<FRTCellId> URTHexLibrary::HexArea(const FRTCellId& Center, int32 Radius)
{
	TArray<FRTCellId> Out;
	if (Radius < 0)
	{
		return Out;
	}
	Out.Reserve(3 * Radius * (Radius + 1) + 1);
	for (int32 Dq = -Radius; Dq <= Radius; ++Dq)
	{
		const int32 RLo = FMath::Max(-Radius, -Dq - Radius);
		const int32 RHi = FMath::Min(Radius, -Dq + Radius);
		for (int32 Dr = RLo; Dr <= RHi; ++Dr)
		{
			Out.Add(FRTCellId(Center.X + Dq, Center.Y + Dr, Center.Layer));
		}
	}
	return Out;
}

namespace
{
	// Scala del "nudge" intero che rompe i pareggi dell'arrotondamento: il punto interpolato viene spostato di
	// 1/1024 di cella in una direzione FISSA (bias 1,1,-2 -> somma nulla, l'invariante cubica regge). Sostituisce
	// l'epsilon float della formulazione classica: una linea che passa esattamente sul confine fra due celle
	// sceglie sempre lo stesso lato, senza oscillare. Calcoli in int64: nessun overflow con mappe grandi.
	constexpr int64 RT_HEX_LINE_SCALE = 1024;

	/** Divisione intera con arrotondamento al piu' vicino (pareggi lontano dallo zero). Den > 0. */
	int64 RoundDivInt(int64 Num, int64 Den)
	{
		return (Num >= 0) ? (2 * Num + Den) / (2 * Den) : -((-2 * Num + Den) / (2 * Den));
	}
}

TArray<FRTCellId> URTHexLibrary::HexLine(const FRTCellId& A, const FRTCellId& B)
{
	TArray<FRTCellId> Out;

	const int32 N = HexDistance(A, B);
	Out.Reserve(N + 1);
	if (N == 0)
	{
		Out.Add(FRTCellId(A.X, A.Y, A.Layer));
		return Out;
	}

	// Estremi in coordinate cubiche (q, r, s = -q-r).
	const int64 Aq = A.X, Ar = A.Y, As = A.CubeZ();
	const int64 Bq = B.X, Br = B.Y, Bs = B.CubeZ();
	const int64 Den = RT_HEX_LINE_SCALE * N;

	for (int32 i = 0; i <= N; ++i)
	{
		// Punto i/N sulla retta come frazione INTERA (numeratori scalati + bias di rottura dei pareggi).
		const int64 Nq = RT_HEX_LINE_SCALE * (Aq * (N - i) + Bq * i) + 1;
		const int64 Nr = RT_HEX_LINE_SCALE * (Ar * (N - i) + Br * i) + 1;
		const int64 Ns = RT_HEX_LINE_SCALE * (As * (N - i) + Bs * i) - 2;

		int64 Rq = RoundDivInt(Nq, Den);
		int64 Rr = RoundDivInt(Nr, Den);
		int64 Rs = RoundDivInt(Ns, Den);

		// Resti interi (scala Den): la componente col resto maggiore si ricava dalle altre due, cosi' q+r+s = 0
		// resta vero. Ordine di preferenza fisso q -> r -> s, come CubeRound.
		const int64 Dq = FMath::Abs(Rq * Den - Nq);
		const int64 Dr = FMath::Abs(Rr * Den - Nr);
		const int64 Ds = FMath::Abs(Rs * Den - Ns);

		if (Dq > Dr && Dq > Ds)
		{
			Rq = -Rr - Rs;
		}
		else if (Dr > Ds)
		{
			Rr = -Rq - Rs;
		}
		// altrimenti si aggiusta s, che non serve all'output (q e r bastano)

		Out.Add(FRTCellId(static_cast<int32>(Rq), static_cast<int32>(Rr), A.Layer));
	}
	return Out;
}

TArray<FRTCellId> URTHexLibrary::HexCone(const FRTCellId& From, const FRTCellId& Target, int32 Range)
{
	TArray<FRTCellId> Out;
	if (Range <= 0 || (From.X == Target.X && From.Y == Target.Y))
	{
		return Out;
	}

	// Direzione principale = primo passo della linea verso il bersaglio (sempre uno dei sei vicini).
	const TArray<FRTCellId> Line = HexLine(From, Target);
	if (Line.Num() < 2)
	{
		return Out;
	}
	const int32 StepX = Line[1].X - From.X;
	const int32 StepY = Line[1].Y - From.Y;

	int32 DirIdx = INDEX_NONE;
	for (int32 I = 0; I < 6; ++I)
	{
		if (RT_HEX_DX[I] == StepX && RT_HEX_DY[I] == StepY)
		{
			DirIdx = I;
			break;
		}
	}
	if (DirIdx == INDEX_NONE)
	{
		return Out;
	}

	// Ventaglio di 120 gradi = due settori a 60 gradi adiacenti. Nel settore fra due direzioni contigue ogni
	// cella e' a*D1 + b*D2 con a,b >= 0 e distanza esattamente a+b: il taglio a+b <= Range da' la profondita'.
	TSet<FRTCellId> Unique;
	auto AddSector = [&Unique, &From, Range](int32 D1, int32 D2)
	{
		for (int32 A = 0; A <= Range; ++A)
		{
			for (int32 B = 0; A + B <= Range; ++B)
			{
				if (A + B < 1)
				{
					continue; // l'origine non fa parte del cono
				}
				Unique.Add(FRTCellId(
					From.X + A * RT_HEX_DX[D1] + B * RT_HEX_DX[D2],
					From.Y + A * RT_HEX_DY[D1] + B * RT_HEX_DY[D2],
					From.Layer));
			}
		}
	};
	AddSector((DirIdx + 5) % 6, DirIdx); // settore sinistro
	AddSector(DirIdx, (DirIdx + 1) % 6); // settore destro

	// TSet non ha ordine garantito: si ordina in modo stabile (determinismo).
	Out = Unique.Array();
	Out.Sort([](const FRTCellId& L, const FRTCellId& R) { return StableLess(L, R); });
	return Out;
}

ERTHexDirection URTHexLibrary::NearestEdgeDirection(const FRTCellId& Cell, const FVector& WorldPoint,
	const FVector& Origin, float HexSize, float LayerHeight)
{
	// I sei lati nell'ordine canonico di `DirectionForEdgeIndex`: iterare l'enum a mano qui significherebbe
	// scrivere una seconda volta un elenco che esiste gia'.
	ERTHexDirection Best = DirectionForEdgeIndex(0);
	double BestDistanceSquared = TNumericLimits<double>::Max();

	for (int32 EdgeIndex = 0; EdgeIndex < 6; ++EdgeIndex)
	{
		const ERTHexDirection Dir = DirectionForEdgeIndex(EdgeIndex);
		const FVector Mid = EdgeMidpointWorld(Cell, Dir, Origin, HexSize, LayerHeight);

		// IN PIANTA: la quota del punto cliccato dipende dalla camera e dal terreno, non da quale lato
		// l'autore stia mirando.
		const double DX = WorldPoint.X - Mid.X;
		const double DY = WorldPoint.Y - Mid.Y;
		const double DistanceSquared = DX * DX + DY * DY;

		// Confronto STRETTO: a parita' di distanza vince l'indice piu' basso, quindi il centro della cella —
		// dove tutti e sei i lati sono equidistanti — da' sempre la stessa risposta invece di dipendere
		// dall'ordine di visita o da un epsilon.
		if (DistanceSquared < BestDistanceSquared)
		{
			BestDistanceSquared = DistanceSquared;
			Best = Dir;
		}
	}

	return Best;
}
