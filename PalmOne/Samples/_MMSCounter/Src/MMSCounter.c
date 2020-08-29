/*
 * MMSCounter.c
 *
 * main file for MMSCounter
 *
 * This wizard-generated code is based on code adapted from the
 * stationery files distributed as part of the Palm OS SDK 4.0.
 *
 * Copyright (c) 1999-2000 Palm, Inc. or its subsidiaries.
 * All rights reserved.
 */
 
#include <PalmOS.h>
#include <PalmOSGlue.h>
#include <HsHelper.h>
#include <HsCreators.h>
#include <MmsHelperCommon.h>
#include <HelperServiceClass.h>
#include <HsExt.h>

#include "MMSCounter.h"
#include "MMSCounter_Rsc.h"

/*********************************************************************
 * Entry Points
 *********************************************************************/

/*********************************************************************
 * Global variables
 *********************************************************************/

/*********************************************************************
 * Internal Constants
 *********************************************************************/

/* Define the minimum OS version we support */
#define ourMinVersion    sysMakeROMVersion(3,0,0,sysROMStageDevelopment,0)
#define kPalmOS20Version sysMakeROMVersion(2,0,0,sysROMStageDevelopment,0)

/*********************************************************************
 * Internal Functions
 *********************************************************************/

/*
 * FUNCTION: GetObjectPtr
 *
 * DESCRIPTION:
 *
 * This routine returns a pointer to an object in the current form.
 *
 * PARAMETERS:
 *
 * formId
 *     id of the form to display
 *
 * RETURNED:
 *     address of object as a void pointer
 */

static void * GetObjectPtr(UInt16 objectID)
{
	FormType * frmP;

	frmP = FrmGetActiveForm();
	return FrmGetObjectPtr(frmP, FrmGetObjectIndex(frmP, objectID));
}

/*
 * FUNCTION: MainFormInit
 *
 * DESCRIPTION: This routine initializes the MainForm form.
 *
 * PARAMETERS:
 *
 * frm
 *     pointer to the MainForm form.
 */

static void MainFormInit(FormType *frmP)
{
	FieldType *field;
	const char *wizardDescription;
	UInt16 fieldIndex;

	fieldIndex = FrmGetObjectIndex(frmP, MainDescriptionField);
	field = (FieldType *)FrmGetObjectPtr(frmP, fieldIndex);
	
	// Set focus to 'Count' button instead
	//FrmSetFocus(frmP, fieldIndex);
	FrmSetFocus(frmP, noFocus);
	FrmSetFocus(frmP, FrmGetObjectIndex(frmP,MainClearTextButton));
	FrmDrawForm(frmP);
			
	wizardDescription =
		"App is registered for New Message Count  notification\n"
		"Tap on the button to retrieve number of unread MMS, SMS, and total unread messages "
		;
				
	/* dont stack FldInsert calls, since each one generates a
	 * fldChangedEvent, and multiple uses can overflow the event queue */
	FldInsert(field, wizardDescription, StrLen(wizardDescription));
}

/*
 * FUNCTION: MainFormDoCommand
 *
 * DESCRIPTION: This routine performs the menu command specified.
 *
 * PARAMETERS:
 *
 * command
 *     menu item id
 */

static Boolean MainFormDoCommand(UInt16 command)
{
	Boolean handled = false;

	UInt16  cardNo;
	LocalID dbID;
	DmSearchStateType searchState;

	DmGetNextDatabaseByTypeCreator(true, &searchState, sysFileTApplication,
									 appFileCreator, true, &cardNo, &dbID);


	switch (command)
	{
		case OptionsAbout:
		MenuEraseStatus(0);
		HsAboutHandspringApp(cardNo, dbID, "2006", "Palm DTS Team");
		handled = true;
		break;

	}

	return handled;
}

/*
 * FUNCTION: MainFormHandleEvent
 *
 * DESCRIPTION:
 *
 * This routine is the event handler for the "MainForm" of this 
 * application.
 *
 * PARAMETERS:
 *
 * eventP
 *     a pointer to an EventType structure
 *
 * RETURNED:
 *     true if the event was handled and should not be passed to
 *     FrmHandleEvent
 */

static Boolean MainFormHandleEvent(EventType * eventP)
{
	Boolean handled = false;
	FormType * frmP = FrmGetActiveForm();

	switch (eventP->eType) 
	{
		case menuEvent:
			return MainFormDoCommand(eventP->data.menu.itemID);

		case frmOpenEvent:
			FrmDrawForm(frmP);
			MainFormInit(frmP);
			handled = true;
			break;
            
        case frmUpdateEvent:
			/* 
			 * To do any custom drawing here, first call
			 * FrmDrawForm(), then do your drawing, and
			 * then set handled to true. 
			 */
			break;
			
		case ctlSelectEvent:
		{
			// Respond when users tap on 'Count' button
			if (eventP->data.ctlSelect.controlID == MainClearTextButton)
			{
				Char text[128] = "";
				UInt16 mmsCount;
				UInt16 smsCount;
				UInt16 totalCount;
				UInt16 prefSize = sizeof(UInt16);
				
				UInt16 fieldIndex = FrmGetObjectIndex(frmP, MainDescriptionField);
				FieldType * field = (FieldType*)GetObjectPtr(MainDescriptionField);

				PrefGetAppPreferences(appFileCreator, 0, &mmsCount, &prefSize, true);
				PrefGetAppPreferences(appFileCreator, 1, &totalCount, &prefSize, true);
				smsCount = totalCount - mmsCount ;

				if (field)
				{
				    Char *pText = NULL;
				    
					FrmSetFocus(frmP, fieldIndex);
					StrPrintF(text, "Unread MMS count: %d  \nUnread SMS count %d \nTotal unread messages count: %d", mmsCount, smsCount, totalCount);
					
					pText = FldGetTextPtr(field);
					if (pText)
    					FldDelete(field, 0, StrLen(pText));

					FldInsert(field, text, StrLen(text));
					FldDrawField(field);
				}

				break;
			}

			break;
		}
		
		default:
		    break;
	}
    
	return handled;
}

/*
 * FUNCTION: AppHandleEvent
 *
 * DESCRIPTION: 
 *
 * This routine loads form resources and set the event handler for
 * the form loaded.
 *
 * PARAMETERS:
 *
 * event
 *     a pointer to an EventType structure
 *
 * RETURNED:
 *     true if the event was handled and should not be passed
 *     to a higher level handler.
 */

static Boolean AppHandleEvent(EventType * eventP)
{
	UInt16 formId;
	FormType * frmP;

	if (eventP->eType == frmLoadEvent)
	{
		/* Load the form resource. */
		formId = eventP->data.frmLoad.formID;
		frmP = FrmInitForm(formId);
		FrmSetActiveForm(frmP);

		/* 
		 * Set the event handler for the form.  The handler of the
		 * currently active form is called by FrmHandleEvent each
		 * time is receives an event. 
		 */
		switch (formId)
		{
			case MainForm:
				FrmSetEventHandler(frmP, MainFormHandleEvent);
				break;

		}
		return true;
	}

	return false;
}

/*
 * FUNCTION: AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.
 */

static void AppEventLoop(void)
{
	UInt16 error;
	EventType event;

	do 
	{
		/* change timeout if you need periodic nilEvents */
		EvtGetEvent(&event, evtWaitForever);

		if (! SysHandleEvent(&event))
		{
			if (! MenuHandleEvent(0, &event, &error))
			{
				if (! AppHandleEvent(&event))
				{
					FrmDispatchEvent(&event);
				}
			}
		}
	} while (event.eType != appStopEvent);
}

/*
 * FUNCTION: AppStart
 *
 * DESCRIPTION:  Get the current application's preferences.
 *
 * RETURNED:
 *     errNone - if nothing went wrong
 */

static Err AppStart(void)
{
	//UInt16 msgCount = 99;
	UInt16 cardNo;
	LocalID dbID;
	Err err;
	SysNotifyParamType notify;

	MemSet (&notify, sizeof (SysNotifyParamType), 0);

//	PrefSetAppPreferences(appFileCreator, 0, 1, &msgCount, sizeof(UInt16), true);
//	PrefSetAppPreferences(appFileCreator, 1, 1, &msgCount, sizeof(UInt16), true);
	
	SysCurAppDatabase(&cardNo, &dbID);
	
	err = SysNotifyRegister(cardNo, dbID, pmNotifyBroadcastNewMsgCount, NULL, sysNotifyNormalPriority, NULL);

	if (!err)
	{
		PmGetNewMsgCountNotifyParamType msgCountParam;
  
		msgCountParam.helperServiceClass = kHelperServiceClassIDMMS; // Use to count unread MMS

		notify.notifyType = pmNotifyGetNewMsgCountFromHelper;
		notify.broadcaster = appFileCreator;
		notify.notifyDetailsP = &msgCountParam;

		// This function will generate pmNotifyBroadcastNewMsgCount event (kHelperServiceClassIDMMS)
		err = SysNotifyBroadcast(&notify);

		msgCountParam.helperServiceClass = kHelperServiceClassIDSMS; // Use to count unread SMS and MMS

		notify.notifyType = pmNotifyGetNewMsgCountFromHelper;
		notify.broadcaster = appFileCreator;
		notify.notifyDetailsP = &msgCountParam;

		// This function will generate pmNotifyBroadcastNewMsgCount event (kHelperServiceClassIDSMS)
		err = SysNotifyBroadcast(&notify);

	}

	return err;
}

/*
 * FUNCTION: AppStop
 *
 * DESCRIPTION: Save the current state of the application.
 */

static void AppStop(void)
{
	UInt16 cardNo;
	LocalID dbID;
	Err err;
	
	SysCurAppDatabase(&cardNo, &dbID);
	
	err = SysNotifyUnregister(cardNo, dbID, pmNotifyBroadcastNewMsgCount, sysNotifyNormalPriority);
	
	/* Close all the open forms. */
	FrmCloseAllForms();

}

/*
 * FUNCTION: RomVersionCompatible
 *
 * DESCRIPTION: 
 *
 * This routine checks that a ROM version is meet your minimum 
 * requirement.
 *
 * PARAMETERS:
 *
 * requiredVersion
 *     minimum rom version required
 *     (see sysFtrNumROMVersion in SystemMgr.h for format)
 *
 * launchFlags
 *     flags that indicate if the application UI is initialized
 *     These flags are one of the parameters to your app's PilotMain
 *
 * RETURNED:
 *     error code or zero if ROM version is compatible
 */

static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
	UInt32 romVersion;

	/* See if we're on in minimum required version of the ROM or later. */
	FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
	if (romVersion < requiredVersion)
	{
		if ((launchFlags & 
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
			(sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
		{
			FrmAlert (RomIncompatibleAlert);

			/* Palm OS versions before 2.0 will continuously relaunch this
			 * app unless we switch to another safe one. */
			if (romVersion < kPalmOS20Version)
			{
				AppLaunchWithCommand(
					sysFileCDefaultApp, 
					sysAppLaunchCmdNormalLaunch, NULL);
			}
		}

		return sysErrRomIncompatible;
	}

	return errNone;
}

/*
 * FUNCTION: PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 * 
 * PARAMETERS:
 *
 * cmd
 *     word value specifying the launch code. 
 *
 * cmdPB
 *     pointer to a structure that is associated with the launch code
 *
 * launchFlags
 *     word value providing extra information about the launch.
 *
 * RETURNED:
 *     Result of launch, errNone if all went OK
 */

UInt32 PilotMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
	Err error;
	SysNotifyParamType* paramP = (SysNotifyParamType*) cmdPBP;
			
	error = RomVersionCompatible (ourMinVersion, launchFlags);
	if (error) return (error);

	switch (cmd)
	{
		case sysAppLaunchCmdNormalLaunch:
			error = AppStart();
			if (error) 
				return error;

			/* 
			 * start application by opening the main form
			 * and then entering the main event loop 
			 */
			FrmGotoForm(MainForm);
			AppEventLoop();

			AppStop();
			break;
			
		case sysAppLaunchCmdNotify:
			// Notification sent by an application in response to
			// a pmNotifyGetNewMsgCountFromHelper notification.
			if (paramP->notifyType == pmNotifyBroadcastNewMsgCount)
			{
				// Get unread message numbers
				PmBroadcastNewMsgCountNotifyParamPtr msgParamP =
					(PmBroadcastNewMsgCountNotifyParamPtr) paramP->notifyDetailsP;

				
				if (msgParamP->helperServiceClass == kHelperServiceClassIDMMS)
    				PrefSetAppPreferences(appFileCreator, 0, 1, &msgParamP->msgCount, sizeof(UInt16), true);
    		    else if (msgParamP->helperServiceClass == kHelperServiceClassIDSMS) 
    		    	// The number we get here is the number of total unread messages, including MMS and SMS
    		        PrefSetAppPreferences(appFileCreator, 1, 1, &msgParamP->msgCount, sizeof(UInt16), true);
    		        
			}
			break;
	}

	return errNone;
}
