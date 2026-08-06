#include "Misc/AutomationTest.h"
#include "Unit/RTUnit.h"

#if WITH_DEV_AUTOMATION_TESTS

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTeamColorForTest,
	"RefactorTactics.Unit.TeamColorFor",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTeamColorForTest::RunTest(const FString&)
{
	const FLinearColor A(0.10f, 0.40f, 1.00f); // team 0 (blu)
	const FLinearColor B(1.00f, 0.20f, 0.15f); // team 1 (rosso)
	TestTrue(TEXT("team 0 -> A"), ARTUnit::TeamColorFor(0, A, B) == A);
	TestTrue(TEXT("team 1 -> B"), ARTUnit::TeamColorFor(1, A, B) == B);
	TestTrue(TEXT("default (>1) -> B"), ARTUnit::TeamColorFor(5, A, B) == B);
	return true;
}

// RingLocalZ: offset Z locale che porta un anello (figlio della mesh) al piano della cella.
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTRingLocalZTest,
	"RefactorTactics.Unit.RingLocalZ",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTRingLocalZTest::RunTest(const FString&)
{
	// Cilindro segnaposto: pivot al centro (offset 90) + scala Z 1.8 -> compensa a -49 (a terra).
	TestEqual(TEXT("cilindro (90, 1.8) -> -49"), ARTUnit::RingLocalZ(90.f, 1.8f), -49.f);
	// Skeletal: pivot ai piedi (offset 0) -> +1 (a livello base della mesh).
	TestEqual(TEXT("skeletal (0, 1.8) -> 1"), ARTUnit::RingLocalZ(0.f, 1.8f), 1.f);
	// Guardia div-by-zero: scala Z 0 -> fallback 1 (nessuna divisione).
	TestEqual(TEXT("scala 0 -> 1 (guardia)"), ARTUnit::RingLocalZ(90.f, 0.f), 1.f);
	return true;
}

#endif // WITH_DEV_AUTOMATION_TESTS

// ---------------------------------------------------------------------------------------------------------
// Scudo temporaneo: le abilita' di supporto danno protezione per UN turno, non permanente (issue #96)
// ---------------------------------------------------------------------------------------------------------

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTemporaryShieldExpiresTest,
	"RefactorTactics.Unit.TemporaryShieldExpiresAtCleanup",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTemporaryShieldExpiresTest::RunTest(const FString&)
{
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->MaxHealth = 100;
	Unit->Health = 100;
	Unit->Shield = 20; // scudo BASE dell'unita': non scade

	Unit->AddTemporaryShield(40);
	TestEqual(TEXT("lo scudo temporaneo si somma a quello base"), Unit->Shield, 60);

	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("a fine turno resta solo lo scudo base"), Unit->Shield, 20);

	// Due applicazioni nello stesso turno si sommano, e scadono insieme.
	Unit->AddTemporaryShield(25);
	Unit->AddTemporaryShield(15);
	TestEqual(TEXT("piu' scudi temporanei si sommano"), Unit->Shield, 60);
	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("scadono tutti insieme"), Unit->Shield, 20);

	// Senza scudo temporaneo la scadenza non tocca nulla.
	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("nessun temporaneo: lo scudo base resta intatto"), Unit->Shield, 20);
	return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FRTTemporaryShieldConsumedFirstTest,
	"RefactorTactics.Unit.DamageConsumesTemporaryShieldFirst",
	EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FRTTemporaryShieldConsumedFirstTest::RunTest(const FString&)
{
	ARTUnit* Unit = NewObject<ARTUnit>();
	if (!TestNotNull(TEXT("unita' di prova"), Unit)) { return false; }
	Unit->MaxHealth = 100;
	Unit->Health = 100;
	Unit->Shield = 20;
	Unit->AddTemporaryShield(40); // totale 60, di cui 40 temporanei

	// Il danno erode PRIMA la parte temporanea: e' quella che sta per scadere comunque, e cosi' lo scudo
	// base dell'unita' non viene consumato finche' c'e' protezione destinata a sparire.
	Unit->ApplyCombatState(/*NewHealth*/ 100, /*NewShield*/ 30); // 30 danni assorbiti
	TestEqual(TEXT("lo scudo totale scende col danno"), Unit->Shield, 30);

	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("il temporaneo residuo (10) scade, il base (20) resta"), Unit->Shield, 20);

	// Danno che supera l'intero scudo: dopo la scadenza non resta nulla (anche il base e' stato consumato).
	Unit->AddTemporaryShield(40);
	Unit->ApplyCombatState(/*NewHealth*/ 90, /*NewShield*/ 0); // scudo esaurito, il resto agli HP
	Unit->ExpireTemporaryShield();
	TestEqual(TEXT("scudo esaurito dal danno: resta 0, non un valore negativo"), Unit->Shield, 0);
	return true;
}
