#include <linux/module.h>
#include <linux/sysfs.h>
#include <linux/kobject.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/slab.h>

static int bypass_charging = 0;
static int charge_limit_percent = 100;
static int battery_protect = 0;

static struct kobject *hades_charging_kobj;

static void write_sysfs_string(const char *path, const char *val)
{
    struct file *f;
    loff_t pos = 0;
    mm_segment_t old_fs;

    f = filp_open(path, O_WRONLY, 0);
    if (!IS_ERR(f)) {
        old_fs = get_fs();
        set_fs(get_ds());
        vfs_write(f, (const char __force __user *)val, strlen(val), &pos);
        set_fs(old_fs);
        filp_close(f, NULL);
    }
}

static void apply_bypass_charging(int mode)
{
    if (mode) {
        write_sysfs_string("/sys/class/power_supply/battery/input_suspend", "1");
    } else {
        write_sysfs_string("/sys/class/power_supply/battery/input_suspend", "0");
    }
}

static ssize_t bypass_charging_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", bypass_charging);
}
static ssize_t bypass_charging_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%d", &bypass_charging);
    apply_bypass_charging(bypass_charging);
    return count;
}
static struct kobj_attribute bypass_charging_attr = __ATTR(bypass_charging, 0644, bypass_charging_show, bypass_charging_store);

static ssize_t charge_limit_percent_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", charge_limit_percent);
}
static ssize_t charge_limit_percent_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%d", &charge_limit_percent);
    if (charge_limit_percent < 50) charge_limit_percent = 50;
    if (charge_limit_percent > 100) charge_limit_percent = 100;
    return count;
}
static struct kobj_attribute charge_limit_percent_attr = __ATTR(charge_limit_percent, 0644, charge_limit_percent_show, charge_limit_percent_store);

static ssize_t battery_protect_show(struct kobject *kobj, struct kobj_attribute *attr, char *buf) {
    return sprintf(buf, "%d\n", battery_protect);
}
static ssize_t battery_protect_store(struct kobject *kobj, struct kobj_attribute *attr, const char *buf, size_t count) {
    sscanf(buf, "%d", &battery_protect);
    if (battery_protect == 1) {
        charge_limit_percent = 85;
    }
    return count;
}
static struct kobj_attribute battery_protect_attr = __ATTR(battery_protect, 0644, battery_protect_show, battery_protect_store);

static struct attribute *hades_charging_attrs[] = {
    &bypass_charging_attr.attr,
    &charge_limit_percent_attr.attr,
    &battery_protect_attr.attr,
    NULL,
};

static struct attribute_group hades_charging_attr_group = {
    .attrs = hades_charging_attrs,
};

static int __init hades_charging_control_init(void)
{
    int ret;
    struct file *f;

    f = filp_open("/sys/class/power_supply/battery/input_suspend", O_RDONLY, 0);
    if (IS_ERR(f)) {
        pr_info("hades_charging_control: Hardware bypass charging not supported (input_suspend missing).\n");
        return -ENODEV;
    }
    filp_close(f, NULL);

    hades_charging_kobj = kobject_create_and_add("hades_charging", kernel_kobj);
    if (!hades_charging_kobj)
        return -ENOMEM;

    ret = sysfs_create_group(hades_charging_kobj, &hades_charging_attr_group);
    if (ret) {
        kobject_put(hades_charging_kobj);
        return ret;
    }

    return 0;
}

static void __exit hades_charging_control_exit(void)
{
    sysfs_remove_group(hades_charging_kobj, &hades_charging_attr_group);
    kobject_put(hades_charging_kobj);
}

module_init(hades_charging_control_init);
module_exit(hades_charging_control_exit);

MODULE_LICENSE("GPL");
