#include "ntddk.h"
#include "stdarg.h"
#include "stdio.h"
#include "ntddmou.h"

#if DBG
#define DbgPrint(arg) DbgPrint arg
#else
#define DbgPrint(arg)
#endif


//Global variables
PDEVICE_OBJECT       MouseDevice;
PDEVICE_OBJECT       HookDeviceObject;


NTSTATUS MouseFilterIrpHandle( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp );
NTSTATUS MouseFilterIrpPassthrough(IN PDEVICE_OBJECT DeviceObject,IN PIRP Irp );
NTSTATUS MyMouseFilterInit( IN PDRIVER_OBJECT DriverObject );


NTSTATUS DriverEntry(
    IN PDRIVER_OBJECT  DriverObject,
    IN PUNICODE_STRING RegistryPath )
{
    PDEVICE_OBJECT         deviceObject        = NULL;
    NTSTATUS               ntStatus;
    WCHAR                  deviceNameBuffer[]  = L"\\Device\\MyMouseFilter";
    UNICODE_STRING         deviceNameUnicodeString;
    WCHAR                  deviceLinkBuffer[]  = L"\\DosDevices\\MyMouseFilter";
    UNICODE_STRING         deviceLinkUnicodeString;
	__debugbreak();
    DbgPrint (("MyMouseFilter.SYS: entering DriverEntry\n"));

    //
    // Create our device.
    //
	    
    RtlInitUnicodeString (&deviceNameUnicodeString,
                          deviceNameBuffer );
    

 	 DriverObject->MajorFunction[IRP_MJ_READ]	    
			   = MouseFilterIrpHandle;
	 DriverObject->MajorFunction[IRP_MJ_CREATE]         =
	 DriverObject->MajorFunction[IRP_MJ_CLOSE]          =
	 DriverObject->MajorFunction[IRP_MJ_FLUSH_BUFFERS]  =
	 DriverObject->MajorFunction[IRP_MJ_CLEANUP]        =
	 DriverObject->MajorFunction[IRP_MJ_DEVICE_CONTROL] 
	                   = MouseFilterIrpPassthrough;

    return MyMouseFilterInit( DriverObject );
}

NTSTATUS MyMouseFilterInit( IN PDRIVER_OBJECT DriverObject )
{    CCHAR		 ntNameBuffer[64];
    STRING		 ntNameString;
    UNICODE_STRING       ntUnicodeString;
    NTSTATUS             status;

    sprintf( ntNameBuffer, "\\Device\\PointerClass0" );
    RtlInitAnsiString( &ntNameString, ntNameBuffer );
    RtlAnsiStringToUnicodeString( &ntUnicodeString, &ntNameString, TRUE );

    status = IoCreateDevice( DriverObject,
			     0,
			     NULL,
			     FILE_DEVICE_MOUSE ,
			     0,
			     FALSE,
			     &HookDeviceObject );

    if( !NT_SUCCESS(status) ) {

	 DbgPrint(("MyMouseFilter: Mouse hook failed to create device!\n"));

	 RtlFreeUnicodeString( &ntUnicodeString );

	 return STATUS_SUCCESS;
    }

    HookDeviceObject->Flags |= DO_BUFFERED_IO;

    status = IoAttachDevice( HookDeviceObject, &ntUnicodeString, &MouseDevice );

    if( !NT_SUCCESS(status) ) {

	 DbgPrint(("MyMouseFilter: Connect with mouse failed!\n"));

	 IoDeleteDevice( HookDeviceObject );

	 RtlFreeUnicodeString( &ntUnicodeString );
	
	 return STATUS_SUCCESS;
    }

    RtlFreeUnicodeString( &ntUnicodeString );

    DbgPrint(("Mouse connected to mouse device\n"));

    return STATUS_SUCCESS;
}

NTSTATUS MouseFilterIoCompletion( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp,
				IN PVOID Context )
{
    PIO_STACK_LOCATION        IrpSp;
    PMOUSE_INPUT_DATA      MouseData;

    IrpSp = IoGetCurrentIrpStackLocation( Irp );
    if( NT_SUCCESS( Irp->IoStatus.Status ) ) {
	
  	MouseData = Irp->AssociatedIrp.SystemBuffer;

	DbgPrint(("Got mouse Irp %s, LastX = %d, LastY = %d \n", ((MouseData->Flags == MOUSE_MOVE_RELATIVE) ? "Relative" : "_ABSOLUTE_"), MouseData->LastX, MouseData->LastY));

	if( MouseData->Flags == MOUSE_MOVE_RELATIVE) {
	//	DbgPrint(("Ralative"));
	//	DbgPrint(("X=%d\n",MouseData->LastX));
		DbgPrint(("Y=%d\n",MouseData->LastY));
		MouseData->LastX=-MouseData->LastX;
		MouseData->LastY=-MouseData->LastY;

       } 
	  if( MouseData->Flags == MOUSE_MOVE_ABSOLUTE) {
	//	DbgPrint(("Absolute"));
		MouseData->LastX = 0xFFFF - MouseData->LastX;
		MouseData->LastY = 0xFFFF - MouseData->LastY;
       }  
    }


    if( Irp->PendingReturned ) {

        IoMarkIrpPending( Irp );

    }

    return Irp->IoStatus.Status;
}


NTSTATUS MouseFilterIrpHandle( IN PDEVICE_OBJECT DeviceObject, IN PIRP Irp )
{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack    = IoGetNextIrpStackLocation(Irp);

    *nextIrpStack = *currentIrpStack;   

    IoSetCompletionRoutine( Irp, MouseFilterIoCompletion, DeviceObject, TRUE, TRUE, TRUE );
	    
    return IoCallDriver( MouseDevice, Irp );
}


NTSTATUS MouseFilterIrpPassthrough(
    IN PDEVICE_OBJECT DeviceObject,
    IN PIRP           Irp )
{
    PIO_STACK_LOCATION currentIrpStack = IoGetCurrentIrpStackLocation(Irp);
    PIO_STACK_LOCATION nextIrpStack    = IoGetNextIrpStackLocation(Irp);

    Irp->IoStatus.Status      = STATUS_SUCCESS;
    Irp->IoStatus.Information = 0;


   if( DeviceObject == HookDeviceObject ) {

       *nextIrpStack = *currentIrpStack;

       return IoCallDriver(MouseDevice, Irp );

   } else {

       return STATUS_SUCCESS;

   }
}


