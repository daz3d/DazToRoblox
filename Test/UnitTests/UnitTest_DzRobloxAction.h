#pragma once
#ifdef UNITTEST_DZBRIDGE

#include <QObject>
#include <UnitTest.h>

class UnitTest_DzRobloxAction : public UnitTest {
	Q_OBJECT
public:
	UnitTest_DzRobloxAction();
	bool runUnitTests();

private:
	bool _DzBridgeRobloxAction(UnitTest::TestResult* testResult);
	bool executeAction(UnitTest::TestResult* testResult);
	bool createUI(UnitTest::TestResult* testResult);
	bool writeConfiguration(UnitTest::TestResult* testResult);
	bool setExportOptions(UnitTest::TestResult* testResult);
	bool readGuiRootFolder(UnitTest::TestResult* testResult);

};

#endif
