#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "UI/RTIconCatalogData.h"
#include "RTIconLibrary.generated.h"

// Forward declaration e non `#include "Ability/RTActionDef.h"`: solo `MakeActionIconFallbackId` prende la
// struct, e per riferimento — la definizione serve al `.cpp`, che gia' la include, non a chi include questo
// header. Tirarci dentro il catalogo azioni farebbe dipendere ogni consumatore di icone da `Ability/`.
//
// 🔑 **La chiave PREFERITA prende solo un `FName`, ed e' una proprieta' del design, non un'economia**: la
// traduzione nome->categoria non ha mai avuto bisogno del `Def`. E' il RIPIEGO che legge `DerivedFromActionId`
// e `BaseActionId`, e per questo chi ha solo un `ActionId` sa comunque derivare la chiave giusta.
struct FRTActionDef;

/**
 * Lettura e validazione del catalogo iconografico: pura, deterministica, senza Actor.
 *
 * Stessa disciplina di `URTCatalogLibrary` per le azioni — il validator e' una funzione pura per costruzione,
 * cosi' un catalogo rotto si scopre in CI e non a schermo.
 */
UCLASS()
class REFACTORTACTICS_API URTIconLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Prefisso obbligatorio di ogni `IconId`. Tiene separati lo spazio delle icone e quello dei Gameplay Tag. */
	static const TCHAR* IconIdPrefix;

	/**
	 * `UI.Icon.` + il percorso semantico che il gioco usa gia': `Action.Move` -> `UI.Icon.Action.Move`,
	 * `Status.Wet` -> `UI.Icon.Status.Wet`.
	 *
	 * Una sola regola per tutte le categorie: la chiave dell'icona si DERIVA dall'identificatore stabile che
	 * esiste, non si inventa accanto a lui. E' anche il motivo per cui un ID scritto a mano che sbaglia
	 * categoria e' un errore di validazione e non un mistero a runtime.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Icons")
	static FName MakeIconId(const FName& SemanticPath);

	/** Il nome della categoria come appare dentro l'`IconId` (`ERTIconCategory::Status` -> `Status`). */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Icons")
	static FString CategoryName(ERTIconCategory Category);

	/**
	 * Il primo segmento di `SemanticPath` e' una delle categorie dichiarate da `D-031`?
	 *
	 * ⚠️ **La domanda non e' «esiste un'icona»** — e' «questa chiave puo' esistere». `D-031` dichiara dodici
	 * categorie e la v0.1 ne popola cinque: una chiave in `Reaction.*` e' **legittima e non ancora disegnata**,
	 * una in `Hero.*` e' **malformata**, e le due cose vogliono risposte diverse.
	 */
	static bool IsDeclaredIconCategory(const FName& SemanticPath);

	/**
	 * La chiave icona di un'AZIONE, tradotta quando il suo `ActionId` non porta una categoria dichiarata.
	 *
	 * 🔑 **Gli `ActionId` e le categorie d'icona sono due tassonomie diverse, e questa funzione e' il punto in
	 * cui si incontrano.** `Action.Move` le attraversa entrambe e non ha bisogno di niente; `Hero.Muiren.TideGuard`
	 * vive nello spazio degli id d'azione e **non** in quello delle icone — `MakeIconId` su di lui produrrebbe
	 * `UI.Icon.Hero.Muiren.TideGuard`, il cui segmento `Hero` non e' fra le dodici di `D-031`.
	 *
	 * La traduzione **tiene il NOME e sostituisce il prefisso con la categoria**: `Hero.Aevik.Overload` ->
	 * `UI.Icon.Action.Overload`, `Gadget.Sprinkler` -> `UI.Icon.Action.Sprinkler`.
	 *
	 * 🔑 **E' la stessa regola che `RequiredIconIds` gia' applica agli `HeroId`** (`Hero.Aevik` ->
	 * `Identity.Aevik`), dove il commento dichiarava di essere *«l'unico punto in cui la regola ha bisogno di
	 * una traduzione»*. Non era l'unico: era il primo.
	 *
	 * ⚠️ **Il nome sopravvive perche' il dock risponde a «quale abilita' e' questa»**, non a «che cosa fa».
	 * Due voci dello stesso kit devono avere chiavi diverse: tradurre verso l'azione core le farebbe
	 * collassare sullo stesso disegno, ed e' precisamente cio' che si guarda per sceglierle.
	 *
	 * ⛔ **Questa chiave e' la PREFERITA, non l'unica**: finche' l'asset non e' disegnato il catalogo non la
	 * risolve, e il chiamante ripiega su `MakeActionIconFallbackId`. Vedi `URTActionSlotWidget::GetIconId`.
	 */
	static FName MakeActionIconId(const FName& ActionId);

	/**
	 * Il RIPIEGO della chiave d'azione: l'icona della core da cui l'azione deriva, quando l'asset proprio non
	 * e' ancora stato disegnato.
	 *
	 * ⚠️ **`DerivedFromActionId` PRIMA di `BaseActionId`, e non e' indifferente.** I due campi esistono
	 * separati per decisione (`D-195`): `BaseActionId` dice di quale delle sette generiche un'azione e' il
	 * profilo, l'altro da dove vengono fase, portata ed effetti. L'icona segue cio' che l'azione **fa**,
	 * quindi il secondo. `Hero.Branth.Ram` eredita da `Action.Charge` e un «profilo di Charge» non esiste:
	 * preferendo `BaseActionId` si perderebbe proprio il caso che i due campi distinguono.
	 *
	 * ⛔ **Torna `NAME_None` per un'abilita' PROPRIA**, che non deriva da nessuna core — e non si indovina dal
	 * nome, che e' la regola del docstring di `BaseActionId`. Per quelle non esiste un ripiego: l'asset va
	 * disegnato, e il `NAME_None` e' il modo in cui il buco resta visibile.
	 */
	static FName MakeActionIconFallbackId(const FRTActionDef& Def);

	/**
	 * Il catalogo risolve questa chiave? Domanda **pura e senza log**, per chi deve scegliere fra due chiavi.
	 *
	 * ⛔ **Non e' `ResolveIcon` con l'esito buttato via.** Quella LOGGA quando una chiave non si risolve, ed
	 * e' il suo scopo: dire quale widget ha chiesto un'icona che non c'era. Usarla per *provare* la chiave
	 * preferita emetterebbe una warning ogni volta che il ripiego funziona — cioe' trasformerebbe in rumore
	 * proprio la diagnostica che `#2963` esiste per rendere leggibile.
	 */
	static bool CatalogHasIcon(const URTIconCatalogData* Catalog, const FName& IconId);

	/**
	 * Le chiavi che il catalogo DEVE avere, derivate dai dati di gioco reali e non da una lista scritta a mano:
	 *
	 * - **Phase** — le quattro fasi volontarie del turno (`Prep`, `Dash`, `Blast`, `Move`). Planning e Cleanup
	 *   non sono fasi in cui il giocatore agisce, e Reaction non e' una fase.
	 * - **Action** — ogni azione di `URTCatalogLibrary::GetCoreActionCatalog()`: se un'azione e' pianificabile,
	 *   prima o poi l'HUD deve disegnarla.
	 * - **Status** — ogni tag registrato sotto `Status.` (`Core/RTGameplayTags.cpp`).
	 * - **Certainty** — i tre livelli di CP 11.2 (`Confirmed`, `Predicted`, `Uncertain`). ⚠️ E' l'unica
	 *   categoria senza un tipo da cui derivare: un `ERTCertainty` creato adesso sarebbe un enum che nessuno
	 *   legge. Tre costanti che il catalogo deve coprire valgono piu' di un tipo senza consumatori.
	 * - **Identity** — i quattro eroi di `URTHeroCatalogLibrary::GetHeroIds()`, piu' la relazione di squadra
	 *   (`Ally`, `Enemy`, il cui consumatore e' gia' `ARTHUD`). ⚠️ Il prefisso viene **tradotto**: gli
	 *   `HeroId` sono `Hero.Aevik`, e `MakeIconId` ne farebbe `UI.Icon.Hero.Aevik` — che `ValidateIconCatalog`
	 *   rifiuta, perche' il segmento (`Hero`) non combacia con la categoria (`Identity`). Si tiene il nome e
	 *   si sostituisce il prefisso: `Hero.Aevik` -> `Identity.Aevik`. E' l'unico punto in cui la regola «la
	 *   chiave si deriva dall'identificatore che esiste» ha bisogno di una traduzione.
	 *
	 * Da qui viene il valore di `EveryKeyResolves`: l'insieme richiesto **non** e' il contenuto del catalogo,
	 * quindi il test non puo' passare per costruzione. Aggiungere un tag di stato o un'azione al catalogo fa
	 * cadere la copertura finche' l'icona non esiste — che e' il solo modo perche' questo dato resti vivo.
	 *
	 * Ordine deterministico: le fasi nell'ordine del turno, le azioni nell'ordine del catalogo, gli stati
	 * ordinati lessicalmente (l'ordine dei tag restituiti dal manager non e' garantito), poi i tre livelli di
	 * certezza e infine l'identita' — eroi nell'ordine del roster, `Ally` e `Enemy` in coda. Le due categorie
	 * nuove non hanno bisogno di un sort: iterano array letterali, non un manager.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Icons")
	static TArray<FName> RequiredIconIds();

	/**
	 * Le chiavi richieste che il catalogo non copre (vuoto = copertura completa). Un `Catalog` nullo le
	 * restituisce tutte: nessun catalogo non e' «zero mancanze».
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Icons")
	static TArray<FName> FindMissingRequiredIcons(const URTIconCatalogData* Catalog);

	/**
	 * Errori strutturali del catalogo (vuoto = catalogo valido). Rifiuta:
	 * `IconId` assente · senza il prefisso `UI.Icon.` · duplicato · categoria dichiarata diversa dal segmento
	 * dentro l'ID · asset nullo · missing-icon del catalogo non impostato.
	 *
	 * Ogni messaggio nomina la chiave colpevole: un errore che non dice QUALE riga e' rotta costringe a
	 * ricontrollare tutto il catalogo a mano.
	 *
	 * Non verifica la COPERTURA — quella e' `FindMissingRequiredIcons`. Sono due domande diverse: «questo
	 * catalogo e' coerente?» e «questo catalogo basta al gioco che abbiamo?».
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Icons")
	static TArray<FString> ValidateIconCatalog(const URTIconCatalogData* Catalog);

	/**
	 * Risolve una chiave in un asset. **Non restituisce mai un esito silenziosamente vuoto**: una chiave
	 * sconosciuta da' il missing-icon del catalogo con `bResolved = false` e una warning che nomina la chiave e
	 * il consumer.
	 *
	 * `Consumer` serve al log: senza, «icona mancante» in una partita con dieci widget non dice dove guardare.
	 *
	 * In Development la warning e' rumorosa apposta; in Shipping il comportamento e' lo stesso e non c'e' nulla
	 * che possa crashare — non si dereferenzia niente, si copia una soft reference.
	 *
	 * ⚠️ **`BlueprintCallable` e NON `BlueprintPure`, ed e' una scelta.** Le tre funzioni qui sopra sono pure
	 * perche' non lasciano traccia; questa **logga** quando la chiave non si risolve, e una funzione pura in
	 * un *property binding* viene valutata **a ogni frame**. Con un catalogo incompleto — che e' lo stato
	 * normale finche' `#220` non chiude — un binding puro trasformerebbe la diagnostica in spam continuo, e
	 * la warning smetterebbe di dire dove guardare proprio quando serve.
	 *
	 * ∴ va chiamata **su evento**: `URTActionSlotWidget::OnActionChanged`, cioe' una volta per cambio
	 * azione. E' anche cio' che lo Step 6.4 del piano di `#613` prescrive, subito dopo l'evento dello
	 * Step 6.3.
	 *
	 * ⚠️ **`Consumer` non ha un default apposta.** Passarlo e' il costo di chiamarla: una stringa vuota
	 * produce un log che non dice quale widget ha chiesto l'icona, cioe' toglie alla warning l'unica cosa
	 * per cui esiste. Chi chiama scrive il proprio nome.
	 */
	UFUNCTION(BlueprintCallable, Category = "RefactorTactics|Icons")
	static FRTIconResolution ResolveIcon(const URTIconCatalogData* Catalog, const FName& IconId,
		const FName& Consumer);
};
