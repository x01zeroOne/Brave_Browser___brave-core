

/* this ALWAYS GENERATED file contains the definitions for the interfaces */


 /* File created by MIDL compiler version 8.xx.xxxx */
/* at a redacted point in time
 */
/* Compiler settings for ../../brave/components/brave_vpn/browser/connection/win/brave_vpn_service/brave_vpn_service.idl:
    Oicf, W1, Zp8, env=Win64 (32b run), target_arch=AMD64 8.xx.xxxx 
    protocol : dce , ms_ext, c_ext, robust
    error checks: allocation ref bounds_check enum stub_data 
    VC __declspec() decoration level: 
         __declspec(uuid()), __declspec(selectany), __declspec(novtable)
         DECLSPEC_UUID(), MIDL_INTERFACE()
*/
/* @@MIDL_FILE_HEADING(  ) */

#pragma warning( disable: 4049 )  /* more than 64k source lines */


/* verify that the <rpcndr.h> version is high enough to compile this file*/
#ifndef __REQUIRED_RPCNDR_H_VERSION__
#define __REQUIRED_RPCNDR_H_VERSION__ 475
#endif

#include "rpc.h"
#include "rpcndr.h"

#ifndef __RPCNDR_H_VERSION__
#error this stub requires an updated version of <rpcndr.h>
#endif /* __RPCNDR_H_VERSION__ */

#ifndef COM_NO_WINDOWS_H
#include "windows.h"
#include "ole2.h"
#endif /*COM_NO_WINDOWS_H*/

#ifndef __brave_vpn_service_h__
#define __brave_vpn_service_h__

#if defined(_MSC_VER) && (_MSC_VER >= 1020)
#pragma once
#endif

#ifndef DECLSPEC_XFGVIRT
#if defined(_CONTROL_FLOW_GUARD_XFG)
#define DECLSPEC_XFGVIRT(base, func) __declspec(xfg_virtual(base, func))
#else
#define DECLSPEC_XFGVIRT(base, func)
#endif
#endif

/* Forward Declarations */ 

#ifndef __IBraveVpnService_FWD_DEFINED__
#define __IBraveVpnService_FWD_DEFINED__
typedef interface IBraveVpnService IBraveVpnService;

#endif 	/* __IBraveVpnService_FWD_DEFINED__ */


/* header files for imported files */
#include "oaidl.h"
#include "ocidl.h"

#ifdef __cplusplus
extern "C"{
#endif 


#ifndef __IBraveVpnService_INTERFACE_DEFINED__
#define __IBraveVpnService_INTERFACE_DEFINED__

/* interface IBraveVpnService */
/* [unique][helpstring][uuid][oleautomation][object] */ 


EXTERN_C const IID IID_IBraveVpnService;

#if defined(__cplusplus) && !defined(CINTERFACE)
    
    MIDL_INTERFACE("A949CB4E-C4F9-44C4-B213-6BF8AA9AC69C")
    IBraveVpnService : public IUnknown
    {
    public:
        virtual HRESULT STDMETHODCALLTYPE EnableVpn( 
            /* [string][in] */ const WCHAR *config) = 0;
        
    };
    
    
#else 	/* C style interface */

    typedef struct IBraveVpnServiceVtbl
    {
        BEGIN_INTERFACE
        
        DECLSPEC_XFGVIRT(IUnknown, QueryInterface)
        HRESULT ( STDMETHODCALLTYPE *QueryInterface )( 
            IBraveVpnService * This,
            /* [in] */ REFIID riid,
            /* [annotation][iid_is][out] */ 
            _COM_Outptr_  void **ppvObject);
        
        DECLSPEC_XFGVIRT(IUnknown, AddRef)
        ULONG ( STDMETHODCALLTYPE *AddRef )( 
            IBraveVpnService * This);
        
        DECLSPEC_XFGVIRT(IUnknown, Release)
        ULONG ( STDMETHODCALLTYPE *Release )( 
            IBraveVpnService * This);
        
        DECLSPEC_XFGVIRT(IBraveVpnService, EnableVpn)
        HRESULT ( STDMETHODCALLTYPE *EnableVpn )( 
            IBraveVpnService * This,
            /* [string][in] */ const WCHAR *config);
        
        END_INTERFACE
    } IBraveVpnServiceVtbl;

    interface IBraveVpnService
    {
        CONST_VTBL struct IBraveVpnServiceVtbl *lpVtbl;
    };

    

#ifdef COBJMACROS


#define IBraveVpnService_QueryInterface(This,riid,ppvObject)	\
    ( (This)->lpVtbl -> QueryInterface(This,riid,ppvObject) ) 

#define IBraveVpnService_AddRef(This)	\
    ( (This)->lpVtbl -> AddRef(This) ) 

#define IBraveVpnService_Release(This)	\
    ( (This)->lpVtbl -> Release(This) ) 


#define IBraveVpnService_EnableVpn(This,config)	\
    ( (This)->lpVtbl -> EnableVpn(This,config) ) 

#endif /* COBJMACROS */


#endif 	/* C style interface */




#endif 	/* __IBraveVpnService_INTERFACE_DEFINED__ */



#ifndef __BraveVpnServiceLib_LIBRARY_DEFINED__
#define __BraveVpnServiceLib_LIBRARY_DEFINED__

/* library BraveVpnServiceLib */
/* [helpstring][version][uuid] */ 



EXTERN_C const IID LIBID_BraveVpnServiceLib;
#endif /* __BraveVpnServiceLib_LIBRARY_DEFINED__ */

/* Additional Prototypes for ALL interfaces */

/* end of Additional Prototypes */

#ifdef __cplusplus
}
#endif

#endif


