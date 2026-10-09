#pragma once

#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNode_SequencePlayer.h"
#include "AnimNodes/AnimNode_Slot.h"
#include "AnimNodes/AnimNode_TwoWayBlend.h"
#include "Unit/RTPresentationRole.h"
#include "RTUnitAnimInstance.generated.h"

class UAnimSequenceBase;


/**
 * Una clip candidata per un ruolo, con la sua identita' e la sua etichetta.
 *
 * `Clip` e' un `TSoftObjectPtr` e non un riferimento duro per una ragione misurata: i pack Paragon
 * vivono in `Content/FabAsset/`, che e' **gitignorato** (riga 97 del `.gitignore`). Un riferimento duro
 * impedirebbe di aprire il progetto a chi clona il repository senza i pack; un soft pointer che non
 * risolve lascia semplicemente il nodo senza sequenza, e l'unita' resta in posa di riferimento invece
 * di far fallire il caricamento.
 */
USTRUCT(BlueprintType)
struct FRTAnimVariant
{
	GENERATED_BODY()

	/**
	 * Identita' della variante.
	 *
	 * ⚠️ **Unica dentro `(eroe, ruolo)`, non globalmente**, ed e' tutto cio' che serve: e' la chiave che
	 * `ActiveClipVariant` nomina, e quella vive dentro un solo `FRTAnimRoleClips`. Lo scan di #2442
	 * assegnera' id globalmente unici, che soddisfano comunque questo vincolo piu' debole — cosi' le due
	 * issue non si aspettano a vicenda.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FName VariantId;

	/** `Heavy`, `Fast`, oppure il neutro `A`/`B`/`C`. Naming locale: non e' un giudizio artistico. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FName Label;

	/** La sequenza. Un soft pointer che non risolve lascia il ruolo senza clip, e non e' un errore. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TSoftObjectPtr<UAnimSequenceBase> Clip;
};

/**
 * L'id riservato alla variante che il default C++ del roster scrive.
 *
 * ⚠️ **E' una funzione e non una costante globale** perche' un `FName` costruito all'inizializzazione
 * statica dipende dalla tabella dei nomi del motore, che a quel punto puo' non esistere ancora.
 */
inline FName RTVarianteRoster() { return FName(TEXT("AV_Roster")); }

/** L'etichetta neutra di partenza: `A`. Le successive sono `B`, `C`, ... */
inline constexpr TCHAR RTPrimaEtichettaNeutra = TEXT('A');

/** Quante etichette neutre esistono prima di esaurire l'alfabeto latino maiuscolo. */
inline constexpr int32 RTNumEtichetteNeutre = 26;

/**
 * Le varianti di UN ruolo di UN eroe, e quale di esse e' attiva.
 *
 * 🔴 **`ActiveClipVariant` non e' `ARTUnit::ActiveVariantId`.** Quella e' la variante d'ABILITA', che
 * sostituisce `Def.Effects` con `Variants[].Effects` e decide esiti; questa nomina una clip e non decide
 * niente. Il nome e' `ActiveClipVariant` e non `ActiveVariant` di proposito: `ActiveVariant` sarebbe una
 * **sottostringa** di `ActiveVariantId`, e un grep su di essa colpirebbe entrambe le tassonomie senza
 * distinguerle.
 */
USTRUCT(BlueprintType)
struct FRTAnimRoleClips
{
	GENERATED_BODY()

	/** Le candidate, nell'ordine in cui l'autore le ha legate. L'ordine non decide niente. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TArray<FRTAnimVariant> Variants;

	/** Chi suona. `NAME_None` = nessuna, e allora il ruolo resta in posa di riferimento. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	FName ActiveClipVariant;

	/** La variante nominata, o `nullptr`. Un id che non esiste non e' un crash: e' «nessuna». */
	REFACTORTACTICS_API const FRTAnimVariant* FindVariant(const FName& VariantId) const;

	/**
	 * La variante attiva, o `nullptr`.
	 *
	 * ⚠️ Restituisce `nullptr` **anche** quando `ActiveClipVariant` nomina una variante che non c'e'
	 * piu': un dato incoerente si legge come «nessuna attiva», non come un crash.
	 */
	REFACTORTACTICS_API const FRTAnimVariant* FindActive() const;

	/**
	 * Lega una variante nuova. Entra **sempre inattiva**, qualunque sia lo stato corrente.
	 *
	 * Con `Label` vuota assegna la prima etichetta neutra libera. Restituisce l'id inserito.
	 */
	REFACTORTACTICS_API FName AddVariant(
		const FName& VariantId, const FName& Label, const TSoftObjectPtr<UAnimSequenceBase>& Clip);

	/**
	 * 🔑 **L'operazione atomica**: la vecchia attiva torna inattiva e la nominata diventa attiva, in un
	 * passo solo. Nessuna doppia conferma.
	 *
	 * Restituisce `false` — e **non tocca niente** — se l'id non esiste. Un `Make Active` fallito non
	 * puo' lasciare il ruolo senza attiva: sarebbe una disattivazione mascherata da errore.
	 */
	REFACTORTACTICS_API bool MakeActive(const FName& VariantId);

	/**
	 * Rimuove una variante.
	 *
	 * ⚠️ **Se era l'attiva, `ActiveClipVariant` diventa `NAME_None` e NON si elegge una sostituta.** La
	 * scelta e' dell'autore, e un ripiego automatico gliela toglierebbe senza dirglielo. Rimuovere una
	 * variante che non era attiva non tocca `ActiveClipVariant`.
	 */
	REFACTORTACTICS_API bool RemoveVariant(const FName& VariantId);

	/**
	 * La prima etichetta neutra libera, tenendo conto dei buchi: con `A` e `C` presenti restituisce `B`.
	 *
	 * Esaurito l'alfabeto restituisce `NAME_None`: ventisei varianti su un ruolo sono gia' oltre
	 * qualunque uso previsto, e inventare `AA` complicherebbe il confronto senza servire a nessuno.
	 */
	REFACTORTACTICS_API FName PrimaEtichettaNeutraLibera() const;
};

/**
 * Le clip di UN'azione, per ruolo di presentazione (#3563, spec «la clip per abilita'» §2.1).
 *
 * 🔴 **Esiste per lo stesso vincolo di `FRTHeroPresentationClips`**: UHT non ammette contenitori ANNIDATI come
 * `UPROPERTY`, e `TMap<FName, TMap<ERTPresentationRole, ...>>` non compila.
 *
 * ⚠️ **Un pool DISTINTO da quello di ruolo**, e «una sola attiva» vale per pool: una clip di ruolo e una d'azione
 * attive per lo stesso `(eroe, ruolo)` convivono, e a risolvere vince l'azione (`ActiveClipFor` a quattro
 * argomenti). L'alternativa — un secondo `ActiveClipVariant` per azione dentro il pool di ruolo — mescolava le
 * varianti dei due livelli e rendeva «una sola attiva» ambiguo (spec §2.1, alternativa scartata).
 */
USTRUCT(BlueprintType)
struct FRTActionPresentationClips
{
	GENERATED_BODY()

	/** Solo i ruoli che qualcuno ha popolato. In v0.1 li consultano solo `Cast` e `Attack` (spec D3). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TMap<ERTPresentationRole, FRTAnimRoleClips> PerRole;
};

/**
 * I ruoli di UN eroe.
 *
 * 🔴 **Questa struct esiste per un vincolo del motore, non per stile: UHT non supporta i contenitori
 * ANNIDATI come `UPROPERTY`.** `TMap<FName, TMap<ERTPresentationRole, ...>>` non compila. Il modo
 * previsto e' una `USTRUCT` intermedia, e il commento sta qui perche' nessuno la «semplifichi» via
 * scoprendo il vincolo a build rotta.
 */
USTRUCT(BlueprintType)
struct FRTHeroPresentationClips
{
	GENERATED_BODY()

	/** Solo i ruoli che qualcuno ha popolato. Un ruolo assente non e' un errore: e' un ruolo assente. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TMap<ERTPresentationRole, FRTAnimRoleClips> PerRole;

	/**
	 * Clip per `(ActionId, ruolo)` (#3563). Chiave: l'`ActionId` dell'evento — un profilo `Hero.X.Y` o una generica
	 * `Action.Z` — mai derivato a valle (`RTResolvedEvent.h:393-397`).
	 *
	 * ⚠️ Un'azione assente, un ruolo assente o una variante non attiva sono tutti «non popolato», e la risoluzione
	 * passa al livello successivo: e' `ActiveClipFor` a quattro argomenti a saperlo, non chi chiama.
	 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TMap<FName, FRTActionPresentationClips> PerAction;

	/** Le varianti del ruolo, o `nullptr` se nessuno l'ha popolato. */
	REFACTORTACTICS_API const FRTAnimRoleClips* FindRole(ERTPresentationRole Role) const;
};

/**
 * 🔴 **Il grafo di animazione dell'unita', in C++ e senza nessun `.uasset`.**
 *
 * La via ovvia sarebbe duplicare l'AnimBlueprint di ogni pack Paragon e ricablarne l'ingresso: quei
 * grafi esistono, sono completi e i loro skeleton combaciano con le mesh che i `BP_Unit_*` gia' usano.
 * E' stata scartata su un numero: **650–735 KB l'uno, ~2,8 MB per quattro**, contro gli **0,7 MB** che
 * pesa oggi tutto `Content/` versionato. Quadruplicare il contenuto binario del repository per una
 * macchina a due stati e' un prezzo che si paga a ogni salvataggio successivo, perche' i `.uasset` non
 * si comprimono per delta.
 *
 * Qui le clip sono **dati versionati e diffabili**, il grafo e' codice sotto test, e il repository non
 * cresce di un byte.
 *
 * ⚠️ **Solo presentazione** (invariante #1): niente qui decide un esito. Se una clip manca, l'unita'
 * resta in posa di riferimento e la partita si gioca uguale.
 */
UCLASS()
class REFACTORTACTICS_API URTUnitAnimInstance : public UAnimInstance
{
	GENERATED_BODY()

public:
	URTUnitAnimInstance();

	/**
	 * Clip per `HeroId`, per ruolo. Il default C++ punta ai pack del roster; resta `EditDefaultsOnly`,
	 * quindi un Blueprint figlio puo' scavalcare una voce senza ricompilare.
	 *
	 * ⚠️ **I nomi delle clip NON si deducono**: `docs/technical/runbooks/guida-animazioni-paragon.md`
	 * §AS.3b li ha misurati sul disco, e **sei caselle su venti** non si chiamano come ci si aspetta —
	 * su Aevik la corsa e' `Run_Fwd` e non `Jog_Fwd`, su Ivrin l'idle e' `Idle_NonCombat`.
	 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "RefactorTactics|Anim")
	TMap<FName, FRTHeroPresentationClips> ClipsPerHero;

	/** I ruoli dell'eroe passato, o `nullptr` se non ce ne sono: un eroe senza voce non e' un errore. */
	const FRTHeroPresentationClips* FindClipsFor(const FName& HeroId) const { return ClipsPerHero.Find(HeroId); }

	/**
	 * La clip attiva di `Role` per `HeroId`, o un puntatore nullo.
	 *
	 * 🔑 **E' l'unico modo previsto di chiedere «cosa suona qui»**, ed esiste perche' le tre condizioni
	 * che danno «niente» — eroe fuori catalogo, ruolo non popolato, nessuna variante attiva — sono
	 * tutte e tre normali e nessuna e' un errore. Chi le controlla a mano ne dimentica una.
	 */
	TSoftObjectPtr<UAnimSequenceBase> ActiveClipFor(const FName& HeroId, ERTPresentationRole Role) const;

	/**
	 * La clip attiva di `Role` per `HeroId` quando il beat conosce l'AZIONE (#3563, spec D2): `PerAction[ActionId]`,
	 * poi `PerAction[BaseActionId]`, poi `PerRole` — l'overload a due argomenti qui sopra.
	 *
	 * 🔴 **Ogni livello «non popolato» passa al successivo**: azione assente, ruolo assente dalla voce dell'azione,
	 * nessuna variante attiva. Una voce d'azione che ha `Cast` ma non `Attack` NON ferma un `Attack`.
	 * ⛔ **`BaseActionId` vuoto salta il proprio livello**: non si deriva dal profilo (`Ruling` di §2.2).
	 */
	TSoftObjectPtr<UAnimSequenceBase> ActiveClipFor(const FName& HeroId, ERTPresentationRole Role,
		const FName& ActionId, const FName& BaseActionId) const;

protected:
	virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
};

/**
 * Se `Clip` SUONA come additiva: un delta che lo slot somma alla posa corrente, non una posa intera (#3590).
 *
 * 🔑 **Decide il test dell'engine, `IsValidAdditive()`, e non il tipo autorato.** `UAnimSequence` tratta la sequenza
 * come additiva solo quando e' valida (`AnimSequence.cpp`, `bTreatAnimAsAdditive`), e lo slot decide allo stesso modo
 * per il montaggio. Un `AdditiveAnimType` senza una posa di riferimento valida suona come posa PIENA: rifiutarla
 * toglierebbe una clip che si vede. `nullptr` non e' additiva: e' un'altra domanda, e la fa chi chiama.
 */
REFACTORTACTICS_API bool RTClipIsAdditive(const UAnimSequenceBase* Clip);

/**
 * Se il ruolo e' un GESTO che deve SOSTITUIRE la posa: `Cast` e `Attack`, i due beat che conoscono l'azione (D3) (#3590).
 *
 * 🔴 **Su un gesto un'additiva non si legge.** Lo slot la somma all'`Idle`: le clip d'azione di Aevik
 * (`LMB_Fire_*`, `Ability_Q_Target`, `Throw_Ready`) sono il rinculo e la mira di un personaggio gia' in posa di tiro,
 * e sull'`Idle` la posa non cambiava (seduta `U70`), mentre tracer e numeri arrivavano.
 * ⛔ **`Hit` NO, ed e' misurato a schermo**: la hit-react e' un sussulto autorato additivo in tutti e quattro i pack,
 * pensato per sommarsi alla posa corrente, e la seduta `U8` (`PIE-AS4b`, 2026-09-28) l'ha vista sui quattro eroi.
 * `Death`, `Idle` e `Move` restano fuori: nessuna misura dice che un'additiva li' sia invisibile.
 */
REFACTORTACTICS_API bool RTRoleWantsAFullBodyClip(ERTPresentationRole Role);

/**
 * Il grafo vero e proprio: due sequence player, un blend fra loro, uno slot per i montaggi.
 *
 *     Idle  ─┐
 *            ├─ TwoWayBlend ── Slot('DefaultSlot') ── Output
 *     Run   ─┘      (alpha)         (Cast/Hit/Death)
 *
 * Il modello e' `FAnimSequencerInstanceProxy` dell'engine, che costruisce i propri nodi allo stesso
 * modo — `SetLinkNode` sui `FPoseLink`, `Update_AnyThread`/`Evaluate_AnyThread` sul nodo radice.
 */
USTRUCT()
struct FRTUnitAnimProxy : public FAnimInstanceProxy
{
	GENERATED_BODY()

	FRTUnitAnimProxy() = default;
	explicit FRTUnitAnimProxy(UAnimInstance* InAnimInstance) : FAnimInstanceProxy(InAnimInstance) {}

	virtual void Initialize(UAnimInstance* InAnimInstance) override;
	virtual void PreUpdate(UAnimInstance* InAnimInstance, float DeltaSeconds) override;
	virtual void UpdateAnimationNode(const FAnimationUpdateContext& InContext) override;
	virtual bool Evaluate(FPoseContext& Output) override;

	/**
	 * 🔴 **I due agganci con cui un grafo montato a mano si DICHIARA al motore.**
	 *
	 * `FAnimInstanceProxy::InitializeRootNode` assegna la radice da qui e da nessun altro posto —
	 * `RootNode = (FAnimNode_Base*) GetCustomRootNode();`. Senza questo override `RootNode` resta
	 * `nullptr`, e allora **due delle quattro traversate non arrivano mai ai nodi**:
	 *
	 *  1. `Initialize_AnyThread`, che i nodi non ricevono mai;
	 *  2. `CacheBones`, che e' il punto in cui i `FBoneReference` si riallineano all'array delle
	 *     *required bones* — e quell'array **cambia a ogni cambio di LOD**
	 *     (`FAnimInstanceProxy::OnPreUpdateLODChanged`).
	 *
	 * ⚠️ **Il difetto non si vede all'avvio**, ed e' la ragione per cui e' passato: le prime
	 * animazioni sono corrette, e la posa degrada solo dopo che il LOD e' cambiato. Su un umanoide una
	 * cache stantia si nota appena; su **Branth**, che porta decine di ossa in fila
	 * (`arm_chain_long_r_01`, `_sub_01`, `_sub_02`, ...), le catene si stendono sullo schermo.
	 *
	 * ⛔ **Non e' un dettaglio di stile**: `FRTUnitAnimClipsTest` verifica che le clip dei quattro
	 * eroi siano nel default C++ e resta verde comunque, perche' un test sui DATI non vede un grafo
	 * che non viene inizializzato. Vedi `#1763`.
	 */
	virtual FAnimNode_Base* GetCustomRootNode() override { return &Slot; }

	/** I nodi del grafo, perche' il motore possa raggiungerli tutti e non solo la radice. */
	virtual void GetCustomNodes(TArray<FAnimNode_Base*>& OutNodes) override
	{
		OutNodes.Add(&IdleNode);
		OutNodes.Add(&RunNode);
		OutNodes.Add(&Blend);
		OutNodes.Add(&Slot);
	}

	/** Quanto l'unita' sta correndo, `0` fermo e `1` in corsa. Lo copia `PreUpdate` dal game thread. */
	float GetRunAlpha() const { return Blend.Alpha; }

private:
	/**
	 * ⚠️ **`_Standalone` e non `FAnimNode_SequencePlayer`**: la variante normale prende la propria
	 * sequenza dalla proprieta' che il compilatore dell'AnimBlueprint le assegna, e fuori da un grafo
	 * compilato resta vuota. La `_Standalone` esiste per l'uso da codice, ed e' l'unica delle due che
	 * si puo' riempire a mano.
	 */
	FAnimNode_SequencePlayer_Standalone IdleNode;
	FAnimNode_SequencePlayer_Standalone RunNode;

	FAnimNode_TwoWayBlend Blend;
	FAnimNode_Slot Slot;
};
