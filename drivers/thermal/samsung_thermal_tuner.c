#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/thermal.h>
#include <linux/err.h>

static int cpu_throttle_temp = 85000;
static int cpu_critical_temp = 95000;
static int gpu_throttle_temp = 80000;
static int battery_temp_limit = 42000;

static struct kobject *samsung_thermal_kobj;
static struct thermal_zone_device *tz_dev;

static ssize_t cpu_throttle_temp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", cpu_throttle_temp);
}
static ssize_t cpu_throttle_temp_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%d", &cpu_throttle_temp);
    if (tz_dev) thermal_zone_device_update(tz_dev);
    return count;
}
static struct kobj_attribute cpu_throttle_temp_attr = __ATTR(cpu_throttle_temp, 0644, cpu_throttle_temp_show, cpu_throttle_temp_store);

static ssize_t cpu_critical_temp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", cpu_critical_temp);
}
static ssize_t cpu_critical_temp_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%d", &cpu_critical_temp);
    if (tz_dev) thermal_zone_device_update(tz_dev);
    return count;
}
static struct kobj_attribute cpu_critical_temp_attr = __ATTR(cpu_critical_temp, 0644, cpu_critical_temp_show, cpu_critical_temp_store);

static ssize_t gpu_throttle_temp_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", gpu_throttle_temp);
}
static ssize_t gpu_throttle_temp_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%d", &gpu_throttle_temp);
    if (tz_dev) thermal_zone_device_update(tz_dev);
    return count;
}
static struct kobj_attribute gpu_throttle_temp_attr = __ATTR(gpu_throttle_temp, 0644, gpu_throttle_temp_show, gpu_throttle_temp_store);

static ssize_t battery_temp_limit_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf)
{
    return sprintf(buf, "%d\n", battery_temp_limit);
}
static ssize_t battery_temp_limit_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count)
{
    sscanf(buf, "%d", &battery_temp_limit);
    if (tz_dev) thermal_zone_device_update(tz_dev);
    return count;
}
static struct kobj_attribute battery_temp_limit_attr = __ATTR(battery_temp_limit, 0644, battery_temp_limit_show, battery_temp_limit_store);

static struct attribute *samsung_thermal_attrs[] = {
    &cpu_throttle_temp_attr.attr,
    &cpu_critical_temp_attr.attr,
    &gpu_throttle_temp_attr.attr,
    &battery_temp_limit_attr.attr,
    NULL,
};

static struct attribute_group samsung_thermal_attr_group = {
    .attrs = samsung_thermal_attrs,
};

static struct thermal_zone_device_ops samsung_tz_ops = {
};

static int __init samsung_thermal_tuner_init(void)
{
    int ret;

    samsung_thermal_kobj = kobject_create_and_add("samsung_thermal", kernel_kobj);
    if (!samsung_thermal_kobj)
        return -ENOMEM;

    ret = sysfs_create_group(samsung_thermal_kobj, &samsung_thermal_attr_group);
    if (ret) {
        kobject_put(samsung_thermal_kobj);
        return ret;
    }

    tz_dev = thermal_zone_device_register("samsung_thermal_tuner", 0, 0, NULL, &samsung_tz_ops, NULL, 0, 0);
    if (IS_ERR(tz_dev)) {
        tz_dev = NULL;
    }

    return 0;
}

static void __exit samsung_thermal_tuner_exit(void)
{
    if (tz_dev)
        thermal_zone_device_unregister(tz_dev);
    sysfs_remove_group(samsung_thermal_kobj, &samsung_thermal_attr_group);
    kobject_put(samsung_thermal_kobj);
}

module_init(samsung_thermal_tuner_init);
module_exit(samsung_thermal_tuner_exit);

MODULE_LICENSE("GPL");
