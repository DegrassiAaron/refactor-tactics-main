#include "Misc/AutomationTest.h"

#include "Components/SkeletalMeshComponent.h"
#include "Engine/World.h"
#include "Misc/ScopeExit.h" // ON_SCOPE_EXIT: il mondo si distrugge anche sui `return false` intermedi
#include "Unit/RTUnit.h"
#include "Unit/RTUnitAnimInstance.h"

#if WITH_DEV_AUTOMATION_TESTS

/**
 * 🔴 **Il canale della presentazione discreta, e cosa questi test possono davvero misurare.**
 *
 * `ARTUnit::PlayPresentationRole` fa due cose: **sceglie** una clip e la **suona**. Solo la prima e'
 * osservabile qui, e la ragione e' strutturale, non una limitazione da aggirare:
 *
 *  - `RTScenarioSession.cpp:807` spawna `ARTUnit::StaticClass()` e **non** i `BP_Unit_*`;
 *  - la Skeletal Mesh la aggiunge il Blueprint, quindi in un mondo di test non c'e';
 *  - senza skeletal non c'e' `AnimInstance`, e senza `AnimInstance` non c'e' niente da riprodurre.
 *
 * ⚠️ **Un test che asserisse «l'animazione giusta e' partita» sarebbe verde per costruzione**, perche'
 * nessuna delle configurazioni confrontate produrrebbe un effetto da annullare. E' la stessa classe di
 * difetto di `#1763`, dove `FRTUnitAnimClipsTest` restava verde su un grafo mai inizializzato.
 *
 * 🔑 Quindi l'oracolo e' `ResolvePresentationClip`: **quale clip il canale ha scelto**. E' deterministico,
 * falsificabile, e non dipende dal rendering. La riproduzione resta giudicabile solo a schermo, ed e'
 * `#2444`.
 *
 * ⚠️ **`FindHeroSkeletal` e `ContactGhost` sono `protected`**: questi test usano solo la superficie
 * pubblica, e tengono il puntatore alla skeletal che creano invece di richiederla all'unita'. Non e' un
 * dettaglio di stile — un test che avesse bisogno di allargare la visibilita' della classe sotto esame
 * starebbe misurando l'implementazione invece del contratto.
 */
namespace
{
	/**
	 * Una clip finta: conta l'IDENTITA' del path, non che risolva. Nessun pack Paragon richiesto.
	 *
	 * ⚠️ **Il nome porta il suffisso `Canale` per necessita', non per gusto.** Unreal compila i `.cpp` in
	 * unity build (`Module.RefactorTactics.N.cpp`), quindi due `namespace` anonimi di file diversi
	 * finiscono nella **stessa** translation unit: un helper omonimo in `RTAnimVariantTests.cpp` diventa
	 * un `C2084` — *«ha gia' un corpo»*. Misurato, non previsto.
	 */
	TSoftObjectPtr<UAnimSequenceBase> ClipCanale(const TCHAR* Nome)
	{
		return TSoftObjectPtr<UAnimSequenceBase>(
			FSoftObjectPath(FString::Printf(TEXT("/Game/Test/Anim/%s.%s"), Nome, Nome)));
	}

	FString PathCanale(const TSoftObjectPtr<UAnimSequenceBase>& Clip)
	{
		return Clip.ToSoftObjectPath().ToString();
	}

	/**
	 * Un'unita' con una skeletal d'eroe e il grafo attaccato, in un mondo di test.
	 *
	 * ⚠️ **La skeletal la aggiunge questo helper, non `ARTUnit`**: nel gioco la aggiunge il Blueprint, e
	 * senza di essa il canale non ha nulla su cui suonare — che e' il caso `MissingSkeletalIsHarmless`.
	 * Registrare il componente e' necessario perche' `GetComponents` lo veda.
	 *
	 * Restituisce la skeletal creata in `OutSkeletal`: e' l'unico modo di raggiungerla dall'esterno senza
	 * toccare `FindHeroSkeletal`, che e' `protected`.
	 */
	ARTUnit* UnitaConGrafo(UWorld* World, const FName& HeroId, USkeletalMeshComponent*& OutSkeletal)
	{
		OutSkeletal = nullptr;
		ARTUnit* Unit = World->SpawnActor<ARTUnit>();
		if (Unit == nullptr) { return nullptr; }
		Unit->HeroId = HeroId;

		USkeletalMeshComponent* Skeletal =
			NewObject<USkeletalMeshComponent>(Unit, TEXT("SkeletalEroeTest"));
		Skeletal->SetupAttachment(Unit->GetRootComponent());
		Skeletal->RegisterComponent();
		Skeletal->SetAnimInstanceClass(URTUnitAnimInstance::StaticClass());

		OutSkeletal = Skeletal;
		return Unit;
	}

	/** Il grafo attaccato a QUELLA skeletal, o `nullptr`. */
	URTUnitAnimInstance* GrafoDi(const USkeletalMeshComponent* Skeletal)
	{
		return Skeletal ? Cast<URTUnitAnimInstance>(Skeletal->GetAnimInstance()) : nullptr;
	}

	/** Sostituisce il ruolo `Role` dell'eroe con `Ruolo`, sull'ISTANZA e non sul CDO. */
	void ImpostaRuolo(URTUnitAnimInstance* Anim, const FName& HeroId,
		ERTPresentationRole Role, const FRTAnimRoleClips& Ruolo)
	{
		FRTHeroPresentationClips& Eroe = Anim->ClipsPerHero.FindOrAdd(HeroId);
		Eroe.PerRole.Add(Role, Ruolo);
	}

	/** Il ruolo vivo dentro l'istanza, per mutarlo dopo averlo impostato. */
	FRTAnimRoleClips* RuoloVivo(URTUnitAnimInstance* Anim, const FName& HeroId, ERTPresentationRole Role)
	{
		FRTHeroPresentationClips* Eroe = Anim ? Anim->ClipsPerHero.Find(HeroId) : nullptr;
		return Eroe ? Eroe->PerRole.Find(Role) : nullptr;
	}
}

// ─── CONTROLLO POSITIVO ─────────────────────────────────────────────────────────────────────────────
//
// 🔴 **Scritto per PRIMO, e non e' cerimonia.** I quattro test che seguono asseriscono in gran parte che
// il canale **non** restituisce niente. Senza un caso in cui restituisce qualcosa **davvero**, un
// `ResolvePresentationClip` che risponde sempre vuoto li passerebbe tutti.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimChannelResolvesActiveVariantTest,
	"RefactorTactics.Anim.Channel.ResolvesActiveVariant",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimChannelResolvesActiveVariantTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("mondo di test"), World)) { return false; }
	ON_SCOPE_EXIT { World->DestroyWorld(false); };

	const FName Eroe(TEXT("Hero.Test"));
	USkeletalMeshComponent* Skeletal = nullptr;
	ARTUnit* Unit = UnitaConGrafo(World, Eroe, Skeletal);
	URTUnitAnimInstance* Anim = GrafoDi(Skeletal);
	if (!TestNotNull(TEXT("l'unita' esiste"), Unit)) { return false; }
	if (!TestNotNull(TEXT("il grafo esiste sull'istanza"), Anim)) { return false; }

	FRTAnimRoleClips Ruolo;
	Ruolo.AddVariant(FName(TEXT("AV_A")), NAME_None, ClipCanale(TEXT("ClipA")));
	Ruolo.AddVariant(FName(TEXT("AV_B")), NAME_None, ClipCanale(TEXT("ClipB")));
	Ruolo.MakeActive(FName(TEXT("AV_A")));
	ImpostaRuolo(Anim, Eroe, ERTPresentationRole::Attack, Ruolo);

	// (1) L'attiva e' A -> il canale sceglie A.
	TestEqual(TEXT("il canale sceglie la variante attiva"),
		PathCanale(Unit->ResolvePresentationClip(ERTPresentationRole::Attack)),
		PathCanale(ClipCanale(TEXT("ClipA"))));

	// (2) 🔑 **Cambiare l'attiva CAMBIA la scelta.** E' la meta' che rende non vacui i test sotto: senza
	//     di essa, «non cambia» non distinguerebbe un canale corretto da uno inerte.
	FRTAnimRoleClips* Vivo = RuoloVivo(Anim, Eroe, ERTPresentationRole::Attack);
	if (!TestNotNull(TEXT("il ruolo e' raggiungibile"), Vivo)) { return false; }
	TestTrue(TEXT("MakeActive(B) riesce"), Vivo->MakeActive(FName(TEXT("AV_B"))));

	TestEqual(TEXT("ora il canale sceglie B"),
		PathCanale(Unit->ResolvePresentationClip(ERTPresentationRole::Attack)),
		PathCanale(ClipCanale(TEXT("ClipB"))));
	return true;
}

// ─── Nessuna attiva ─────────────────────────────────────────────────────────────────────────────────
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimChannelNoActiveResolvesToNothingTest,
	"RefactorTactics.Anim.Channel.NoActiveResolvesToNothing",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimChannelNoActiveResolvesToNothingTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("mondo di test"), World)) { return false; }
	ON_SCOPE_EXIT { World->DestroyWorld(false); };

	const FName Eroe(TEXT("Hero.Test"));
	USkeletalMeshComponent* Skeletal = nullptr;
	ARTUnit* Unit = UnitaConGrafo(World, Eroe, Skeletal);
	URTUnitAnimInstance* Anim = GrafoDi(Skeletal);
	if (!TestNotNull(TEXT("l'unita' esiste"), Unit)) { return false; }
	if (!TestNotNull(TEXT("il grafo esiste"), Anim)) { return false; }

	// Due varianti legate, NESSUNA attiva: e' lo stato in cui una entra sempre (`#2441`).
	FRTAnimRoleClips Ruolo;
	Ruolo.AddVariant(FName(TEXT("AV_A")), NAME_None, ClipCanale(TEXT("ClipA")));
	Ruolo.AddVariant(FName(TEXT("AV_B")), NAME_None, ClipCanale(TEXT("ClipB")));
	ImpostaRuolo(Anim, Eroe, ERTPresentationRole::Attack, Ruolo);

	// Anti-vacuita': le varianti ci sono davvero, quindi il vuoto sotto non e' «non c'era niente».
	const FRTAnimRoleClips* Vivo = RuoloVivo(Anim, Eroe, ERTPresentationRole::Attack);
	if (!TestNotNull(TEXT("il ruolo e' raggiungibile"), Vivo)) { return false; }
	TestEqual(TEXT("due varianti legate"), Vivo->Variants.Num(), 2);
	TestTrue(TEXT("e nessuna e' attiva"), Vivo->ActiveClipVariant.IsNone());

	TestTrue(TEXT("nessuna clip scelta senza un'attiva"),
		Unit->ResolvePresentationClip(ERTPresentationRole::Attack).IsNull());
	return true;
}

// ─── 🔴 L'attiva sparisce, la sorella NON viene promossa ────────────────────────────────────────────
//
// **E' il test che porta il valore di questa issue.** Va validato per mutazione: si scrive nel canale la
// scelta automatica del primo sibling e **questo solo test** deve diventare rosso.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimChannelVanishedActiveDoesNotPromoteSiblingTest,
	"RefactorTactics.Anim.Channel.VanishedActiveDoesNotPromoteSibling",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimChannelVanishedActiveDoesNotPromoteSiblingTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("mondo di test"), World)) { return false; }
	ON_SCOPE_EXIT { World->DestroyWorld(false); };

	const FName Eroe(TEXT("Hero.Test"));
	USkeletalMeshComponent* Skeletal = nullptr;
	ARTUnit* Unit = UnitaConGrafo(World, Eroe, Skeletal);
	URTUnitAnimInstance* Anim = GrafoDi(Skeletal);
	if (!TestNotNull(TEXT("l'unita' esiste"), Unit)) { return false; }
	if (!TestNotNull(TEXT("il grafo esiste"), Anim)) { return false; }

	FRTAnimRoleClips Ruolo;
	Ruolo.AddVariant(FName(TEXT("AV_A")), NAME_None, ClipCanale(TEXT("ClipA")));
	Ruolo.AddVariant(FName(TEXT("AV_B")), NAME_None, ClipCanale(TEXT("ClipB")));
	Ruolo.MakeActive(FName(TEXT("AV_A")));
	ImpostaRuolo(Anim, Eroe, ERTPresentationRole::Attack, Ruolo);

	// Premessa esplicita: senza di essa il vuoto finale non proverebbe niente.
	if (!TestEqual(TEXT("premessa: il canale sceglie A"),
		PathCanale(Unit->ResolvePresentationClip(ERTPresentationRole::Attack)),
		PathCanale(ClipCanale(TEXT("ClipA"))))) { return false; }

	// A sparisce. B e' ancora li', ed e' inattiva.
	FRTAnimRoleClips* Vivo = RuoloVivo(Anim, Eroe, ERTPresentationRole::Attack);
	if (!TestNotNull(TEXT("il ruolo e' raggiungibile"), Vivo)) { return false; }
	TestTrue(TEXT("rimozione di AV_A riesce"), Vivo->RemoveVariant(FName(TEXT("AV_A"))));

	TestEqual(TEXT("B e' rimasta"), Vivo->Variants.Num(), 1);
	TestTrue(TEXT("nessuna attiva dopo la rimozione"), Vivo->ActiveClipVariant.IsNone());

	// 🔑 Il canale risponde «nessuna», NON la clip di B. Eleggerla toglierebbe all'autore una scelta che
	//    e' sua, in silenzio — ed e' esattamente cio' che l'intera pipeline esiste per non fare.
	TestTrue(TEXT("il canale non sceglie niente"),
		Unit->ResolvePresentationClip(ERTPresentationRole::Attack).IsNull());
	TestNotEqual(TEXT("e in particolare NON ha scelto B"),
		PathCanale(Unit->ResolvePresentationClip(ERTPresentationRole::Attack)),
		PathCanale(ClipCanale(TEXT("ClipB"))));
	return true;
}

// ─── Un ruolo non popolato non e' un errore ─────────────────────────────────────────────────────────
//
// ⚠️ E' il caso NORMALE, non un margine: sette ruoli su nove non hanno oggi nessuna variante, ed e' la
// misura che da' il titolo a questa issue.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimChannelUnpopulatedRoleIsNotAnErrorTest,
	"RefactorTactics.Anim.Channel.UnpopulatedRoleIsNotAnError",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimChannelUnpopulatedRoleIsNotAnErrorTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("mondo di test"), World)) { return false; }
	ON_SCOPE_EXIT { World->DestroyWorld(false); };

	const FName Eroe(TEXT("Hero.Test"));
	USkeletalMeshComponent* Skeletal = nullptr;
	ARTUnit* Unit = UnitaConGrafo(World, Eroe, Skeletal);
	URTUnitAnimInstance* Anim = GrafoDi(Skeletal);
	if (!TestNotNull(TEXT("l'unita' esiste"), Unit)) { return false; }
	if (!TestNotNull(TEXT("il grafo esiste"), Anim)) { return false; }

	// Solo `Attack` e' popolato. Gli altri otto no.
	FRTAnimRoleClips Ruolo;
	Ruolo.AddVariant(FName(TEXT("AV_A")), NAME_None, ClipCanale(TEXT("ClipA")));
	Ruolo.MakeActive(FName(TEXT("AV_A")));
	ImpostaRuolo(Anim, Eroe, ERTPresentationRole::Attack, Ruolo);

	// Controllo positivo locale: il ruolo popolato risponde.
	if (!TestFalse(TEXT("premessa: Attack risponde"),
		Unit->ResolvePresentationClip(ERTPresentationRole::Attack).IsNull())) { return false; }

	const ERTPresentationRole NonPopolati[] = {
		ERTPresentationRole::Cast, ERTPresentationRole::Dash,
		ERTPresentationRole::Defend, ERTPresentationRole::Fall,
		ERTPresentationRole::Hit, ERTPresentationRole::Death
	};
	for (const ERTPresentationRole Role : NonPopolati)
	{
		TestTrue(TEXT("un ruolo non popolato non da' clip, e non e' un errore"),
			Unit->ResolvePresentationClip(Role).IsNull());
	}

	// Un eroe fuori catalogo: stessa risposta, stessa assenza di errore.
	Unit->HeroId = FName(TEXT("Hero.NonEsiste"));
	TestTrue(TEXT("un eroe fuori catalogo non da' clip"),
		Unit->ResolvePresentationClip(ERTPresentationRole::Attack).IsNull());
	return true;
}

// ─── Senza skeletal d'eroe il canale tace ───────────────────────────────────────────────────────────
//
// 🔑 **E' il caso di OGNI scenario headless**, non un margine: `RTScenarioSession.cpp:807` spawna
// `ARTUnit::StaticClass()`, e la skeletal la aggiunge il Blueprint. `PlayPresentationRole` deve
// attraversarlo senza dire niente — se qui crashasse, ogni scenario del corpus cadrebbe nel momento in
// cui il TurnManager chiamera' il canale.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTAnimChannelMissingSkeletalIsHarmlessTest,
	"RefactorTactics.Anim.Channel.MissingSkeletalIsHarmless",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTAnimChannelMissingSkeletalIsHarmlessTest::RunTest(const FString&)
{
	UWorld* World = UWorld::CreateWorld(EWorldType::Game, false);
	if (!TestNotNull(TEXT("mondo di test"), World)) { return false; }
	ON_SCOPE_EXIT { World->DestroyWorld(false); };

	// Nessuna skeletal d'eroe aggiunta: e' l'unita' come la spawnano gli scenari.
	ARTUnit* Unit = World->SpawnActor<ARTUnit>();
	if (!TestNotNull(TEXT("unita' spawnata"), Unit)) { return false; }
	Unit->HeroId = FName(TEXT("Hero.Gadget"));

	// ⚠️ Non si asserisce «zero componenti skeletal»: `ContactGhost` e' una `USkeletalMeshComponent` e
	// c'e' sempre. Cio' che conta e' che il canale non la scambi per la mesh dell'eroe — ed e' la
	// ragione per cui usa `FindHeroSkeletal`, che la esclude per IDENTITA'.
	TestTrue(TEXT("il canale non sceglie niente"),
		Unit->ResolvePresentationClip(ERTPresentationRole::Attack).IsNull());

	// E suonare non deve fare nulla, ne' crashare.
	Unit->PlayPresentationRole(ERTPresentationRole::Attack);
	TestTrue(TEXT("l'unita' e' ancora valida dopo una presentazione a vuoto"), IsValid(Unit));
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS
