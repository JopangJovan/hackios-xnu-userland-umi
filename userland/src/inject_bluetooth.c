/* iOS 16.5 (20F66) H4 initialization compatibility for QCA6390.
 * Apple's init sequence treats the optional Write Default Erroneous Data
 * Reporting command as mandatory. Skip that step only when the controller's
 * actual Supported Commands mask says it is absent. No HCI reply is altered. */
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOKitLib.h>
#include <mach/mach.h>
#include <mach-o/dyld.h>
#include <mach-o/loader.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

static void (*finish_init_step)(unsigned,const char *);
static void skip_optional_error_reporting(void)
{
    fprintf(stderr,"UMI Bluetooth: skipping unsupported optional error reporting\n");
    finish_init_step(0,"UMI optional erroneous-data reporting unavailable");
}
__attribute__((constructor)) static void initialize(void)
{
    io_service_t service=IOServiceGetMatchingService(0,IOServiceNameMatching("bluetooth"));
    if(!service) return;
    CFTypeRef mask=IORegistryEntryCreateCFProperty(service,CFSTR("HKBluetoothSupportedCommands"),kCFAllocatorDefault,0);
    IOObjectRelease(service);
    if(!mask) return;
    bool absent=CFGetTypeID(mask)==CFDataGetTypeID() && CFDataGetLength((CFDataRef)mask)==64 &&
        !(CFDataGetBytePtr((CFDataRef)mask)[18]&0x08);
    CFRelease(mask);
    if(!absent) return;
    const struct mach_header_64 *header=NULL;
    for(uint32_t i=0;i<_dyld_image_count();i++) {
        const struct mach_header *candidate=_dyld_get_image_header(i);
        const char *name=_dyld_get_image_name(i);
        if(candidate && candidate->filetype==MH_EXECUTE && name && !strcmp(name,"/usr/sbin/bluetoothd")) {
            header=(const struct mach_header_64 *)candidate;
            break;
        }
    }
    const uint8_t expected_uuid[16]={0x96,0x39,0x30,0xf1,0xd5,0x65,0x32,0xb5,0x91,0xd6,0x43,0xaf,0x71,0xe2,0xc8,0x5a};
    bool image_matches=false;
    if(!header || header->magic!=MH_MAGIC_64 || header->ncmds>1024) return;
    const uint8_t *commands=(const uint8_t *)(header+1),*end=commands+header->sizeofcmds;
    for(unsigned i=0;i<header->ncmds;i++) {
        if((size_t)(end-commands)<sizeof(struct load_command)) return;
        const struct load_command *command=(const void *)commands;
        if(command->cmdsize<sizeof(*command) || command->cmdsize>(size_t)(end-commands)) return;
        if(command->cmd==LC_UUID && command->cmdsize>=sizeof(struct uuid_command))
            {
            image_matches=!memcmp(((const struct uuid_command *)command)->uuid,expected_uuid,sizeof(expected_uuid));
        }
        commands+=command->cmdsize;
    }
    if(!image_matches) { fprintf(stderr,"UMI Bluetooth: unsupported daemon UUID; optional init unchanged\n"); return; }
    uintptr_t base=(uintptr_t)header;
    void (**step)(void)=(void *)(base+0x7fbde0);
    const uint32_t *original=(const void *)(base+0x10b970);
    const uint32_t *completion=(const void *)(base+0x1092d8);
    if((uintptr_t)*step!=(uintptr_t)original || original[0]!=0xd100c3ff ||
        original[1]!=0xa9014ff4 || completion[0]!=0xd10103ff || completion[1]!=0xa90157f6) {
        fprintf(stderr,"UMI Bluetooth: optional init ABI mismatch; unchanged\n"); return;
    }
    vm_address_t page=(vm_address_t)step&~((vm_address_t)vm_page_size-1);
    if(vm_protect(mach_task_self(),page,vm_page_size,FALSE,VM_PROT_READ|VM_PROT_WRITE|VM_PROT_COPY)!=KERN_SUCCESS) {
        fprintf(stderr,"UMI Bluetooth: cannot update private init table\n"); return;
    }
    finish_init_step=(void *)(base+0x1092d8);
    *step=skip_optional_error_reporting;
    vm_protect(mach_task_self(),page,vm_page_size,FALSE,VM_PROT_READ);
    fprintf(stderr,"UMI Bluetooth: enabled QCA optional-command compatibility\n");
}
