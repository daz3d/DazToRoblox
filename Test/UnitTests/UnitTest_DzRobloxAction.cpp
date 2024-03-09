#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzRobloxAction.h"
#include "DzRobloxAction.h"


UnitTest_DzRobloxAction::UnitTest_DzRobloxAction()
{
	m_testObject = (QObject*) new DzRobloxAction();
}

bool UnitTest_DzRobloxAction::runUnitTests()
{
	RUNTEST(_DzBridgeRobloxAction);
	RUNTEST(executeAction);
	RUNTEST(createUI);
	RUNTEST(writeConfiguration);
	RUNTEST(setExportOptions);
	RUNTEST(readGuiRootFolder);

	return true;
}

bool UnitTest_DzRobloxAction::_DzBridgeRobloxAction(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(new DzRobloxAction());
	return bResult;
}

bool UnitTest_DzRobloxAction::executeAction(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzRobloxAction*>(m_testObject)->executeAction());
	return bResult;
}

bool UnitTest_DzRobloxAction::createUI(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzRobloxAction*>(m_testObject)->createUI());
	return bResult;
}

bool UnitTest_DzRobloxAction::writeConfiguration(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzRobloxAction*>(m_testObject)->writeConfiguration());
	return bResult;
}

bool UnitTest_DzRobloxAction::setExportOptions(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	DzFileIOSettings arg;
	TRY_METHODCALL(qobject_cast<DzRobloxAction*>(m_testObject)->setExportOptions(arg));
	return bResult;
}

bool UnitTest_DzRobloxAction::readGuiRootFolder(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzRobloxAction*>(m_testObject)->readGuiRootFolder());
	return bResult;
}


#include "moc_UnitTest_DzRobloxAction.cpp"

#endif
