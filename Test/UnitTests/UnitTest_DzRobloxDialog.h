#pragma once
#ifdef UNITTEST_DZBRIDGE

#include <QObject>
#include "UnitTest.h"

class UnitTest_DzRobloxDialog : public UnitTest {
	Q_OBJECT
public:
	UnitTest_DzRobloxDialog();
	bool runUnitTests();

private:
	bool _DzBridgeRobloxDialog(UnitTest::TestResult* testResult);
	bool getIntermediateFolderEdit(UnitTest::TestResult* testResult);
	bool resetToDefaults(UnitTest::TestResult* testResult);
	bool loadSavedSettings(UnitTest::TestResult* testResult);
	bool HandleSelectIntermediateFolderButton(UnitTest::TestResult* testResult);
	bool HandleAssetTypeComboChange(UnitTest::TestResult* testResult);

};


#endif
