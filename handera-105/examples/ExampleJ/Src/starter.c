/******************************************************************************
 *
 * File: Starter.c
 *
 * Project : Example J
 *
 *****************************************************************************/

#include <PalmOS.h>
#include <VfsMgr.h>
#include <NotifyMgr.h>
#include "StarterRsc.h"

#include "Vga.h"          
#include "Audio.h"
#include "TrgChars.h"
#include "MainForm.h"
#include "starter.h"

/***********************************************************************
 *
 *   Internal Structures
 *
 ***********************************************************************/
typedef struct 
{
    UInt8 replaceme;
} StarterPreferenceType;

typedef struct 
{
    UInt8 replaceme;
} StarterAppInfoType;

/***********************************************************************
 *
 *   Global variables
 *
 ***********************************************************************/
typedef StarterAppInfoType* StarterAppInfoPtr;

Boolean vgaPresent;
Boolean audioPresent;
Boolean auxButtonPresent;
Boolean cardChangedOccurred;

// Define the minimum OS version we support (2.0 for now).
#define ourMinVersion   sysMakeROMVersion(2,0,0,sysROMStageRelease,0)

/***********************************************************************
 *
 * FUNCTION:    RomVersionCompatible
 *
 * DESCRIPTION: This routine checks that a ROM version is meet your
 *              minimum requirement.
 *
 * PARAMETERS:  requiredVersion - minimum rom version required
 *                                (see sysFtrNumROMVersion in SystemMgr.h 
 *                                for format)
 *              launchFlags     - flags that indicate if the application 
 *                                UI is initialized.
 *
 * RETURNED:    error code or zero if rom is compatible
 *
 ***********************************************************************/
static Err RomVersionCompatible(UInt32 requiredVersion, UInt16 launchFlags)
{
    UInt32 romVersion;
 
/*------------------------------------------------------------------------
 * See if we're on in minimum required version of the ROM or later.
 *----------------------------------------------------------------------*/
    FtrGet(sysFtrCreator, sysFtrNumROMVersion, &romVersion);
    if (romVersion < requiredVersion)
    {
        if ((launchFlags & (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp)) ==
           (sysAppLaunchFlagNewGlobals | sysAppLaunchFlagUIApp))
        {
            FrmAlert (RomIncompatibleAlert);
        
/*------------------------------------------------------------------------
 * Palm OS 1.0 will continuously relaunch this app unless we switch to 
 * another safe one.
 *----------------------------------------------------------------------*/
            if (romVersion < ourMinVersion)
            {
                  AppLaunchWithCommand(sysFileCDefaultApp, sysAppLaunchCmdNormalLaunch, NULL);
            }
        }
        return sysErrRomIncompatible;
   }
 
   return errNone;
}

/***********************************************************************
 *
 * FUNCTION:    AppHandleEvent
 *
 * DESCRIPTION: This routine loads form resources and set the event
 *              handler for the form loaded.
 *
 * PARAMETERS:  event  - a pointer to an EventType structure
 *
 * RETURNED:    true if the event has handle and should not be passed
 *              to a higher level handler.
 *
 ***********************************************************************/
static Boolean AppHandleEvent(EventPtr eventP)
{
    UInt16  formId;
    FormPtr frmP;

    if (eventP->eType == frmLoadEvent)
    {
/*------------------------------------------------------------------------
 * Load the form resource.
  *----------------------------------------------------------------------*/
        formId = eventP->data.frmLoad.formID;
        frmP   = FrmInitForm(formId);
        FrmSetActiveForm(frmP);
/*------------------------------------------------------------------------
 * Set the event handler for the form.  The handler of the currently
 * active form is called by FrmHandleEvent each time is receives an event.
 *----------------------------------------------------------------------*/
        switch (formId)
        {
            case MainForm:
                FrmSetEventHandler(frmP, MainFormHandleEvent);
                break;
            default:
                ErrFatalDisplay("Invalid Form Load Event");
                break;
 
        }
        return(true);
    }
    
    return(false);
}

/***********************************************************************
 *
 * FUNCTION:    AppEventLoop
 *
 * DESCRIPTION: This routine is the event loop for the application.  
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void AppEventLoop(void)
{
    UInt16 error;
    EventType event;

    do 
    {
        EvtGetEvent(&event, evtWaitForever);
 
        if (! SysHandleEvent(&event))
            if (! MenuHandleEvent(0, &event, &error))
                if (! AppHandleEvent(&event))
                    FrmDispatchEvent(&event);
                    
    } while (event.eType != appStopEvent);
}


/***********************************************************************
 *
 * FUNCTION:     AppStart
 *
 * DESCRIPTION:  
 *
 * PARAMETERS:   nothing
 *
 * RETURNED:     Err value 0 if nothing went wrong
 *
 ***********************************************************************/
static Err AppStart(void)
{
    StarterPreferenceType prefs;
    UInt16                prefsSize;
    UInt32                version, keys;
    UInt16                cardNo;
    LocalID               appID;

/*------------------------------------------------------------------------
 * Read the saved preferences / saved-state information.
 *----------------------------------------------------------------------*/
    prefsSize = sizeof(StarterPreferenceType);
    if (PrefGetAppPreferences(appFileCreator, appPrefID, &prefs, &prefsSize, true) != 
            noPreferenceFound)
    {
    }

/*------------------------------------------------------------------------
 * Verify system has VFS Manager
 *----------------------------------------------------------------------*/
    if (FtrGet(sysFileCVFSMgr, vfsFtrIDVersion, &version) != errNone)
    {
        FrmAlert(VfsExtNotFoundAlert);
        return(-1);
    }    

/*------------------------------------------------------------------------
 * Make sure Audio Extension is present
 *----------------------------------------------------------------------*/
    if (_TRGAudioFeaturePresent(&version))          
        audioPresent  = true;
    else    
    {
        audioPresent = false;
        FrmAlert(AudioExtNotFoundAlert);
        return(-1);
    }    

/*------------------------------------------------------------------------
 * Check for an Aux Button
 *----------------------------------------------------------------------*/
    if (_TRGKeyFeaturePresent(&keys) && (keys & keyBitAux))
        auxButtonPresent = true;
    else
        auxButtonPresent = false;

/*------------------------------------------------------------------------
 * Check to see if VGA extension present
 *----------------------------------------------------------------------*/
    if (_TRGVGAFeaturePresent(&version))          
        vgaPresent  = true;
    else
        vgaPresent = false;

/*------------------------------------------------------------------------
 * Register for Card Removal/Insertion notifications
 *----------------------------------------------------------------------*/
    SysCurAppDatabase(&cardNo, &appID);
    SysNotifyRegister(0, appID, sysNotifyVolumeMountedEvent,   NULL, sysNotifyNormalPriority, NULL);
    SysNotifyRegister(0, appID, sysNotifyVolumeUnmountedEvent, NULL, sysNotifyNormalPriority, NULL);

    cardChangedOccurred = false;

    return(errNone);
}

/***********************************************************************
 *
 * FUNCTION:    AppStop
 *
 * DESCRIPTION: Save the current state of the application.
 *
 * PARAMETERS:  nothing
 *
 * RETURNED:    nothing
 *
 ***********************************************************************/
static void AppStop(void)
{
    StarterPreferenceType prefs;
    LocalID               appID;
    UInt16                cardNo;

/*------------------------------------------------------------------------
 * Write the saved preferences / saved-state information.  This data 
 * will be backed up during a HotSync.
 *----------------------------------------------------------------------*/
    PrefSetAppPreferences (appFileCreator, appPrefID, appPrefVersionNum, 
                           &prefs, sizeof (prefs), true);

/*------------------------------------------------------------------------
 * Unregister for Card Removal/Insertions
 *----------------------------------------------------------------------*/
    SysCurAppDatabase(&cardNo, &appID);
    SysNotifyUnregister(0, appID, sysNotifyVolumeMountedEvent,   sysNotifyNormalPriority);
    SysNotifyUnregister(0, appID, sysNotifyVolumeUnmountedEvent, sysNotifyNormalPriority);

/*------------------------------------------------------------------------
 * Close all the open forms.
 *----------------------------------------------------------------------*/
    FrmCloseAllForms ();
}


/***********************************************************************
 *
 * FUNCTION:    StarterPalmMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 *
 * RETURNED:    Result of launch
 *
 ***********************************************************************/
static UInt32 StarterPalmMain(UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    Err     error;
    Boolean draw;
 
    if ((error = RomVersionCompatible (ourMinVersion, launchFlags)) != 0)
        return (error);
 
    switch (cmd)
    {
        case sysAppLaunchCmdNormalLaunch:
            if ((error = AppStart()) != errNone)
                return(error);
                
            if (vgaPresent)
                VgaSetScreenMode(screenMode1To1, rotateModeNone);

            FrmGotoForm(MainForm);
            AppEventLoop();
            AppStop();
            break;

        case sysAppLaunchCmdNotify : // HandEra popup silk screen
            switch (((SysNotifyParamType *)cmdPBP)->notifyType)
            {
                case sysNotifyVolumeMountedEvent :
                    if ((FrmGetActiveFormID() == MainForm) && FrmVisible(FrmGetActiveForm()))
                        draw = true;
                    else
                    {
                        draw = false;
                        cardChangedOccurred = true;
                    }    
                    MainFormHandleVolumeMounted(draw);
                    break;
                case sysNotifyVolumeUnmountedEvent :
                    if ((FrmGetActiveFormID() == MainForm) && FrmVisible(FrmGetActiveForm()))
                        draw = true;
                    else
                    {
                        draw = false;
                        cardChangedOccurred = true;
                    }    
                    MainFormHandleVolumeUnmounted(draw);
                    break;
            }    
            break;    

        default:
            break;
  
    }
    return(errNone);
}


/***********************************************************************
 *
 * FUNCTION:    PilotMain
 *
 * DESCRIPTION: This is the main entry point for the application.
 *
 * PARAMETERS:  cmd - word value specifying the launch code. 
 *              cmdPB - pointer to a structure that is associated with the launch code. 
 *              launchFlags -  word value providing extra information about the launch.
 * RETURNED:    Result of launch
 *
 ***********************************************************************/
UInt32 PilotMain( UInt16 cmd, MemPtr cmdPBP, UInt16 launchFlags)
{
    return StarterPalmMain(cmd, cmdPBP, launchFlags);
}

