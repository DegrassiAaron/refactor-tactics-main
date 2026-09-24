#include "Debug/RTContextInspector.h"

#include "Core/RTEnumName.h"
#include "Map/RTHexCellData.h"
#include "Replay/RTReplayPrivacyLibrary.h"
#include "Turn/RTHexSim.h"
#include "Turn/RTTurnLog.h"

#include "Styling/CoreStyle.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include "Widgets/Text/STextBlock.h"

FString URTContextInspectorLibrary::DescribePublicEvent(const FRTPublicReplayEntry& Entry)
{
	// Colonne, non prosa: vedi l'header per la ragione. Tutte e sole quelle che
	// `URTReplayPrivacyLibrary::ToPublicTrace` lascia passare.
	FString Line = FString::Printf(TEXT("T%d %s/%s"),
		Entry.TurnNumber, *RTReflection::EnumName(Entry.Phase), *RTReflection::EnumName(Entry.Category));

	if (!Entry.ActionId.IsNone())
	{
		Line += FString::Printf(TEXT(" %s"), *Entry.ActionId.ToString());
	}
	// `UnitId` a zero significa **riga di mondo**, non l'unita' zero: e' la sentinella del TurnLog (D-063),
	// e gli `StableUnitId` partono da 1. Stamparlo darebbe un soggetto che non esiste.
	if (Entry.UnitId != 0)
	{
		Line += FString::Printf(TEXT(" u%d"), Entry.UnitId);
	}
	Line += FString::Printf(TEXT(" %s->%s"), *Entry.SrcCell.ToString(), *Entry.TgtCell.ToString());
	if (Entry.Amount != 0)
	{
		Line += FString::Printf(TEXT(" amm=%d"), Entry.Amount);
	}
	return Line;
}

FRTContextInspectorView URTContextInspectorLibrary::Compose(int32 ObserverTeamId,
	const FRTHexCellData& Cell, const FRTHexSnapshot& Snapshot, const TArray<FRTPlannedIntent>& Intents,
	const TArray<FRTTurnLogEntry>& AuditTrace, ERTContextView Mode)
{
	FRTContextInspectorView View;
	View.Cell = Cell.Id;
	View.ObserverTeamId = ObserverTeamId;

	// La cella e gli intenti hanno gia' i loro owner, e `DescribeContext` li giustappone.
	const TArray<FString> Contesto =
		URTDebugReportLibrary::DescribeContext(ObserverTeamId, Cell, Snapshot, Intents, Mode);
	if (Contesto.Num() > 0)
	{
		View.CellLine = Contesto[0];
		for (int32 i = 1; i < Contesto.Num(); ++i)
		{
			// La riga tecnica, quando c'e', e' l'ULTIMA che `DescribeContext` accoda: si riconosce dal
			// prefisso, che e' un dato del suo formato e non una convenzione inventata qui.
			if (Contesto[i].StartsWith(TEXT("[tecnico]")))
			{
				View.TechnicalLines.Add(Contesto[i]);
			}
			else
			{
				View.IntentLines.Add(Contesto[i]);
			}
		}
	}
	View.bTechnical = View.TechnicalLines.Num() > 0;

	// 🔴 I DUE confini della traccia, composti — e in quest'ordine. Prima QUALI VOCI questo osservatore
	// era autorizzato a conoscere, poi QUALI COLONNE di quelle voci sono pubbliche. Invertirli non
	// cambierebbe il risultato, ma ometterne uno si': il primo da solo lascerebbe le colonne di audit su
	// voci proprie, il secondo da solo lascerebbe le voci altrui con le sole colonne pubbliche — che e'
	// comunque sapere che un fatto e' accaduto, e dove.
	const TArray<FRTTurnLogEntry> Visibili =
		URTReplayPrivacyLibrary::FilterEntriesForObserver(AuditTrace, ObserverTeamId);
	const TArray<FRTPublicReplayEntry> Pubbliche = URTReplayPrivacyLibrary::ToPublicTrace(Visibili);

	for (const FRTPublicReplayEntry& Entry : Pubbliche)
	{
		// Cio' che e' successo SU QUESTA CELLA: partenza o arrivo. Un evento che la nomina come bersaglio
		// appartiene alla cella quanto uno che ne parte.
		if (Entry.SrcCell == Cell.Id || Entry.TgtCell == Cell.Id)
		{
			View.EventLines.Add(DescribePublicEvent(Entry));
		}
	}

	return View;
}

FRTContextInspectorRequest URTContextInspectorLibrary::ParseCommandArgs(const TArray<FString>& Args)
{
	FRTContextInspectorRequest Richiesta;
	if (Args.Num() == 0)
	{
		return Richiesta; // mostra, squadra 0
	}

	// ⚠️ `Equals` con `IgnoreCase` e non un confronto secco: chi digita in console non ha motivo di
	// ricordarsi la cassa, e un `OFF` che non spegnesse sarebbe la stessa sorpresa da cui veniamo.
	if (Args[0].Equals(TEXT("off"), ESearchCase::IgnoreCase))
	{
		Richiesta.bOff = true;
		return Richiesta;
	}

	// Tutto il resto e' un TeamId, come in ogni altro `rt.Debug.*`: `Atoi` su cio' che non e' un numero
	// da `0`, che e' la squadra 0 — il default, non un caso speciale.
	Richiesta.ObserverTeamId = FCString::Atoi(*Args[0]);
	return Richiesta;
}

TArray<FString> URTContextInspectorLibrary::AllLines(const FRTContextInspectorView& View)
{
	TArray<FString> Out;
	if (!View.CellLine.IsEmpty()) { Out.Add(View.CellLine); }
	Out.Append(View.IntentLines);
	Out.Append(View.EventLines);
	Out.Append(View.TechnicalLines);
	return Out;
}

void URTContextInspectorWidgetBase::ShowFor(const FRTContextInspectorView& InView)
{
	View = InView;
}

bool URTContextInspectorWidgetBase::HasContent() const
{
	return URTContextInspectorLibrary::AllLines(View).Num() > 0;
}

TArray<FText> URTContextInspectorWidgetBase::GetLines() const
{
	TArray<FText> Out;
	for (const FString& Line : URTContextInspectorLibrary::AllLines(View))
	{
		Out.Add(FText::FromString(Line));
	}
	return Out;
}

FText URTContextInspectorWidgetBase::GetHeaderText() const
{
	// ⚠️ L'osservatore e' nell'intestazione, e non e' un dettaglio diagnostico: un pannello che non dicesse
	// per chi e' composto si leggerebbe come «la verita'», mentre e' una verita' parziale per costruzione.
	const FString Chi = View.ObserverTeamId == RTObserver::Omniscient
		? FString(TEXT("onnisciente"))
		: FString::Printf(TEXT("squadra %d"), View.ObserverTeamId);
	return FText::FromString(FString::Printf(TEXT("%s — %s"), *View.Cell.ToString(), *Chi));
}

FRTContextInspectorPlacement URTContextInspectorWidgetBase::Placement() const
{
	return FRTContextInspectorPlacement();
}

TSharedRef<SWidget> URTContextInspectorWidgetBase::RebuildWidget()
{
	const FRTContextInspectorPlacement Posa = Placement();

	TSharedRef<SVerticalBox> Righe = SNew(SVerticalBox);

	// 🔑 L'intestazione per PRIMA e sempre: dice QUALE cella e PER CHI. Un pannello che non lo dicesse
	// si leggerebbe come «la verita'», mentre e' una verita' parziale per costruzione.
	Righe->AddSlot()
		.AutoHeight()
		.Padding(0.f, 0.f, 0.f, 6.f)
		[
			SNew(STextBlock)
				.Text_Lambda([this]() { return GetHeaderText(); })
				// Il COLORE si dichiara, non si eredita: senza, il testo prende quello di default dello stile
				// e puo' risultare illeggibile sul proprio fondo (#3242, secondo difetto).
				.ColorAndOpacity(FSlateColor(FLinearColor(0.85f, 0.92f, 1.f)))
		];

	// ⚠️ Un numero FISSO di slot, riempiti da lambda. Slate si costruisce una volta e il contenuto cambia
	// a ogni frame: aggiungere uno slot per riga qui dentro congelerebbe il pannello alla PRIMA vista
	// mostrata, e le viste successive avrebbero piu' righe di quante ce ne sono.
	for (int32 i = 0; i < MaxRighe; ++i)
	{
		Righe->AddSlot()
			.AutoHeight()
			[
				SNew(STextBlock)
					.Text_Lambda([this, i]()
					{
						const TArray<FText> Linee = GetLines();
						return Linee.IsValidIndex(i) ? Linee[i] : FText::GetEmpty();
					})
					.ColorAndOpacity(FSlateColor(FLinearColor::White))
					.AutoWrapText(true)
			];
	}

	return SNew(SOverlay)
		+ SOverlay::Slot()
			.HAlign(Posa.Horizontal)
			.VAlign(Posa.Vertical)
			.Padding(FMargin(0.f, 0.f, Posa.Margin, Posa.Margin))
			[
				SNew(SBox)
					.MaxDesiredWidth(Posa.MaxWidth)
					[
						SNew(SBorder)
							.Padding(FMargin(12.f, 9.f))
							.BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush"))
							// Opaco all'85%, come l'overlay del verdetto: sotto ci puo' essere la board, e un
							// fondo troppo trasparente renderebbe illeggibili entrambi.
							.BorderBackgroundColor(FLinearColor(0.f, 0.f, 0.f, 0.85f))
							[
								Righe
							]
					]
			];
}