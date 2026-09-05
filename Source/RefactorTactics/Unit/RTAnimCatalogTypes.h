#pragma once

#include "CoreMinimal.h"
#include "RTAnimCatalogTypes.generated.h"

/**
 * Il giudizio umano su una clip. **Nessuna automazione lo scrive** oltre il default.
 *
 * ⛔ `Promoted` non e' raggiungibile da nessun percorso automatico del codice, e la garanzia e' strutturale:
 * l'unico ingresso automatico al catalogo e' `URTAnimCatalogLibrary::AllocateIds`, che scrive **solo**
 * `Unreviewed`. Promuovere significa editare l'authoring source, cioe' lasciare un diff con un autore.
 *
 * ⚠️ `Unreviewed` e' il valore **zero** apposta: una voce costruita di default e' «mai guardata» e non
 * «promossa». Se il default fosse `Promoted`, dimenticare di rivedere una clip la promuoverebbe da sola — che
 * e' il fallimento silenzioso che questo enum esiste per impedire. Stessa scelta di
 * `ERTPresentationKind::Cues` in `RTPresentationBinding.h`, e per la stessa ragione.
 *
 * Aggiungere valori solo IN CODA: il formato li serializza per NOME, ma un consumatore che li confronta per
 * valore numerico non deve cambiare significato sotto i piedi.
 */
UENUM(BlueprintType)
enum class ERTAnimClipStatus : uint8
{
	/** Mai guardata da una persona. Lo stato di ogni clip che lo scanner scopre. */
	Unreviewed,
	/** Guardata, plausibile, non ancora scelta. */
	Candidate,
	/** Scelta. ⛔ Solo per mano umana. */
	Promoted,
	/** Guardata e scartata. Vale quanto una promozione: e' lavoro che non va rifatto. */
	Rejected
};

/**
 * La meta' RIGENERABILE di una voce: tutto cio' che si puo' rileggere dall'asset.
 *
 * 🔑 E' una struct annidata e non un prefisso sui nomi dei campi. E' la differenza fra una convenzione e una
 * struttura: chi rinfresca il catalogo sostituisce questo oggetto **per intero** e non puo' toccare il
 * giudizio umano nemmeno per errore, mentre con campi affiancati basta una `for` distratta.
 *
 * `AssetPath` e' l'unico campo obbligatorio: e' l'identita' del referente. Gli altri restano vuoti finche'
 * lo scanner (#2446) non li riempie, e la loro assenza **non** e' un errore di validazione — un catalogo che
 * pretendesse la durata prima che qualcuno abbia aperto la clip sarebbe rosso il giorno in cui nasce.
 */
USTRUCT(BlueprintType)
struct FRTAnimClipDerived
{
	GENERATED_BODY()

	/**
	 * Percorso oggetto Unreal della clip (`/Game/FabAsset/.../Idle.Idle`).
	 *
	 * 🔴 **Punta a un asset che il repository NON vede**: `.gitignore:105` ignora `/Content/FabAsset/` (~48 GB).
	 * E' la ragione per cui il validator e' diviso in due meta': `ValidateCatalog` non tocca mai il disco,
	 * `ValidateReferents` lo tocca e dichiara `NOT RUN` quando i pack non ci sono.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FString AssetPath;

	/** Nome dell'asset, come sta sul disco. Rigenerabile: lo scanner lo riscrive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FString AssetName;

	/** Skeleton della clip. ⚠️ Si legge dalla MESH, non dal nome del file (guida Paragon, §AS.3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FString Skeleton;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	float DurationSeconds = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	int32 FrameCount = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	bool bHasRootMotion = false;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	bool bIsAdditive = false;

	/** Impronta dell'asset, per accorgersi che la clip e' cambiata sotto un giudizio gia' dato. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FString Fingerprint;
};

/**
 * La meta' AUTHORED di una voce: il giudizio umano, che nessuna automazione sovrascrive.
 *
 * ⛔ Nessuna funzione di questo modulo deduce una `Label` da un nome di file. `Heavy`, `Fast`, `Attack` sono
 * giudizi artistici: derivarli dalla stringa `Cast.uasset` produrrebbe un dato che sembra misurato e non lo
 * e'. La guida Paragon §AS.3b ha gia' pagato questo errore in senso inverso — sei caselle su venti portano un
 * nome diverso da quello atteso, e dedurre non ha mai funzionato per un pack intero.
 */
USTRUCT(BlueprintType)
struct FRTAnimClipAuthored
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	ERTAnimClipStatus Status = ERTAnimClipStatus::Unreviewed;

	/** Etichetta artistica scritta da una persona (`Heavy`, `Fast`, …). Vuota finche' nessuno la scrive. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FString Label;

	/** Perche' questa clip e' stata promossa o scartata. E' la memoria che oggi non esiste. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FString Notes;
};

/** Una voce del catalogo: un `AV_ID` stabile, cio' che si rilegge e cio' che una persona ha deciso. */
USTRUCT(BlueprintType)
struct FRTAnimCatalogEntry
{
	GENERATED_BODY()

	/**
	 * `AV_` + cifre decimali, zero-padding a quattro: `AV_0001`.
	 *
	 * ⚠️ Oltre `AV_9999` la larghezza **cresce** (`AV_10000`): non c'e' wraparound, e ogni confronto e'
	 * **numerico sul suffisso**, mai lessicale sulla stringa — `AV_10000` e' maggiore di `AV_9999`, cosa che
	 * un ordinamento lessicale nega.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FName Id;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FRTAnimClipDerived Derived;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FRTAnimClipAuthored Authored;
};

/**
 * Il catalogo delle animazioni: il dato che ricorda quale clip e' stata guardata, promossa o scartata.
 *
 * Esiste perche' oggi quel lavoro **non lascia traccia**. `ls .../Gadget/Animations` misura **85** voci per un
 * solo eroe; il triage delle clip si e' fatto una volta, a occhio, in prosa, e non ha prodotto un dato che
 * qualcuno possa rileggere.
 *
 * ⚠️ **Non e' un `UPrimaryDataAsset`, ed e' una scelta contro un precedente che va superato con evidenza.** Il
 * referto `animazioni-paragon-issue-orchestrator-spec-panel-2026-08-30.md` §8 ha respinto «Data Asset /
 * profili di presentation» perche' *«il livello di indirezione in piu' non ha un consumatore»* — e quel
 * giudizio **resta valido a runtime**. Cio' che e' cambiato e' il consumatore di *authoring*: la persona che
 * deve scegliere fra 85 clip senza riaprirle ogni volta. Una `USTRUCT` letta da JSON serve quella persona
 * senza reintrodurre l'indirezione respinta, e tiene il dato **fuori da ogni `.uasset`**, dove e' diffabile.
 *
 * ⛔ **Non entra in nessun percorso di decisione.** TurnLog, ordinamento e `StateHash` non lo vedono. Se un
 * giorno una regola di gioco leggesse questo catalogo, sarebbe la presentazione che decide un esito — cioe' un
 * fallimento da riportare, non un aggiustamento da fare.
 */
USTRUCT(BlueprintType)
struct FRTAnimCatalog
{
	GENERATED_BODY()

	/**
	 * Versione massima del formato che questo codice sa leggere. Un file piu' NUOVO viene **rifiutato**, e il
	 * messaggio nomina la build.
	 *
	 * 🔑 E' il verso che conta, ed e' la lezione gia' pagata da `URTScenarioLoader::SupportedVersion`: una
	 * build vecchia che **ignora** i campi che non conosce legge un catalogo dimezzato e non se ne accorge —
	 * uscirebbe verde su un dato che non ha capito. Rifiutare accusa la build, che e' la cosa giusta.
	 */
	static constexpr int32 CurrentFormatVersion = 1;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	int32 FormatVersion = FRTAnimCatalog::CurrentFormatVersion;

	/**
	 * High-water mark degli `AV_ID`: il prossimo ID da coniare. Avanza in modo **monotono** e non torna mai
	 * indietro.
	 *
	 * 🔴 **E' l'intera ragione per cui questo campo sta nel formato invece di essere calcolato.** Con
	 * `max(id esistenti) + 1`, rimuovere l'ultima voce farebbe riassegnare il suo ID alla clip successiva —
	 * in silenzio, e con un `AV_ID` che in un commit precedente significava un'altra clip. Il criterio «un
	 * catalogo da cui una voce e' stata rimossa non riassegna quell'ID» **non e' soddisfacibile** senza
	 * persistere il massimo raggiunto.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	int32 NextId = 1;

	/** Le voci, nell'ordine dell'authoring source. L'ordine e' quello del file: il round-trip lo conserva. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TArray<FRTAnimCatalogEntry> Entries;
};
