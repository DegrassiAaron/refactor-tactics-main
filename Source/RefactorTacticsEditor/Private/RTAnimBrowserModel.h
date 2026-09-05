#pragma once

#include "CoreMinimal.h"
#include "Unit/RTAnimCatalogTypes.h"

/** Una riga del browser: cio' che la lista mostra, gia' filtrato. */
struct FRTAnimBrowserRow
{
	FName                Id;
	FString              AssetName;
	FString              AssetPath;
	FString              Pack;        // `Gadget`, `Wraith`, … dedotto dal path, non dal nome della clip
	ERTAnimClipStatus    Status = ERTAnimClipStatus::Unreviewed;
	FString              Label;
	float                DurationSeconds = 0.f;
	bool                 bHasRootMotion = false;
	bool                 bIsAdditive = false;
	int32                NumBindings = 0;
};

/**
 * Il modello del browser delle animazioni: filtri, righe, e i comandi che scrivono il giudizio umano.
 *
 * 🔑 **Esiste separato dal widget per una ragione sola: cosi' resta qualcosa da misurare.** Slate su un
 * editor vivo non lo vede nessun automation test — `RTDevSandboxLauncherTests.cpp` lo dichiara in testa
 * per il proprio pannello, e vale identico qui. Se filtri e comandi vivessero dentro `SRTAnimBrowserPanel`,
 * l'intera issue sarebbe a carico dell'occhio umano.
 *
 * ⛔ **Non conosce Slate.** Nessun `SWidget`, nessun `FSlateBrush`: il pannello lo interroga, non il
 * contrario.
 */
class FRTAnimBrowserModel
{
public:
	/** Carica il catalogo. `false` con `OutError` scritto: un catalogo illeggibile non e' un catalogo vuoto. */
	bool LoadFrom(const FString& CatalogPath, FString& OutError);

	/** Scrive il catalogo cosi' com'e' in memoria. */
	bool SaveTo(const FString& CatalogPath, FString& OutError) const;

	// ── Filtri ──────────────────────────────────────────────────────────────────────────────────────
	/** `NAME_None` = tutti i personaggi. */
	void SetPackFilter(const FString& Pack) { PackFilter = Pack; }
	/** Sottostringa, case-insensitive, su nome e `AV_ID`. Vuota = nessun filtro. */
	void SetSearchText(const FString& Text) { SearchText = Text; }
	/** Non impostato = tutti gli stati. */
	void SetStatusFilter(TOptional<ERTAnimClipStatus> Status) { StatusFilter = Status; }

	/** Le righe che passano **tutti** i filtri, in ordine di `AV_ID`. */
	TArray<FRTAnimBrowserRow> VisibleRows() const;

	/** Tutte le righe, ignorando i filtri. Serve ai test per non confondere «filtrato» con «assente». */
	int32 TotalRowCount() const { return Catalog.Entries.Num(); }

	// ── Comandi utente ──────────────────────────────────────────────────────────────────────────────
	/**
	 * 🔴 **L'UNICO punto in cui `Status` viene scritto, in tutto il progetto.**
	 *
	 * Non e' una promessa: e' la struttura. Il pannello chiama questa e nient'altro, e nessun percorso
	 * automatico — scan, bind, filtro, caricamento — la attraversa. `Promoted` significa che una persona
	 * ha guardato la clip, e l'unico modo di scriverlo e' che una persona prema il pulsante.
	 */
	bool ApplyUserStatus(const FName& Id, ERTAnimClipStatus NewStatus);

	/**
	 * Lega la clip a `(eroe, ruolo)`. **Entra sempre INATTIVA**, qualunque sia lo stato del ruolo.
	 *
	 * ⛔ Rifiuta se la clip non e' `Promoted`: legare cio' che nessuno ha guardato e' esattamente il
	 * salto che questo strumento esiste per impedire.
	 */
	bool BindToRole(const FName& Id, const FName& HeroId, ERTPresentationRole Role);

	/**
	 * Rende attiva questa clip per `(eroe, ruolo)`, **atomicamente**: qualunque altra attiva per lo
	 * stesso ruolo torna inattiva nello stesso passo.
	 */
	bool MakeActive(const FName& Id, const FName& HeroId, ERTPresentationRole Role);

	/**
	 * Toglie il legame. Se era l'attiva, il ruolo resta **senza attiva** e non si elegge una sostituta:
	 * la scelta e' dell'autore, e il ripiego e' la posa di riferimento.
	 */
	bool Unbind(const FName& Id, const FName& HeroId, ERTPresentationRole Role);

	/** Il pack di un path Paragon (`.../ParagonGadget/...` -> `Gadget`), o vuoto. */
	static FString PackFromAssetPath(const FString& AssetPath);

	const FRTAnimCatalog& GetCatalog() const { return Catalog; }

private:
	FRTAnimCatalogEntry* FindEntry(const FName& Id);
	const FRTAnimCatalogEntry* FindEntry(const FName& Id) const;

	FRTAnimCatalog Catalog;

	FString PackFilter;
	FString SearchText;
	TOptional<ERTAnimClipStatus> StatusFilter;
};
