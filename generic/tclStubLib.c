/* 
 * tclStubLib.c --
 *
 *	Stub object that will be statically linked into extensions that wish
 *	to access Tcl.
 *
 * Copyright (c) 1998-1999 by Scriptics Corporation.
 * Copyright (c) 1998 Paul Duffin.
 *
 * See the file "license.terms" for information on usage and redistribution
 * of this file, and for a DISCLAIMER OF ALL WARRANTIES.
 */

/*
 * We need to ensure that we use the stub macros so that this file contains
 * no references to any of the stub functions.  This will make it possible
 * to build an extension that references Tcl_InitStubs but doesn't end up
 * including the rest of the stub functions.
 */

#ifndef USE_TCL_STUBS
#define USE_TCL_STUBS
#endif
#undef USE_TCL_STUB_PROCS

#include "tclInt.h"
#include "tclPort.h"

/*
 * Ensure that Tcl_InitStubs is built as an exported symbol.  The other stub
 * functions should be built as non-exported symbols.
 */

TclStubs *tclStubsPtr = NULL;
TclPlatStubs *tclPlatStubsPtr = NULL;
TclIntStubs *tclIntStubsPtr = NULL;
TclIntPlatStubs *tclIntPlatStubsPtr = NULL;

static TclStubs *	HasStubSupport _ANSI_ARGS_((Tcl_Interp *interp));

typedef Tcl_Obj *(NewStringObjProc) _ANSI_ARGS_((CONST char *bytes,
				size_t length));


static TclStubs *
HasStubSupport (interp)
    Tcl_Interp *interp;
{
    Interp *iPtr = (Interp *) interp;

    if (!iPtr->stubTable) {
	/* No stub table at all? Nothing we can do. */
	return NULL;
    }
    if (iPtr->stubTable->magic != TCL_STUB_MAGIC) {
	/*
	 * We cannot acces interp->result and interp->freeProc
	 * any more: They will be gone in Tcl 9. In stead,
	 * assume that the iPtr->stubTable entry from Tcl_Interp
	 * and the Tcl_NewStringObj() and Tcl_SetObjResult() entries
	 * in the stub table don't change in Tcl 9. Need to add
	 * a test-case in Tcl 9 to assure that.
	 * 
	 * The signature of Tcl_NewStringObj will change: the length
	 * parameter will be of type size_t. But passing the value
	 * (size_t)-1 will work, whatever the signature will be.
	 */
	NewStringObjProc *newStringObj = (NewStringObjProc *)
		iPtr->stubTable->tcl_NewStringObj;
	iPtr->stubTable->tcl_SetObjResult(interp, newStringObj(
		"This extension is compiled for Tcl 8.x", (size_t)-1));
	return NULL;
    }
    return iPtr->stubTable;
}

/*
 *----------------------------------------------------------------------
 *
 * Tcl_InitStubs --
 *
 *	Tries to initialise the stub table pointers and ensures that
 *	the correct version of Tcl is loaded.
 *
 * Results:
 *	The actual version of Tcl that satisfies the request, or
 *	NULL to indicate that an error occurred.
 *
 * Side effects:
 *	Sets the stub table pointers.
 *
 *----------------------------------------------------------------------
 */

#ifdef Tcl_InitStubs
#undef Tcl_InitStubs
#endif

CONST char *
Tcl_InitStubs (interp, version, exact)
    Tcl_Interp *interp;
    CONST char *version;
    int exact;
{
    CONST char *actualVersion = NULL;
    ClientData pkgData = NULL;

    /*
     * We can't optimize this check by caching tclStubsPtr because
     * that prevents apps from being able to load/unload Tcl dynamically
     * multiple times. [Bug 615304]
     */

    tclStubsPtr = HasStubSupport(interp);
    if (!tclStubsPtr) {
	return NULL;
    }

    actualVersion = Tcl_PkgRequireEx(interp, "Tcl", version, exact, &pkgData);
    if (actualVersion == NULL) {
	return NULL;
    }
    tclStubsPtr = (TclStubs*)pkgData;

    if (tclStubsPtr->hooks) {
	tclPlatStubsPtr = tclStubsPtr->hooks->tclPlatStubs;
	tclIntStubsPtr = tclStubsPtr->hooks->tclIntStubs;
	tclIntPlatStubsPtr = tclStubsPtr->hooks->tclIntPlatStubs;
    } else {
	tclPlatStubsPtr = NULL;
	tclIntStubsPtr = NULL;
	tclIntPlatStubsPtr = NULL;
    }
    
    return actualVersion;
}
