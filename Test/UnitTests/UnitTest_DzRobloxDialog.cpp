#ifdef UNITTEST_DZBRIDGE

#include "UnitTest_DzRobloxDialog.h"
#include "DzRobloxDialog.h"


UnitTest_DzRobloxDialog::UnitTest_DzRobloxDialog()
{
	m_testObject = (QObject*) new DzRobloxDialog();
}

bool UnitTest_DzRobloxDialog::runUnitTests()
{
	RUNTEST(_DzBridgeRobloxDialog);
	RUNTEST(getIntermediateFolderEdit);
	RUNTEST(resetToDefaults);
	RUNTEST(loadSavedSettings);
	RUNTEST(HandleSelectIntermediateFolderButton);
	RUNTEST(HandleAssetTypeComboChange);

	return true;
}

bool UnitTest_DzRobloxDialog::_DzBridgeRobloxDialog(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(new DzRobloxDialog());
	return bResult;
}

bool UnitTest_DzRobloxDialog::getIntermediateFolderEdit(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzRobloxDialog*>(m_testObject)->getIntermediateFolderEdit());
	return bResult;
}

bool UnitTest_DzRobloxDialog::resetToDefaults(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzRobloxDialog*>(m_testObject)->resetToDefaults());
	return bResult;
}

bool UnitTest_DzRobloxDialog::loadSavedSettings(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzRobloxDialog*>(m_testObject)->loadSavedSettings());
	return bResult;
}

bool UnitTest_DzRobloxDialog::HandleSelectIntermediateFolderButton(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzRobloxDialog*>(m_testObject)->HandleSelectIntermediateFolderButton());
	return bResult;
}

bool UnitTest_DzRobloxDialog::HandleAssetTypeComboChange(UnitTest::TestResult* testResult)
{
	bool bResult = true;
	TRY_METHODCALL(qobject_cast<DzRobloxDialog*>(m_testObject)->HandleAssetTypeComboChange(0));
	return bResult;
}


#include "moc_UnitTest_DzRobloxDialog.cpp"
#endif
