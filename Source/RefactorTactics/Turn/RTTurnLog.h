#pragma once

#include "CoreMinimal.h"
#include "Core/RTTypes.h"
#include "Turn/RTTurnRules.h"
#include "RTTurnLog.generated.h"

/**
 * Categoria dell'esito registrato nel TurnLog.
 *
 * `Fallback` e' un valore AGGIUNTO in coda (CP 4.3), `Reaction` un altro (CP 5.1): la categoria viaggia come
 * uint8 nel formato serializzato, quindi i log scritti prima restano leggibili e la versione del formato non
 * cambia. Aggiungerli in mezzo avrebbe invece rinumerato `Combat`, cioe' riscritto il significato dei file
 * gia' su disco.
 */
UENUM(BlueprintType)
enum class ERTLogCategory : uint8 { Move, Combat, Fallback, Reaction, Environment };

/**
 * Come una CELLA e' cambiata durante il turno (CP 8.4). La mappa e' un sistema di gioco: se una cella prende
 * fuoco o si allaga, il replay deve poterlo dire — altrimenti una unita' che a T+1 incassa danno «senza
 * motivo» resta inspiegabile, e il TurnLog non e' piu' una traccia autoritativa.
 *
 * `Amount` della voce porta i turni di durata; `TgtCell` la cella modificata; `ActionId` chi l'ha causata.
 */
UENUM(BlueprintType)
enum class ERTEnvironmentOutcome : uint8
{
	/** La superficie della cella e' cambiata (fuoco acceso, acqua creata). */
	SurfaceChanged,
	/** Una modifica temporanea e' scaduta: la cella e' tornata alla superficie originale. */
	SurfaceRestored,
	/** La modifica non e' avvenuta: la superficie di destinazione non l'ammette (acqua e metallo non bruciano). */
	SurfaceRejected,
	/** Una superficie ne ha rimossa un'altra: l'acqua che arriva sul fuoco lo spegne. */
	SurfaceExtinguished,
	/**
	 * Una copertura ha incassato danno ed e' ancora in piedi (CP 9.2). `SrcCell` e `TgtCell` sono le due
	 * celle del BORDO — la coppia lo identifica senza aggiungere un campo direzione al log — e `Amount`
	 * l'integrita' RESIDUA, cosi' chi legge il replay sa quanto manca al crollo.
	 */
	CoverDamaged,
	/**
	 * Una copertura e' stata abbattuta: da qui in poi quel bordo si attraversa. E' l'evento che spiega perche'
	 * al turno successivo una linea di tiro esiste dove prima non c'era.
	 */
	CoverDestroyed,
	/**
	 * Una porta si e' chiusa (CP 9.3): da qui in poi quel bordo non si attraversa e non si vede attraverso.
	 * `SrcCell`/`TgtCell` sono le due celle del BORDO, come per le coperture — nessun campo nuovo nel log.
	 */
	DoorClosed,
	/** Una porta si e' aperta: spiega perche' esiste un passaggio dove prima non c'era. */
	DoorOpened,
	/**
	 * Un ponte e' comparso (CP 9.4): `SrcCell`/`TgtCell` sono le due celle collegate e `Amount` i turni che
	 * durera' (0 = permanente).
	 */
	BridgeCreated,
	/** Un ponte e' stato tolto o e' scaduto: da qui i due layer sono di nuovo separati. */
	BridgeRemoved,
	/** Un ponte ha incassato danno ed e' ancora in piedi; `Amount` e' l'integrita' RESIDUA. */
	BridgeDamaged,
	/** Un ponte e' crollato: e' l'evento che spiega perche' un percorso fra due layer non esiste piu'. */
	BridgeDestroyed,
	/**
	 * Una copertura e' stata ERETTA in partita (CP 9.5): `SrcCell`/`TgtCell` sono le due celle del bordo, come
	 * per ogni altro evento di struttura, e `Amount` i turni che durera' (0 = permanente).
	 *
	 * Aggiunto IN CODA: l'esito viaggia come `uint8` nel TurnLog serializzato, quindi inserirlo in mezzo
	 * rinumererebbe gli eventi gia' scritti e i replay del corpus golden racconterebbero un'altra partita.
	 */
	CoverCreated,
	/** Una copertura temporanea e' scaduta: da qui in poi quel bordo non ripara piu' nessuno. */
	CoverExpired,
	/** Una copertura e' stata SPOSTATA su un altro bordo (`Bastion.Reconfigure`): non ne nasce una seconda. */
	CoverMoved,
	/**
	 * La copertura non e' nata: bersaglio fuori portata, bordo non dichiarato o gia' riparato. E' il `Cancel`
	 * che il catalogo dichiara, reso VISIBILE — un'azione che sparisce in silenzio e' indistinguibile da un bug.
	 */
	CoverRejected
};

/**
 * Quale fallback e' stato applicato a un'azione che non era piu' eseguibile. Speculare a `ERTActionFallback`
 * (catalogo §7), ma e' un tipo separato: questo e' cio' che il turno ha REGISTRATO, quello e' cio' che
 * l'azione DICHIARA — e i due possono differire (`BasicAttack` degrada a `Cancel` in v0.1).
 */
UENUM(BlueprintType)
enum class ERTFallbackOutcome : uint8
{
	Stopped,      // Fallback.Stop: fermata all'ultima posizione valida
	Waited,       // Fallback.Wait: azione sostituita con l'attesa
	AttackedCell, // Fallback.AttackCell: persa l'unita' bersaglio, colpita la cella pianificata
	Cancelled     // Fallback.Cancel: nessun effetto (ci finisce anche BasicAttack, non praticabile in v0.1)
};

/**
 * Esito del movimento di un'unita' nel turno (dal resolver ResolvePaths).
 *
 * `BlockedByPriority` e `BlockedByImpact` sono valori AGGIUNTI in coda (CP 4.8, collisioni con priorita'):
 * viaggiano come uint8 nel formato serializzato, quindi i log gia' scritti restano leggibili.
 */
UENUM(BlueprintType)
enum class ERTMoveOutcome : uint8
{
	Stayed,            // non pianificava movimento (path < 2 celle)
	Moved,             // raggiunta la destinazione pianificata (scambio incluso)
	BlockedContested,  // fermata (o parziale) per destinazione contesa a PARITA' di priorita'
	BlockedByUnit,     // fermata (o parziale) per cella occupata da un'unita' ferma
	BlockedByPriority, // fermata per destinazione contesa persa contro una mobilita' con priorita' migliore
	BlockedByImpact,   // fermata per scontro frontale con un'altra mobilita' lineare in arrivo opposto
	/**
	 * Fermata perche' la TOPOLOGIA e' cambiata dopo che il percorso era stato pianificato: una porta chiusa,
	 * un muro alzato, una cella sparita (CP 9.3). Aggiunto in CODA: le tracce gia' scritte non cambiano
	 * significato.
	 *
	 * Lo scrive il chiamante e non `ResolveHexPaths`: il troncamento avviene PRIMA che il resolver veda il
	 * percorso, quindi lui classificherebbe `Moved` — vero sul percorso troncato, falso su cio' che l'unita'
	 * aveva pianificato.
	 */
	BlockedByTopology
};

/** Esito di un attacco nel turno. Priorita': Lethal > ShieldAbsorbed > TerrainBonus > Hit. */
UENUM(BlueprintType)
enum class ERTCombatOutcome : uint8
{
	Hit,            // danno inflitto agli HP
	ShieldAbsorbed, // danno assorbito interamente dallo scudo (HP invariati)
	Lethal,         // bersaglio portato a HP <= 0
	NoLineOfSight,  // attacco pianificato scartato per LOS bloccata
	TerrainBonus,   // colpo a segno con bonus altura (+danno), non letale
	/**
	 * Cura applicata (CP 8.5, `Action.Heal`). Valore AGGIUNTO in coda come `Fallback`/`Reaction` prima: le
	 * tracce gia' scritte non cambiano significato. Sta fra gli esiti di combattimento e non in una categoria
	 * propria perche' e' la stessa domanda — quanti punti vita ha cambiato quell'azione, e a chi.
	 */
	Healed
};

/**
 * Voce del TurnLog: un esito autoritativo del turno con il suo reason code. Osservabilita' separata
 * dalla presentazione (non e' FRTResolvedEvent). Deterministica: la chiave dell'unita' e' la sua cella
 * di partenza del turno (max 1 unita'/cella), mai un pointer.
 */
USTRUCT(BlueprintType)
struct FRTTurnLogEntry
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	ERTMatchPhase Phase = ERTMatchPhase::Move;

	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	ERTLogCategory Category = ERTLogCategory::Move;

	/**
	 * Valore dell'enum di categoria: `ERTMoveOutcome` se Move, `ERTCombatOutcome` se Combat, `ERTFallbackOutcome`
	 * se Fallback, `ERTReactionOutcome` se Reaction. Intero: no float (#4).
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	uint8 Outcome = 0;

	/** Chiave stabile: cella di partenza dell'unita' nel turno. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FRTCellId SrcCell;

	/** Bersaglio (Combat) o destinazione (Move); = SrcCell se non applicabile. */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FRTCellId TgtCell;

	/** Danno effettivo (Combat) o numero di celle percorse (Move). */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	int32 Amount = 0;

	/**
	 * IDENTITA' dell'azione che ha prodotto la voce, quando ne ha una (CP 5.5). `NAME_None` = non dichiarata.
	 *
	 * Serve perche' le reazioni degli eroi riusano la semantica delle azioni core: senza questo campo
	 * `Bastion.Interposition` e `Action.Intercept` produrrebbero voci IDENTICHE, e un replay non potrebbe piu'
	 * dire quale abilita' e' scattata. E' l'ActionId del catalogo, cioe' la chiave stabile: non cambia mai.
	 *
	 * Oggi lo popolano le voci di categoria `Reaction`. Le altre categorie lo lasciano vuoto — completare i
	 * reason code delle voci di combattimento e' CP 11.3, e riempirlo a meta' qui direbbe meno del nulla.
	 */
	UPROPERTY(BlueprintReadOnly, Category = "RefactorTactics|TurnLog")
	FName ActionId;

	FRTTurnLogEntry() = default;
};

/**
 * Versione del formato di serializzazione binaria del TurnLog. Ogni formato serializzato e' versionato
 * (invariante #4): il loader rifiuta versioni sconosciute invece di interpretare byte arbitrari.
 * Non e' UENUM (uint16 esce dai vincoli UHT del BlueprintType uint8): e' una costante di formato interna.
 */
enum class ERTTurnLogFormatVersion : uint16
{
	Initial      = 1, // header + voci, senza checksum (mai persistito su file)
	WithChecksum = 2, // + checksum FNV del payload in coda: rileva la corruzione del contenuto
	/**
	 * + `ActionId` per voce (CP 5.5), scritto come lunghezza uint16 e byte UTF-8 in coda alla voce.
	 * E' il primo campo a lunghezza variabile del formato. Le tracce in versione 2 restano LEGGIBILI: il
	 * loader le accetta e ne lascia l'ActionId vuoto, che e' esattamente cio' che quei byte dicevano.
	 */
	WithActionId = 3,
	/**
	 * + `FormatId` nell'HEADER (CP 10.3): l'identita' del formato di partita in vigore, subito dopo i flags.
	 * Sta nell'header e non nelle voci perche' sarebbe una costante ripetuta N volte, e **non entra
	 * nell'hash**: l'hash mescola i campi delle voci, e includervi un campo di contesto invaliderebbe in
	 * blocco ogni hash golden. Le tracce in versione 3 restano LEGGIBILI, con `FormatId` neutro.
	 */
	WithFormatId = 4
};

/**
 * Topologia della griglia a cui appartengono le celle del log, dichiarata nei flags dell'header.
 * Le voci portano 3 interi per cella: nel quadrato sono offset (X,Y,Layer), nell'esagonale assiali (q,r,Layer).
 * Senza questo marcatore le due tracce sarebbero indistinguibili e un confronto incrociato darebbe un falso
 * "nessuna divergenza". Square = 0 -> i file scritti prima di questa estensione restano leggibili.
 * Non e' UENUM (uint16, come ERTTurnLogFormatVersion): e' una costante di formato interna.
 */
enum class ERTLogTopology : uint16
{
	Square = 0,
	Hex    = 1
};
