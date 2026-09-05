#pragma once

#include "CoreMinimal.h"

/**
 * LA GUARDIA STRUTTURALE DELLA PRIVACY: un tipo dichiarato `server-only` non raggiunge un client.
 *
 * 🔑 **Non duplica `URTIntentPrivacyLibrary::FilterForTeam`, e la differenza è tutto il punto.** Il filtro
 * risponde a «*questo osservatore ha diritto a questo dato?*» — privacy **logica**, e quattro test la
 * presidiano già. Questa guardia risponde a un'altra domanda: «*esiste una via per cui quel dato partirebbe
 * comunque?*» — privacy **strutturale**. Un `FilterForTeam` perfettamente corretto non impedisce a
 * `FRTPlannedIntent` di finire dentro una `UPROPERTY(Replicated)` in un refactor futuro, e a quel punto il
 * byte parte senza che nessun test logico se ne accorga. È la distinzione che PDR-04 chiama *leak diretto*
 * contro *leak indiretto*.
 *
 * ⚠️ **Nasce verde per assenza, e per questo il controllo positivo non è opzionale.** Misurato su
 * `fac75cff`: zero proprietà `Replicated` in tutto `Source/` — l'unica occorrenza della parola è dentro un
 * commento (`Player/RTPlayerState.h`). Uno sweep che non può fallire è indistinguibile da un checker rotto,
 * quindi `RTServerOnlyGuardFixtures.h` pianta un leak vero e un test asserisce che la guardia lo trovi.
 * Senza quel secondo test, «zero leak» e «guardia cieca» sono lo stesso output.
 *
 * ⛔ **Cosa NON copre, perché nessuno lo deduca dal nome.**
 * - **La serializzazione su file.** TurnLog e replay hanno il proprio owner — `URTReplayPrivacyLibrary`,
 *   [D-316]/[D-317] — e la propria domanda. Qui si guardano solo le vie di **rete**.
 * - **La privacy logica.** Se un DTO filtrato contiene il campo sbagliato, questa guardia resta verde: è
 *   `FilterForTeam` a decidere cosa entra in `FRTIntentView`, e sono i suoi test a dirlo.
 * - **`AActor::bReplicates` su una classe server-only.** Sarebbe la quarta rotta, e oggi **non ha soggetto**:
 *   nessuna `UCLASS` è marcata `RTServerOnly`, quindi il controllo girerebbe sull'insieme vuoto. Si aggiunge
 *   il giorno in cui una classe lo diventa — non prima, o è una riga che nessuno può vedere fallire.
 *
 * Owner documentale: `docs/product/piano-canonico-mvp.md` §5 invariante #6 · PDR-04 §9 passo 6.
 */

/** La via per cui un tipo `server-only` raggiungerebbe un client. */
enum class ERTLeakRoute : uint8
{
	/** Il tipo ha esso stesso una proprietà `CPF_Net`. È il caso letterale del DoD. */
	OwnMember,

	/**
	 * Una `UPROPERTY(Replicated)` — ovunque nel grafo dei tipi — lo **contiene**, direttamente o annidato.
	 * È la rotta che conta davvero per una `USTRUCT`: `FRTPlannedIntent` non avrà mai un membro replicato,
	 * perché la replica si dichiara sulla **classe** che lo trasporta.
	 */
	ReplicatedProperty,

	/** Un parametro di una `UFunction` con `FUNC_Net` lo contiene. PDR-04 §9 passo 3 nomina gli RPC. */
	RpcParameter,
};

/** Una via di fuga trovata. Il `Path` serve a chi deve capirla, non solo a sapere che esiste. */
struct FRTReplicationLeak
{
	/** Il tipo marcato `RTServerOnly` che verrebbe esposto. */
	FName ServerOnlyType;

	ERTLeakRoute Route = ERTLeakRoute::OwnMember;

	/** Chi lo trasporta: `UClasse::Proprietà` oppure `UClasse::Funzione(param)`. */
	FString Carrier;

	/** La catena di tipi che ci arriva, per un leak annidato: `FOuter::Campo -> FInner::Campo -> FTarget`. */
	FString Path;

	/** Riga leggibile per il messaggio di fallimento del test. */
	FString Describe() const;
};

namespace RTServerOnlyGuard
{
	/** La chiave di metadata che dichiara un tipo `server-only`: `USTRUCT(meta = (RTServerOnly))`. */
	extern REFACTORTACTICS_API const TCHAR* const ServerOnlyMetaKey;

	/**
	 * La chiave che marca una fixture di test — un tipo che viola **apposta**, per provare che la guardia
	 * veda. Lo sweep di produzione la esclude, e **conta** le esclusioni: un'esclusione che cresce in
	 * silenzio è il modo in cui un gate smette di coprire senza mai diventare rosso.
	 */
	extern REFACTORTACTICS_API const TCHAR* const FixtureMetaKey;

	/**
	 * Vero se la reflection espone i metadata su questo binario.
	 *
	 * ⚠️ **Misurato, non assunto**: `HasMetaData` vive sotto `WITH_METADATA`, e `CoreMiscDefines.h` lo
	 * definisce come `WITH_EDITORONLY_DATA`. In un binario senza metadata la guardia non può leggere le
	 * dichiarazioni — e il test che la usa deve **fallire**, non passare. Un gate che tace quando non può
	 * misurare è peggio di un gate assente: produce un verde.
	 */
	REFACTORTACTICS_API bool IsMetadataAvailable();

	/**
	 * Tutti i tipi marcati `RTServerOnly` fra quelli caricati — `UScriptStruct` e `UClass`.
	 *
	 * @param bIncludeFixtures  se falso, esclude i tipi marcati anche `RTServerOnlyGuardFixture`.
	 */
	REFACTORTACTICS_API TArray<UStruct*> CollectServerOnlyTypes(bool bIncludeFixtures);

	/**
	 * Le vie di fuga per l'insieme dato, in un solo attraversamento del grafo di reflection.
	 * Vuoto = nessuna. Funzione pura: legge la reflection e non tocca nessuno stato.
	 */
	REFACTORTACTICS_API TArray<FRTReplicationLeak> FindLeaks(const TArray<UStruct*>& ServerOnlyTypes);

	/** Comodità per un tipo solo: è la forma che usa il controllo positivo. */
	REFACTORTACTICS_API TArray<FRTReplicationLeak> FindLeaksForType(UStruct* ServerOnlyType);
}
