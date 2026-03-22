# HadesKernel Custom Build

## Project Identity
- Kernel: hadesKernel custom build
- Source: https://github.com/corsicanu/android_kernel_samsung_universal8895
- Devices: Samsung Galaxy Note8 (greatlte) / S8 (dreamlte) / S8+ (dream2lte)
- SoC: Samsung Exynos 8895 (universal8895), dual-cluster ARM big.LITTLE
  - Little cluster: CPUs 0-3 (Cortex-A53), 455MHz to 1690MHz
  - Big cluster: CPUs 4-7 (Mongoose M2), 741MHz to 2314MHz
  - GPU: Mali-G71 MP20
- Base Linux kernel: 4.4.x
- Target ROM: HadesROM Q v3.0 (Android 10 / OneUI 2.5)
- Build target: Stable daily driver with root + performance + thermal control

---

## TASK 1: KernelSU Next Integration

IMPORTANT: kprobe is NOT available on kernel 4.4. Use manual hooks ONLY. Never use kprobe method.

Step 1a: Add KernelSU Next as submodule by running this command in kernel root:
curl -LSs "https://raw.githubusercontent.com/KernelSU-Next/KernelSU-Next/main/kernel/setup.sh" | bash -s main

Step 1b: Edit file fs/exec.c
Find the function: do_execveat_common
Add this block BEFORE the function prototype (before the line that starts with "static int do_execveat_common"):

#ifdef CONFIG_KSU
extern bool ksu_execveat_hook __read_mostly;
extern int ksu_handle_execveat(int *fd, struct filename **filename_ptr, void *argv, void *envp, int *flags);
extern int ksu_handle_execveat_sucompat(int *fd, struct filename **filename_ptr, void *argv, void *envp, int *flags);
#endif

Add this block INSIDE the function body, before the first return statement:

#ifdef CONFIG_KSU
    if (unlikely(ksu_execveat_hook))
        ksu_handle_execveat(&fd, &filename, &argv, &envp, &flags);
    else
        ksu_handle_execveat_sucompat(&fd, &filename, &argv, &envp, &flags);
#endif

Step 1c: Edit file fs/read_write.c
Find the function: vfs_read
Add this block at the very top of the function body, as the first lines inside the opening brace:

#ifdef CONFIG_KSU
extern bool ksu_vfs_read_hook __read_mostly;
extern int ksu_handle_vfs_read(struct file **file_ptr, char __user **buf_ptr, size_t *count_ptr, loff_t **pos);
    if (unlikely(ksu_vfs_read_hook))
        ksu_handle_vfs_read(&file, &buf, &count, &pos);
#endif

Step 1d: Edit file fs/open.c
Find the function: do_faccessat
Add this block at the top of the function body, as the first lines inside the opening brace:

#ifdef CONFIG_KSU
extern int ksu_handle_faccessat(int *dfd, const char __user **filename_user, int *mode, int *flags);
    ksu_handle_faccessat(&dfd, &filename, &mode, NULL);
#endif

---

## TASK 2: Thermal Management Driver

Create a new file: drivers/thermal/samsung_thermal_tuner.c

This driver must expose a sysfs group at the path: /sys/kernel/samsung_thermal/

Create the following sysfs nodes, all with read/write permissions (0644):
- cpu_throttle_temp   default value: 85000  (this is millidegrees Celsius, so 85000 = 85 degrees C)
- cpu_critical_temp   default value: 95000
- gpu_throttle_temp   default value: 80000
- battery_temp_limit  default value: 42000

Each write to these nodes must call thermal_zone_device_update() on the relevant thermal zone.
Register the driver with the kernel thermal framework using thermal_zone_device_register().
Use kobject_create_and_add() to create the sysfs group under kernel_kobj.
Use module_init and module_exit macros.
Set MODULE_LICENSE to "GPL".

---

## TASK 3: CPU and GPU Performance Tuner Driver

Create a new file: drivers/cpufreq/hades_governor_tuner.c

This driver must expose a sysfs group at the path: /sys/kernel/hades_perf/

Create the following sysfs nodes, all with read/write permissions (0644):
- little_max_freq     default value: 1690000 (kHz)
- little_min_freq     default value: 455000 (kHz)
- big_max_freq        default value: 2314000 (kHz)
- big_min_freq        default value: 741000 (kHz)
- cpu_governor        default value: schedutil
- io_scheduler        default value: cfq
- gpu_max_freq        default value: 900 (MHz, Mali-G71 MP20)
- gpu_min_freq        default value: 266 (MHz)

When any freq node is written, call cpufreq_update_policy() on all relevant CPUs.
Little cluster is CPUs 0 to 3. Big cluster is CPUs 4 to 7.
When governor node is written, apply the new governor to all CPUs.
Use kobject_create_and_add() to create the sysfs group under kernel_kobj.
Use module_init and module_exit macros.
Set MODULE_LICENSE to "GPL".

---

## TASK 4: Bypass Charging and Charge Control Driver

Create a new file: drivers/power/hades_charging_control.c

The Samsung Exynos 8895 uses the MAX77865 charger IC.
The bypass charging interface is at: /sys/class/power_supply/battery/input_suspend
Writing 1 to input_suspend suspends charging input so the phone runs directly from the charger.

This driver must expose a sysfs group at the path: /sys/kernel/hades_charging/

Create the following sysfs nodes, all with read/write permissions (0644):
- bypass_charging      values: 0 for normal charging, 1 for bypass mode
                       This node must write to /sys/class/power_supply/battery/input_suspend internally
- charge_limit_percent default value: 100, range: 50 to 100
                       When battery reaches this percent, suspend charging input
- battery_protect      values: 0 for off, 1 for on
                       When set to 1, automatically sets charge_limit_percent to 85

IMPORTANT: If the hardware does not support bypass charging (input_suspend node does not exist),
the driver must still load cleanly, log a clear message with pr_info saying hardware not supported,
and return -ENODEV gracefully without crashing.

Use kobject_create_and_add() to create the sysfs group under kernel_kobj.
Use module_init and module_exit macros.
Set MODULE_LICENSE to "GPL".

---

## TASK 5: Update All Three Defconfig Files

Apply ALL of the following CONFIG entries to these three files:
- arch/arm64/configs/greatlte_defconfig   (Note8)
- arch/arm64/configs/dreamlte_defconfig   (S8)
- arch/arm64/configs/dream2lte_defconfig  (S8+)

Entries to add:
CONFIG_KSU=y
CONFIG_KSU_DEBUG=n
CONFIG_CPU_FREQ_GOV_SCHEDUTIL=y
CONFIG_CPU_FREQ_GOV_INTERACTIVE=y
CONFIG_HZ_300=y
CONFIG_IOSCHED_CFQ=y
CONFIG_IOSCHED_DEADLINE=y
CONFIG_DEFAULT_IOSCHED="cfq"
CONFIG_ZRAM=y
CONFIG_ZSMALLOC=y
CONFIG_CRYPTO_LZ4=y
CONFIG_SAMSUNG_THERMAL_TUNER=y
CONFIG_HADES_GOVERNOR_TUNER=y
CONFIG_HADES_CHARGING_CONTROL=y

---

## TASK 6: Update Kconfig Files

Add to drivers/thermal/Kconfig:
config SAMSUNG_THERMAL_TUNER
    bool "Samsung Exynos 8895 Thermal Tuner"
    default y
    help
      Exposes sysfs interface for thermal management on Exynos 8895 devices.

Add to drivers/cpufreq/Kconfig:
config HADES_GOVERNOR_TUNER
    bool "HadesKernel CPU and GPU Governor Tuner"
    default y
    help
      Exposes sysfs interface for CPU and GPU frequency and governor control.

Add to drivers/power/Kconfig:
config HADES_CHARGING_CONTROL
    bool "HadesKernel Charging Control"
    default y
    help
      Exposes sysfs interface for bypass charging and charge limit control.

Add to drivers/thermal/Makefile:
obj-$(CONFIG_SAMSUNG_THERMAL_TUNER) += samsung_thermal_tuner.o

Add to drivers/cpufreq/Makefile:
obj-$(CONFIG_HADES_GOVERNOR_TUNER) += hades_governor_tuner.o

Add to drivers/power/Makefile:
obj-$(CONFIG_HADES_CHARGING_CONTROL) += hades_charging_control.o

---

## Sysfs Node Reference for Companion App

Performance nodes:
/sys/kernel/hades_perf/little_max_freq
/sys/kernel/hades_perf/little_min_freq
/sys/kernel/hades_perf/big_max_freq
/sys/kernel/hades_perf/big_min_freq
/sys/kernel/hades_perf/cpu_governor
/sys/kernel/hades_perf/io_scheduler
/sys/kernel/hades_perf/gpu_max_freq
/sys/kernel/hades_perf/gpu_min_freq

Thermal nodes:
/sys/kernel/samsung_thermal/cpu_throttle_temp
/sys/kernel/samsung_thermal/cpu_critical_temp
/sys/kernel/samsung_thermal/gpu_throttle_temp
/sys/kernel/samsung_thermal/battery_temp_limit

Charging nodes:
/sys/kernel/hades_charging/bypass_charging
/sys/kernel/hades_charging/charge_limit_percent
/sys/kernel/hades_charging/battery_protect

Read only info nodes:
/sys/class/thermal/thermal_zone0/temp   (CPU temperature in millidegrees)
/sys/class/thermal/thermal_zone8/temp   (GPU temperature in millidegrees)
/sys/class/power_supply/battery/capacity (battery percentage)
/sys/kernel/ksu/version                  (KernelSU status, exists if active)

---

## Critical Rules the AI Must Follow

1. NEVER remove or modify any existing hadesKernel features
2. NEVER break compatibility with Samsung proprietary blobs
3. NEVER use kprobe based KernelSU integration - use manual hooks only
4. ALWAYS wrap all KernelSU code in #ifdef CONFIG_KSU and #endif guards
5. ALWAYS make bypass charging fail gracefully if hardware not supported
6. ALWAYS add Makefile and Kconfig entries for every new driver file
7. Apply defconfig changes to ALL THREE device defconfigs, not just one
8. After all patches are applied, verify the kernel compiles with make greatlte_defconfig and make -j$(nproc)
9. If Possible turn off the Knox e-fuse.
