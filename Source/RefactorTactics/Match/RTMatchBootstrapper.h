#pragma once

#include "CoreMinimal.h"
#include "Templates/SubclassOf.h"
#include "Map/RTMapSource.h"
#include "Turn/RTMatchFormatData.h"

class ARTHexMapActor;
class ARTTurnManager;
class ARTUnit;
class URTMatchFormatData;
struct FRTStartupReport;

/**
 * COSA e' stato chiesto per questa partita: i valori **gia' risolti** da chi la ordina.
 *
 * 🔑 **La linea che questa struct disegna e' la ragione per cui esiste.**
 *
 *     ARTGameMode          risolve COSA e' stato richiesto
 *     FRTMatchBootstrapper costruisce COME quella partita nasce
 *
 * ⛔ **Il bootstrapper non legge console variable ne' riga di comando.** Le tre scale di precedenza del
 * progetto — scenario, sorgente mappa, autobattle — hanno la stessa forma e vivono tutte nello stesso file,
 * dove si confrontano fra loro. Spostarne una qui la separerebbe dalle sorelle e aprirebbe una seconda sede
 * per la domanda «chi vince»: e' la stessa ragione per cui `RTScenarioEntry::Winner` esiste in un posto solo.
 *
 * ∴ un test puo' allestire una partita **senza toccare lo stato globale del processo**, che con le console
 * variable non e' possibile: una cvar dura quanto l'editor e la si dimentica accesa.
 */
struct FRTMatchBootstrapConfig
{
	/** La sorgente mappa gia' risolta (proprieta' < riga di comando < console). */
	ERTMapSource MapSource = ERTMapSource::LevelAsset;

	/**
	 * La fixture per NOME, gia' letta dalla console. Vuota = nessuna.
	 *
	 * ⚠️ E' il gradino piu' specifico dei tre e vince su `MapSource`, ma un nome **sconosciuto** non ripiega
	 * in silenzio: si dichiara e si prosegue con la sorgente configurata.
	 */
	FString MapFixtureId;

	/** Raggio dell'arena di ripiego. `0` = nessun ripiego: la partita non si allestisce e il log lo dice. */
	int32 DemoArenaRadius = 4;

	/** Il formato assegnato, se c'e'. Assente ⇒ formato spedito ⇒ ripiego. */
	const URTMatchFormatData* MatchFormat = nullptr;

	/** Quale formato SPEDITO usare quando `MatchFormat` non e' assegnato. */
	FName ShippedFormatId;

	/** Le due formazioni per `HeroId` del catalogo. La loro cardinalita' deve combaciare col formato. */
	TArray<FName> Team0Heroes;
	TArray<FName> Team1Heroes;

	/** Classe visiva per `HeroId`. Un eroe assente ricade su `ARTUnit` — il cilindro segnaposto. */
	TMap<FName, TSubclassOf<ARTUnit>> HeroUnitClasses;

	/** La modalita' non presidiata, gia' risolta. Entrambe le squadre al bot. */
	bool bAutobattle = false;

	/** Chi l'ha decisa, per il log e per la banda: `BP_GameMode`, `rt.Match.Autobattle`, `-RTAutobattle`. */
	FString AutobattleSourceLabel;

	/**
	 * Quante unita' della squadra del GIOCATORE sono pianificate dal bot, contate dal FONDO di `Team0Heroes`.
	 *
	 * `0` — nessuna: e' il comportamento storico, e resta il default. `1` su una formazione `[Gadget, Phase]`
	 * mette Phase al bot e lascia Gadget al giocatore.
	 *
	 * ⚠️ **Dal fondo, e non «tutti tranne il primo»**: la regola deve valere anche a 3v3 (D-256), dove il
	 * giocatore puo' volerne comandare due su tre. Un intero risponde a quella domanda, un booleano no.
	 *
	 * ⛔ **Non e' l'autobattle a meta'.** Quella modalita' toglie il giocatore dalla partita e lo dice alla
	 * banda; questa gli lascia il comando di almeno un'unita' — `FRTMatchBootstrapper` cappa il valore
	 * perche' una squadra 0 interamente al bot senza autobattle non avrebbe nessuno che possa chiudere il
	 * turno. Le due si sommano senza contraddirsi: con l'autobattle in vigore il cap non serve, perche' li'
	 * l'assenza di input e' il punto.
	 */
	int32 BotAllyCount = 0;

	/** Chi l'ha deciso, per il log: `BP_GameMode`, `rt.Match.BotAllies`, `-RTBotAllies`. */
	FString BotAllySourceLabel;

	/** Secondi di Planning gia' risolti. **Negativo = non intervenire**: vale il valore del `TurnManager`. */
	float PlanningSeconds = -1.f;
};

/**
 * Cosa e' successo all'allestimento, per chi lo ha ordinato.
 *
 * ⚠️ **Non e' il rapporto d'avvio**: quello e' `FRTStartupReport`, lo legge un widget, ed elenca le
 * condizioni. Questo dice al chiamante le quattro cose che deve scrivere nel proprio stato, e nient'altro.
 */
struct FRTMatchBootstrapOutcome
{
	/**
	 * 🔑 **L'allestimento e' arrivato a DECIDERE la modalita' della sessione.**
	 *
	 * Falso quando il formato non si risolve: li' non si allestisce niente, e chi ordina non deve latchare
	 * una modalita' per una partita che non esiste — la banda a schermo annuncerebbe un autobattle che non
	 * sta girando. E' la stessa distinzione fra «cosa si puo' chiedere» e «cosa la partita e'» che
	 * `IsAutobattleInEffect()` esiste per tenere.
	 */
	bool bModeLatched = false;

	/** La modalita' in vigore. Significativa **solo** se `bModeLatched`. */
	bool bAutobattleInEffect = false;

	/** Le unita' sono entrate in campo. Falso anche quando il livello portava gia' le proprie. */
	bool bUnitsSpawned = false;

	/**
	 * Le regole in vigore, valide **solo** se `bModeLatched`. Escono di qui perche' il composition root ne
	 * ha bisogno per derivare i posti dei giocatori: e' un OUTPUT dell'allestimento, non una seconda
	 * risoluzione del formato — quella resta una sola.
	 */
	FRTMatchRules Rules;
};

/**
 * COME NASCE UNA PARTITA: mappa, formato, ritmo del turno, celle di partenza, roster, unita', equipaggiamento.
 *
 * ## Perche' e' uscito dal GameMode
 *
 * Erano dieci responsabilita' in una funzione di duecentotrenta righe, e ognuna aveva una ragione diversa di
 * cambiare: la geometria della mappa, il vocabolario dei formati, la modalita' non presidiata, il catalogo
 * eroi, l'equipaggiamento. Il GameMode le teneva insieme perche' e' il primo Actor che esiste, non perche'
 * fossero la stessa cosa.
 *
 * ## Cosa NON fa, e va detto perche' sarebbe facile
 *
 * - ⛔ **Non risolve precedenze**: vedi `FRTMatchBootstrapConfig`.
 * - ⛔ **Non conosce il frontend**: un allestimento che fallisce lo dichiara nel rapporto, non apre modali.
 * - ⛔ **Non apre una pipeline parallela per i bot** (invariante #10): cambia **chi** e' segnato come bot,
 *   non **come** decide. `PlanBots` -> `ChooseBestPlan` resta la sola strada.
 * - ⛔ **Non ricodifica «2v2»**: la composizione la dichiara il FORMATO (`UnitsPerTeam`), ed e' la ragione
 *   per cui lo stress 4v4 e' un formato e non un caso speciale del codice di allestimento.
 *
 * Classe C++ pura: nessuno stato, nessuna reflection, nessun delegate. Il fatto che sia un insieme di
 * funzioni statiche e' la dichiarazione che l'allestimento non ha memoria fra una partita e l'altra.
 */
class REFACTORTACTICS_API FRTMatchBootstrapper
{
public:
	/**
	 * Allestisce la partita sulla mappa data.
	 *
	 * Fail-closed su ogni dato di gameplay invalido: formato non risolvibile, formato incompatibile con la
	 * mappa, celle di partenza insufficienti, formazione che non combacia col formato, eroe assente dal
	 * catalogo. In tutti quei casi **nessuna unita' entra in campo** — una partita allestita a meta' a
	 * schermo somiglia a una partita normale, ed e' il difetto piu' caro da diagnosticare.
	 *
	 * @param HexMap       l'actor mappa: e' anche l'`Outer` delle arene generate e la fonte del contesto
	 *                     geometrico. `nullptr` -> non fa nulla.
	 * @param TurnManager  destinatario di regole, ritmo e modalita'. `nullptr` e' **degradato, non fatale**:
	 *                     la partita prosegue senza limite di round, e il rapporto lo dichiara.
	 * @param Report       il rapporto d'avvio, che questa funzione **scrive**: fase raggiunta e condizioni.
	 *                     Il chiamante lo azzera prima, perche' e' lui a sapere quando una partita ricomincia.
	 */
	static FRTMatchBootstrapOutcome Bootstrap(ARTHexMapActor* HexMap, ARTTurnManager* TurnManager,
		const FRTMatchBootstrapConfig& Config, FRTStartupReport& Report);
};
