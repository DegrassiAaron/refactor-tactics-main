#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Unit/RTGraykitTypes.h"
#include "RTGraykitLibrary.generated.h"

/**
 * Il valutatore procedurale del graykit: `posa = Evaluate(descriptor, tempo normalizzato)`.
 *
 * ## Perche' e' una funzione pura, e cosa cio' esclude
 *
 * 🔑 **Non ha stato, non tiene un `DeltaTime`, non ricorda il frame precedente.** E' la condizione che rende
 * la posa *seekabile*: un replay che salta al 70 % di un'azione ottiene la stessa posa che otterrebbe
 * arrivandoci un frame alla volta, e un test la misura senza far girare un mondo. Un'animazione a stato
 * incrementale non ha nessuna delle due proprieta', ed e' precisamente la forma che il mandato esclude.
 *
 * ⛔ **Non e' un secondo owner del pacing.** Il tempo normalizzato lo produce `URTPlaybackLibrary`
 * (`Turn/RTPlaybackLibrary.h`): `RouteAlpha` per la fase di movimento, `AlphaAtMicroStep` per il passo
 * discreto, `PhaseTime` per la fase. Questa libreria **consuma** quell'alpha e non ne calcola uno proprio —
 * se lo facesse, due orologi governerebbero la stessa risoluzione e il primo cambio di velocita' di playback
 * li sfaserebbe.
 *
 * ⛔ **Non e' un secondo owner della presentazione.** *Quale* azione si sta presentando lo dicono
 * `ERTPresentationRole` (#2441) e la tabella evento -> cue di `URTPresentationBindingLibrary`
 * ([D-278](../../../docs/decisions/RT_PDR_00_Decision_Log.md), #1801). Qui si decide soltanto *come si
 * deforma il corpo* mentre quell'azione scorre.
 *
 * ⛔ **E non tocca il gameplay.** Nessuna funzione di questa classe legge o scrive occupancy, pathfinding,
 * collisione o stato canonico: prendono struct di valore e ne restituiscono un'altra. La garanzia e'
 * strutturale — non c'e' un `AActor` in nessuna firma — e `FRTGraykitNoGameplayMutationTest` la misura
 * sull'unita' vera.
 */
UCLASS()
class REFACTORTACTICS_API URTGraykitLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:

	/**
	 * Semi-altezza di riferimento del cilindro graykit, in centimetri.
	 *
	 * ⚠️ **Duplica `ARTUnit::UnitHalfHeight` per NON includere `RTUnit.h`**, che trascinerebbe l'intera
	 * unita' — abilita', piano, turno — dentro una libreria che non conosce nessuna di quelle cose. La
	 * duplicazione e' sorvegliata: `FRTGraykitAnchorTest` confronta le due costanti e diventa rosso se
	 * divergono, che e' il modo di tenere un numero in due posti senza che il secondo invecchi in silenzio.
	 */
	static constexpr float DefaultHalfHeight = 90.f;

	/** Raggio di riferimento del cilindro, in centimetri. Stessa sorveglianza di `DefaultHalfHeight`. */
	static constexpr float DefaultRadius = 45.f;

	/**
	 * La frazione percorsa DENTRO la finestra dell'operatore, prima dell'easing.
	 *
	 * Restituisce `0` prima di `StartTime` e `1` dopo la fine — cioe' un operatore concluso **tiene** il suo
	 * valore finale invece di tornare a riposo.
	 *
	 * ⚠️ **Che tenga il valore finale e' una scelta, e ha un costo che va detto**: un `Translate` non
	 * riavvolto lascia il corpo spostato per il resto dell'azione. E' cio' che serve a un assestamento
	 * (`EaseOut` che arriva e resta), e cio' che NON serve a un impulso — per il quale esiste `PingPong`,
	 * che torna a zero da solo. Un operatore che tornasse sempre a riposo renderebbe il primo caso
	 * inesprimibile.
	 *
	 * ⛔ `Duration <= 0` restituisce `0`: l'operatore e' inerte, non permanente.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Graykit")
	static float WindowAlpha(const FRTGraykitOperator& Op, float NormalizedTime);

	/**
	 * Rimappa `[0,1] -> [0,1]` secondo l'easing.
	 *
	 * `PingPong` usa `Cycles` per contare quante andate-e-ritorno stanno nella finestra; gli altri lo
	 * ignorano. L'ingresso viene clampato: nessun easing estrapola fuori dalla finestra.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Graykit")
	static float ApplyEasing(ERTGraykitEasing Easing, float Alpha, float Cycles = 1.f);

	/**
	 * L'offset LOCALE di un anchor rispetto al pivot visivo dell'unita'.
	 *
	 * 🔑 **`Center` e' l'origine e `Ground` sta a `-HalfHeight`**: il pivot del cilindro segnaposto e' al
	 * centro, non ai piedi, ed e' la stessa convenzione che `ARTUnit::VisualZOffset` dichiara.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Graykit")
	static FVector AnchorOffset(ERTGraykitAnchor Anchor, float HalfHeight = 90.f, float Radius = 45.f);

	/** Gli anchor in ordine di dichiarazione. Serve ai test e a chi enumera senza reinventare l'elenco. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Graykit")
	static TArray<ERTGraykitAnchor> AllAnchors();

	/**
	 * Valuta il descriptor al tempo normalizzato dato e restituisce la posa.
	 *
	 * `NormalizedTime` viene clampato a `[0,1]`: un chiamante che passasse `1.4` ottiene la posa finale, non
	 * un'estrapolazione. ⚠️ Un `NaN` viene trattato come `0` — un solo campione avvelenato non deve poter
	 * propagare `NaN` nella trasformazione di un componente, dove diventerebbe un'unita' invisibile senza un
	 * errore.
	 *
	 * Un `Op` che questa build non conosce contribuisce **zero** e non interrompe la valutazione degli altri.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Graykit")
	static FRTGraykitPose Evaluate(const FRTGraykitDescriptor& Descriptor, float NormalizedTime,
		float HalfHeight = 90.f, float Radius = 45.f);

	/**
	 * Il descriptor di locomozione per uno stile.
	 *
	 * 🔴 **I quattro stili non condividono un descriptor scalato**: ognuno ha la sua composizione. `Run` non
	 * e' `Normal` con `Magnitude` piu' alta — porta un `Stretch` direzionale che `Normal` non ha e un
	 * `Lean` con perno a `Ground` di ampiezza diversa. E' il vincolo che il mandato pone (*«non considerare
	 * Run semplicemente un Move piu' veloce»*) e che `FRTGraykitMoveRunDistinctTest` misura contando gli
	 * operatori, non solo confrontando le pose.
	 *
	 * ⚠️ **I numeri qui dentro sono un punto di partenza, non un accordo di design.** Nessuno li ha ancora
	 * guardati a schermo: la validazione visuale appartiene a una seduta Editor, e finche' non avviene questi
	 * valori sono `NOT RUN` sul piano estetico per quanto verdi siano i test.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Graykit")
	static FRTGraykitDescriptor DescriptorForStyle(ERTGraykitLocomotionStyle Style);

	/** Gli stili in ordine di dichiarazione. */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Graykit")
	static TArray<ERTGraykitLocomotionStyle> AllStyles();

	/**
	 * Quanto due pose differiscono, in una sola cifra.
	 *
	 * Somma le distanze di offset (cm), le differenze angolari (gradi) e gli scarti di scala (frazione x100)
	 * delle tre parti. ⛔ **Non e' una metrica percettiva**: dice che due pose sono *diverse*, mai che sono
	 * *distinguibili a occhio* — quella e' una misura che si prende guardando, e appartiene alla seduta
	 * Editor. Serve ai test per avere una soglia invece di un confronto campo per campo.
	 */
	UFUNCTION(BlueprintPure, Category = "RefactorTactics|Graykit")
	static float PoseDistance(const FRTGraykitPose& A, const FRTGraykitPose& B);
};
