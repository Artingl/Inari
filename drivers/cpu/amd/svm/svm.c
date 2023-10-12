#include <kernel/kernel.h>
#include <kernel/include/C/string.h>

#include <drivers/cpu/amd/svm/svm.h>
#include <drivers/impl.h>

#include <drivers/memory/pmm.h>

uintptr_t __host_state = NULL;
bool __svm_initialized = false;

int cpu_svm_make(struct SVM **svm)
{
    int r;
    struct SVM *ptr = kmalloc(sizeof(struct SVM));
    memset(svm, 0, sizeof(struct SVM));

    // check support for SVM
    if ((r = __cpu_svm_init()) != SVM_SUCCESS)
        goto err;

    ptr->__vmcb = pmm_alloc_frames(1);

    kident(ptr->__vmcb, PAGE_SIZE, KERN_PAGE_RW);
    memset(ptr->__vmcb, 0, PAGE_SIZE);

    goto end;
err:    
    cpu_svm_cleanup(svm);
    *svm = NULL;
    return r;
end:
    *svm = ptr;
    return SVM_SUCCESS;
}

void cpu_svm_cleanup(struct SVM* svm)
{
    if (svm->__vmcb != NULL)
    {
        // unmap page and deallocate memory frame for the VMCB
        pmm_free_frames(svm->__vmcb, 1);
        kunident(svm->__vmcb, PAGE_SIZE);
    }

    kfree(svm);
}

int __cpu_svm_init()
{
    int r = SVM_SUCCESS;
    uint32_t lo, hi;

    if (__svm_initialized)
        goto end;
    
    // check support for SVM
    if ((r = cpu_svm_support()) != SVM_SUCCESS)
        goto end;
    
    // set SVME bit in EFER so SVM instructions will be available
    __get_msr(IA32_EFER, &lo, &hi);
    lo |= IA32_EFER_SVME;
    __set_msr(IA32_EFER, lo, hi);

    kident(__host_state, PAGE_SIZE, KERN_PAGE_RW);
    memset(__host_state, 0, PAGE_SIZE);

    // set address to the memory which will contain host state when entering guest environment
    __set_msr(VM_HSAVE_PA, __host_state, 0x00);
    __get_msr(VM_HSAVE_PA, &lo, &hi);

    __svm_initialized = true;
end:
    return r;
}

int cpu_svm_support()
{
    // AMD Programmer's Manual, SVM 15.4
    
    uint32_t eax, ebx, ecx, edx, lo, hi;
    __cpuid(0x80000001, &eax, &ebx, &ecx, &edx);

    if (!(ecx & CPUID_SVM_FEATURE_BIT))
        return SVM_NOT_SUPPORTED;

    __get_msr(AMD_VM_CR, &lo, &hi);
    if (!(lo & AMD_VM_CR_SVMDIS))
        return SVM_SUCCESS;

    __cpuid(0x8000000A, &eax, &ebx, &ecx, &edx);
    if (!(edx & CPUID_SVM_SVML))
        return SVM_NOT_ENABLED;

    return SVM_DISABLED_WITH_KEY;
}
