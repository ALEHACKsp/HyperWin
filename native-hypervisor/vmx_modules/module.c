#include <vmm/vmm.h>
#include <vmx_modules/module.h>
#include <debug.h>

VOID MdlInitModule(IN PSHARED_CPU_DATA sharedData, IN PMODULE module, IN MODULE_INITIALIZER moduleInitializer,
    IN PGENERIC_MODULE_DATA moduleData, IN VMEXIT_HANDLER defaultHandler)
{
    for(QWORD i = 0; i < VMEXIT_HANDLERS_MAX; module->isHandledOnVmExit[i] = FALSE,
            module->vmExitHandlers[i] = NULL, i++);
    module->moduleName = NULL;
    if(defaultHandler)
    {
        module->hasDefaultHandler = TRUE;
        module->defaultHandler = defaultHandler;
    }
    if(moduleInitializer)
        ASSERT(moduleInitializer(sharedData, module, moduleData) == STATUS_SUCCESS);
}

VOID MdlRegisterVmExitHandler(IN PMODULE module, IN QWORD exitReason, IN VMEXIT_HANDLER handler)
{
    module->isHandledOnVmExit[exitReason] = TRUE;
    module->vmExitHandlers[exitReason] = handler;
}

VOID MdlRegisterModule(IN PSHARED_CPU_DATA sharedData, IN PMODULE module)
{
    PMODULE* newArr;
    
    sharedData->heap.allocate(&sharedData->heap, ++(sharedData->modulesCount) * sizeof(PMODULE), &newArr);
    HwCopyMemory(newArr, sharedData->modules, (sharedData->modulesCount - 1) * sizeof(PMODULE));
    newArr[sharedData->modulesCount - 1] = module;
    if(sharedData->modules)
        sharedData->heap.deallocate(&sharedData->heap, sharedData->modules);
    sharedData->modules = newArr;
}

VOID MdlSetModuleName(IN PSHARED_CPU_DATA sharedData, IN PMODULE module, IN PCHAR moduleName)
{
    sharedData->heap.allocate(&sharedData->heap, (StringLength(moduleName) + 1) * sizeof(CHAR),
        &(module->moduleName));
    HwCopyMemory(module->moduleName, moduleName, StringLength(moduleName) + 1);
}

STATUS MdlGetModuleByName(OUT PMODULE* module, IN PCHAR name)
{
    PSHARED_CPU_DATA shared;
    QWORD nameLength;

    shared = VmmGetVmmStruct()->currentCPU->sharedData;
    nameLength = StringLength(name);
    for(QWORD i = 0; i < shared->modulesCount; i++)
    {
        if(!HwCompareMemory(name, shared->modules[i]->moduleName, nameLength))
        {
            *module = shared->modules[i];
            return STATUS_SUCCESS;
        }
    }
    return STATUS_MODULE_NOT_FOUND;
}